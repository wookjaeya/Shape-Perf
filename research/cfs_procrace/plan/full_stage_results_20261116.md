# 첫 관측 주기 뒤의 단계: 결과 기록 (2026-11-16 시작)

이 문서는 첫 자동 관측 주기(`plan/observation_path_results_20261116.md` §1–§7, 검토 뒤 고침 §7.11, lead 의 결정 §7.12) 다음의 단계들을 적는다. 한 단계에 한 절이고, 절마다 선언, 바뀐 것, 시험, 바이트 동일성, 측정, 공간, 계획과 다르게 한 것을 둔다. 표기는 앞 결과 문서와 같다: [D] 일 전에 적은 선언, [F] 파일에서 읽음, [M] 명령으로 잼, [I] 추론·예상, [미확인] 근거가 아직 없음.

## 1. 판정 규칙 개정 2: 경로 A 부재 결과의 이름 결합 (문서만, 빌드 없음)

기록: `../records/judgement-rules-rev2-20261116/`. 출력 디렉터리는 만들지 않았다(측정 출력이 작아 기록 디렉터리에 둔다). Kairos 도구를 빌드하지 않고, 부팅하지 않고, push 하지 않는다.

### 1.0 선언 [D]

- **무엇을 하는가.** lead 의 결정 §7.12 항목 2 대로, 판정 규칙 문서 `plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` 에 날짜 붙은 "개정 2" 절(8절)을 덧붙여 경로 A 부재 결과와 게시자의 생성 사이의 이름 결합을 정의한다: 다섯 조건, 그것이 주는 상태, 빠진 조건마다의 값, 같은 조건 위의 refuted 규칙, 3.3·3.6 의 바뀜, C1 관계 밖의 대조 요구, 입력과 그것을 주는 뒤 단계, 규칙마다의 1차 근거. 개정 1 의 글은 바꾸지 않는다(머리 아래에 8절을 가리키는 날짜 붙은 한 줄만 더한다).
- **도구**: 빌드 없음. 정체성이 바뀌는 도구 없음. 다시 링크되는 바이너리 없음. Kairos 소스, 어휘 자료(`kairos/tests/static/data/reason_vocabulary.txt`), 설명 스크립트(`env/obs1/explain.py`)와 그 밖의 스크립트를 바꾸지 않는다.
- **읽기만 하는 측정**: `measure.sh`(이 기록 디렉터리; 쓰는 곳은 이 디렉터리와 scratchpad 뿐) — 어휘 시험을 지금 자료로, 그리고 바탕 사유 한 줄을 더한 scratchpad 사본으로 돌림; 다섯 빌드의 경로 A 행과 계획에 든 행; C1 부팅 기록의 키 글 사건; 프레임워크 계층 자료의 역할; C1 계획의 삭제 경로 site 수.
- **기록의 값**: 이 주기의 설명 기록은 다시 내지 않는다(§7.12 항목 2: 축 1 을 적힌 대로 쓴 `unlinked` 그대로).

### 1.1 바뀐 것 [F]

- `plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md`: 머리 아래 날짜 붙은 한 줄, 새 8절(8.0 지위·범위·내력, 8.1 변경표 N01–N12, 8.2 이름 결합 G-NAME, 8.3 refuted, 8.4 하위 사유 표, 8.5 3.3·3.6 의 바뀜과 검사 9–11, 8.6 대조, 8.7 입력과 뒤 단계, 8.8 관계 1 의 종이 적용, 8.9 규칙과 1차 근거 G1–G14, 8.10 어휘 자료, 8.11 남은 문제 8–13). 0–7절의 글은 그대로다(`git diff` 는 덧붙임뿐).
- 새 기록 디렉터리 `records/judgement-rules-rev2-20261116/`: `measure.sh`, `vocab_test.txt`, `path_a_rows.tsv`, `c1_record_sites.txt`, `framework_roles.txt`, `unit_tools.txt`.
- 이 결과 문서(새).

### 1.2 시험 [M]

- 어휘 시험 `test_static_reasons`(MC 바이너리 sha256 `900e48cd…`): 지금 자료(`4b964d34…`)로 612 점검 통과, rc 0. 바탕 사유 한 줄을 더한 scratchpad 사본으로는 `test_static_reasons.cpp:64` 의 줄 수 점검 `[23 vs 22]` 에서 614 가운데 1 실패, rc 1. 자료는 앞뒤로 같다(`vocab_test.txt`; 사본은 지웠다).
- `env/tools/tests/run_unit.sh` 15/15(`unit_tools.txt`; 점검 15 의 설명 기록 fixture 20/20). 스크립트를 바꾸지 않았으므로 앞 단계와 같은 결과를 확인한 것이다.
- ctest 전체와 lint 는 돌리지 않았다: 빌드가 없고 Kairos 의 어떤 파일도 바뀌지 않았다. 어휘 자료를 읽는 하나는 위에서 돌렸다.

### 1.3 바이트 동일성 [M]

- 누적 목록 `env/tools/ledger_check.sh`: 45 행, 실패 0(덧붙임 앞뒤). 판정 규칙 문서의 sha256 을 담은 목록은 없다(`records/*.tsv`, `records/*/*.sha256`, `plan/*/SHA256SUMS` 에서 이름 0) [M].
- 도구 등록부 `env/tools/tools_check.sh --deep`: 31 행, 실패 0.

### 1.4 측정 [M]

- 다섯 빌드의 경로 A 행(`path_a_rows.tsv`; relations 는 `plangen_relcheck/<빌드>/relations.txt`, 계획은 `plangen/<빌드>/plan.side`):

| 빌드 | relations sha256 | 경로 A 행 | 계획에 든 행 | 계획된 행 |
|---|---|---|---|---|
| c1_pre_tx | `7c7396d2…` | 127 | 127 | 5 (A21, A22, A23, A50, A51) |
| c1_post_tx | `0a3bf6d9…` | 127 | 127 | 4 (A22, A23, A50, A51) |
| r2_core | `3a6fcf81…` | 68 | 68 | 0 |
| c5_641 | `b62fd1f2…` | 66 | 66 | 0 |
| c5_642 | `996d32bd…` | 66 | 66 | 0 |

- 2026-11-16 검토 뒤 정정: §6.7 `rules-04`. 위 표의 계획된 경로 A 행(c1_pre_tx 의 A22, A23, A50, A51 과 c1_post_tx 의 넷)은 판정 규칙 8.6 (b)·(c)가 대조로 든 행이다. relations 규칙 개정 7(§2) 뒤의 모든 계획(`relrev7_plans`, `order_rev1b_plans`, `schema12_plans`, `extfix_plans`, `revfix_plans`)에서 경로 A 의 계획된 행은 c1_pre_tx 의 A21 하나이고, 그것은 규칙을 맞춘 C1 행이다. 그래서 지금 계획의 부팅은 8.6 의 사전 시험이 되지 않는다(판정 규칙 8.12 (ㄷ)). 위 수는 그날의 계획의 것으로 둔다.
- C1 부팅 기록(`c1_record_sites.txt`; sidecar `8f5f200c…`, dump 글 `1290a5f6…`): A20 은 `tier=3 … state=cut`. 관계 1 의 행의 키 글 site 둘: 272(`OS_CountSemCreate` 안 `memchr`, `osapi-countsem.c:91`), 273(`OS_CountSemGetIdByName` 안 `OS_ObjectIdFindByName`, `:212`). 사건: 273 에 행위자 12 aseq 26 "BP0_tsem", 행위자 13 aseq 11 "BP0_tsem"·aseq 15 "BP1_tsem"(모두 arg0 = 0x3); 272 에 행위자 13 aseq 13 "BP0_tsem"·aseq 17 "BP1_tsem", 행위자 14 aseq 4 "SCH_LAB"(모두 arg2 = 0x14). C1 계획의 삭제 경로 site(`OS_ObjectIdFinalizeDelete`, `OS_CountSemDelete`) 0.
- 프레임워크 계층 자료 `e2a39203…`(`framework_roles.txt`): run-loop 4, app-exit 4. 이름 공간 항목 0.
- 이 주기의 설명 기록 45 의 link 칸: unlinked 45, refuted 0(`records/obs1-20261116-reviewfix/index.tsv`).
- 이 수들이 판정 규칙 8절의 "오늘" 글과 8.6, 8.8 의 근거다. 규칙의 결과로 낸 기록은 없다(규칙을 구현한 스크립트가 없다).

### 1.5 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 측정 전 | 3,703,164,928 | 읽기 시작 |
| 09:45:57 | 3,702,517,760 | 측정과 문서 뒤 |

- 하한 314,572,800 B 를 넘지 않았다. 남긴 크기: 기록 디렉터리 8,163 B. 지운 것: scratchpad 의 어휘 사본(측정 스크립트가 지움)과 이 단계의 scratchpad 작업 파일. 그 밖에 아무것도 지우지 않았다.

### 1.6 계획과 다르게 한 것

- **어휘 자료에 바탕 사유를 더하지 않았다.** 이 단계의 지시는 3.5 와 어휘 자료에 사유를 더하라는 것이었다. 두 까닭으로 하지 않았다. (1) 판정 규칙 3.5 의 하위 사유 규칙: 빠진 조건은 모두 "자원이나 주체의 실행 내 식별이 없음"의 구체적 까닭이므로 `link_unlinked` 의 하위 사유이고, 3.5 는 표에 없는 까닭을 가장 가까운 사유의 하위 사유로 쓰라고 한다. 어휘 자료는 바탕 사유만 담는다(그 머리 줄 1–4). (2) 자료에 한 줄을 더하면 `test_static_reasons.cpp:64` 의 줄 수 점검이 실패한다(§1.2). 고치려면 시험 소스를 바꾸고 빌드해야 하는데, 이 단계는 문서만 바꾼다. 그래서 8.4 의 하위 사유 표(고정된 글 14 줄, 기존 둘 포함)를 3.5 에 더했고, 바탕 사유가 따로 필요하다고 정해지면 3.5 에 먼저 적고 자료와 시험을 그 사유를 내는 코드와 같은 선언된 빌드에서 바꾸는 차례를 8.10 에 적었다.
- **이름 결합의 상태를 새 값이 아니라 `established` 의 하위 종류(바탕 `name_binding`)로 정했다.** 3.3 의 `violation` 표의 글이 그대로 서고(`observed` 는 `established` 를 요구), `not_observed_in_runs` 가 그 바탕에서 나오지 않음이 경로 A 의 축 3 규칙에서 따른다(8.5). `odf = shown` 은 이 개정에서 바탕 `instance` 만 받는다고 명시했다(실행 사이의 대응 규칙이 없으므로).
- **기초 요건 여섯을 다섯 조건 앞에 명시했다.** 개정 1 과 설명 스크립트 개정 2 가 이미 요구하는 것(결합, 행위자 묶기, 부재 상수, 생성자 성공 상수, 키 글, 생성이 사용 뒤)이고 새 조건이 아니다. 생성자 성공 상수는 지금 행에 없어(§7.11.7 (b)) 이것만으로도 오늘의 관계 1 은 결합할 수 없다.
- **refuted 의 기록 단위 조건 셋**(8.3 의 (1)–(3))을 더했다. 쌍의 refuted 는 다섯 조건 위에서만 키 글의 차이를 쓰고(lead 의 결정), 기록의 refuted 는 행의 생성자 자리가 덮였을 때만이다. 관측하지 않은 생성자 자리를 남긴 채 기록을 `refuted_link` 로 빼면 빈칸을 정상으로 채우는 것이라서다(판정 규칙 0.2).
- **설명 스크립트의 refuted 갈래**(`env/obs1/explain.py:970-973`)가 개정 2 에 맞지 않음을 적었다. 이 주기의 기록에서 그 갈래는 한 번도 서지 않았다(refuted 0). 고치는 것은 이름 결합을 구현하는 다음 스크립트 판의 일이다.

### 1.7 열린 것

- 이름 결합의 입력(판정 규칙 8.7): 이름 공간 항목(공통 의미 단계), P0 `strcmp` 의 소스 대조(같은 단계; 로컬 사본 밖은 PI 의 선택), 종류별 writer 와 표 선택 partition(relations 규칙 개정 7), 생성자 성공 상수를 행에(relations 규칙의 개정), 삭제·회수 site 의 계획(관측 계획 생성기의 다음 판), 이름의 수명(기록 분석 단계). 그때까지 설명 스크립트는 `unlinked` 를 낸다.
- 대조(8.6): A20 은 계획에서 잘려 오늘 대조할 수 없다. 대조의 예산과 계획은 뒤 단계에서 정한다. 다른 빌드의 경로 A 행은 실행 기록이 생기는 대로 같은 규칙을 돌린다.
- 이름 결합을 실행 사이의 대응(R08 (2))에 쓰는 규칙은 정하지 않았다.

## 2. relations 규칙 개정 7: 객체 정체성과 writer 의 전제 (kz-static 빌드 하나)

기록: `../records/relrules-rev7-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/relrev7_check/`(relations 다섯 빌드), `/home/user/work/redesign/obs1/relrev7_extract/`(다섯 빌드 다시 뽑기), `/home/user/work/redesign/obs1/relrev7_plans/`(계획 다섯 빌드), 모두 새 디렉터리. T8CF(`/home/user/work/redesign/step2a/tests_8c_fix`)와 앞선 출력 디렉터리에는 쓰지 않는다. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 2.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다.

- **무엇을 하는가.** kz-static relations 규칙 개정 7(규칙은 `kairos/docs/static_facts.md` §5o): (1) 경로 A 의 사용은 그 호출 사슬이 검색에 넘기는 비교 함수가 그 이름 공간의 것일 때만 그 이름 공간에 든다(A2 callback 검색); 다른 함수만 넘기면 빠지고(`left out:` 줄), 풀리지 않으면 행으로 남고 `link_unlinked(namespace membership unresolved)`. (2) partition 은 검색의 표 선택에 닿는 상수만 받고, 나머지는 `not a table selector:` 로 적는다. (3) 행이 쓰는 writer 집합마다 `writers:` 줄(두 규칙의 상태, 전제, 덮지 않는 것); writer 0 은 판정이 아니고 `writer_set_incomplete(zero writers under the rules)`. (4) lookup 과 행이 이름 댄 생성자의 out 포인터 인자(`id holder:`)와 키의 포인터 필드(`aliases:`). (5) 행이 이름 댄 생성자의 callee 가 돌려줄 수 있는 상수(`callee-check: <callee> returns …`; 결과 문서 `plan/observation_path_results_20261116.md` §7.11.7 의 열린 점 (b): 경로 A 행에 생성자 callee 의 반환 상수가 없어 생성자 out 을 성공 상수 아래에서 읽을 수 없었다). (6) 생성자가 없는 lookup 의 dependency 칸에 까닭(다른 partition 의 생성자만, 키 매개변수 없이 쓰이는 키, writer 집합이 완전하지 않음, 풀리지 않은 간접 호출이 닿을 수 있는 키 쓰기, writer 없음, 소속이 풀리지 않음).
- **1차 근거.** 개정 6 의 A2 검색은 비교 함수에 닿는 간접 호출을 가진 모든 함수를 그 이름 공간의 검색으로 본다(`kairos/lib/llvm/relations.cpp` 의 `searches_of`, 이 빌드 전 판): 넘긴 callback 을 보지 않는다. C1 의 OSAL(묶음 1df262a): 하나의 검색 루프 `OS_ObjectIdFindNextMatch`(`osal/src/os/shared/src/osapi-idmap.c:535-570`)가 callback `MatchFunc` 로 비교한다; 이름 조회는 `OS_ObjectNameMatch` 를 넘기고(`osapi-idmap.c:950`, 생성의 중복 검사도 `:1170`), 경로 변환은 `OS_FileSys_FindVirtMountPoint` 를 넘긴다(`osapi-filesys.c:727`). 표는 `token->obj_type` 이 고른다(`osapi-idmap.c:546-547`, 저장 `:320`); 그 옆의 잠금 방식은 `token->lock_mode` 로 저장되고(`:319`) 검색 루프가 읽지 않는다. CALL_TARGETS 의 `via` 는 함수 값이 지나온 흐름 노드를 적고, 매개변수 노드는 `P:<함수 키>#<번호>` 다(`kairos/lib/llvm/ast_fn.cpp:1399`).
- **코드**(`kairos/`): `lib/llvm/relations.cpp`(개정 문자열 "relations rules revision 7 (2026-11-16)"; `Search` 의 callback 매개변수와 `callback_param`; `membership_states`, `use_membership`; `selects_table`, `in_exit`, `exit_reads_local`, 새 `partition`; `writer_premises`, `flags_text`, `id_holders`, `callee_returns`, `no_creator_reasons`; `KeyValue` 의 `image_field`, `alias`; 경로 A 의 사용 수집과 행; B·C 행의 `writers:` 줄; 머리의 `summary A membership and creators` 줄; 사유 표에 셋: `writer_set_incomplete(zero writers under the rules)`, `writer_set_incomplete(key writes reachable through unresolved indirect calls)`, `link_unlinked(namespace membership unresolved)`), `lib/llvm/relations.hpp`(머리 주석). `tests/static/fixtures/relcallback.c`(새, 사례 이름 없음), `tests/static/static_golden.cpp`(사례 하나), `tests/static/CMakeLists.txt`(fixture·golden 목록, 머리 주석), `tests/static/test_static_cli.cpp`(개정 문자열 확인 6 → 7, 사용 지점 키 블록의 종류 인자가 표를 고르게 함, 새 블록 하나). `docs/static_facts.md` §5o(새)와 §5e 의 가리킴 한 문장.
  - 판정 규칙 3.5 의 어휘(바탕 사유)는 바꾸지 않는다: 새 셋은 기존 바탕 사유의 하위 사유다(3.5 의 하위 사유 규칙). 어휘 자료 `tests/static/data/reason_vocabulary.txt` 와 `test_static_reasons` 의 줄 수 점검(22)은 그대로다.
  - 하지 않는 것: facts schema(`include/`), 추출(`ast_*`, `argflow`, `finalize`, `serialize`, `lockset`, `target_rules`, `rel_facts`), 계획 생성기(`plan_gen.*`, `cmd_plan.cpp`), `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `instrument/`, `tests/lint/`, 2a 드라이버와 그 허용표, 설명 스크립트(`env/obs1/explain.py`).
- **도구 해시와 정체성**(`cmake/KzSourceIdentity.cmake` 의 규칙):
  - kz-static: 파일 sha256, `--identity` 의 `tool_sha256`, `tool_source` 가 바뀐다(`lib/llvm` 이 바뀜). `core_source` `f6e3c7b4…` 는 그대로다 [I].
  - 다시 링크되지 않아야 하는 것(빌드 전 sha256 은 `pre_mc.txt`): kz `931dd39c…`, kz-dump `05623054…`, kz-rt-dump `4390d4f1…`, kz-phi `7d34e6e2…`, kz-instr-clang `28db4017…`, kz_lint `5f62ec20…`. kz, kz-phi, kz-dump 의 정체성 정의는 그대로다. kzrt-manifest 는 다시 컴파일되지 않는다(`runtime/` 그대로).
  - 다시 링크되는 시험 프로그램: `kz-llvm` 을 링크하는 여덟, `test_static_reasons`, `test_static_plan`, `static_golden`(소스), `test_static_cli`(소스).
  - `CMakeCache.txt` `bc2f43b4…` 는 그대로다 [I]. flags.make 목록의 해시는 바뀐다(kz-static 의 `flags.make` 에 든 도구 소스 정의).
- **도구 보존.** 이 빌드는 kz-static `de1315b3` 를 제자리에서 덮는다. 그 압축 사본(`/home/user/work/preserve/tools-20261116-c/kz-static.de1315b3.xz`, 계획 생성기 빌드 뒤에 만듦)을 빌드 전에 다시 확인했다: `xz -t` OK, 스트림 sha256 = `de1315b3…`, 제자리 파일과 `cmp` 같음(`tool_copy/ROUNDTRIP.txt`). 새 도구는 `de1315b3` 의 남은 출력(계획 다섯, relations 다섯, c1_pre_tx 다시 뽑기)을 바이트 동일하게 다시 만들지 못한다(규칙이 바뀐다). 그래서 `de1315b3` 은 "대체됨"이 아니라 사본으로 남고, 그 출력도 남긴다. 등록부: `de1315b3` 행 in-place+copy → copy, 새 도구 행 in-place.
- **golden**:
  - 바뀌는 여섯(relations 를 그리는 것 전부): `relations`, `relations_np`, `relpaths_np`, `relsound_np`, `relincomplete_np`, `relincomplete_cut_np`. 바뀌는 것은 relations 부분뿐이고, 아래 "relations 다섯 빌드" 의 갈래 안이다(`compare_declared.py --golden`: facts 부분 바이트 동일, 행은 번호로 맞춤; fixture 에서 빠지는 행은 없다 [I]).
  - `plangen_np.plan.golden`: 두 줄(manifest 머리와 sidecar 의 `rules` 줄)의 "relations rules revision 6" → "7" 만. 그 두 줄을 되돌린 글은 옛 golden 과 바이트 동일해야 한다.
  - 새 하나: `relcallback_np.golden`(`KZ_GOLDEN_UPDATE=1` 로 만들고 읽어 본다).
  - 그 밖의 golden(`tests/static/data` 의 나머지, `tests/golden/data`, `tests/phi/data`)은 바뀌지 않는다.
- **relations 다섯 빌드**(`rerun.sh`, 새 도구, T8CF 의 NP facts 그대로): 앞 도구 `de1315b3` 의 출력(`/home/user/work/redesign/obs1/plangen_relcheck/<b>/`)과 `compare_declared.py`(빌드 전에 이 선언과 함께 커밋)로 사용 지점 키마다 맞춘다. 허용하는 갈래(스크립트 머리에 같은 글):
  - 머리: `rules` 줄 6 → 7; `summary A` 줄과 `summary incomplete` 줄의 수; `summary A` 다음의 새 줄 `summary A membership and creators (relations rules revision 7): …` 하나. 그 밖의 머리 줄은 같다.
  - `[A]` 머리 줄은 행 수만; `[B]`, `[C]` 머리 줄은 같다. 이름 공간 블록은 `uses:` 줄(수와 새 꼬리 `; left out, another callback …`)과 새 `left out:` 줄 밖에서 같다.
  - 행 집합: B·C 의 키는 모두 양쪽에 있다. A 의 키는 새 글에서 빠질 수 있으나, 같은 이름 공간 블록에 같은 호출·호출자·callee#매개변수의 `left out:` 줄이 있을 때만이다. 새 키는 없다. 남는 행의 키와 키 글은 같다(키는 생성자·partition·계수를 담지 않음, §5l).
  - B·C 행: 새 `writers:` 줄 하나 밖에서 같다. `incomplete:` 는 `writer_set_incomplete(zero writers under the rules)` 를 더할 수 있다. `layers:` 는 새 줄이 이름 대는 파일의 `basis-deviation(...)` 항목을 더할 수 있고, 그때 `reading_options_differ(basis deviation of a unit of the row)` 가 더해질 수 있다.
  - 남는 A 행: 머리는 행 번호와 partition 만 다를 수 있다. 새 줄 `membership:`, `aliases:`, `id holder:`, `callee-check:`, `callee-check (returns not followed):`, `writers:`. `partition:` 줄은 바뀔 수 있다. `key:`, `actors:`, `handling:`, `run-order:`, `violation:` 는 같다. `creator:`/`not a pair:` 줄은 빠지거나(생성자가 빠짐, 다른 partition) 들어올 수 있고(같은 partition 이 됨), 생성자 줄 끝에 `; namespace membership unresolved: …` 가 붙을 수 있다. 생성자 집합이 같으면 dependency 는 옛 글이거나 옛 글 + `: <까닭>` 이고 feasibility·flip 은 같다. 집합이 다르면("creator set changed") 그 셋과 `layers:` 의 `P0-unverified(...)` 항목이 새 집합을 따른다. `incomplete:` 는 새 사유 셋과 `reading_options_differ(...)` 를 더할 수 있고, 생성자 집합이 달라진 행에서만 `key_unresolved(creator key)`, `truncated(static image)`, `fixed_proof_open(no static order between the actors)` 를 얻거나 잃을 수 있다.
  - `keys.txt`: 빠진 행의 줄이 없고, 번호와 머리 수(`rows …`)와 셋째 줄(규칙 개정)이 바뀐다. 남는 행마다 키와 키 글이 같다(위 스크립트가 본다). `relations.log` 는 빈 파일이다.
  - 빠진 (사용, 이름 공간) 짝마다 소스 대조(`callback_check.py`): `left out:` 줄이 이름 댄 callback 이, 줄이 가리키는 호출의 그 번째 인자로 소스 파일에 그대로 있고(묶음 루트 기준; 캐스트·괄호 허용), 이름 공간의 비교 함수와 다르다. 다섯 빌드 모두 어긋남 0 이어야 한다.
  - 기대(측정 전 [I]; 빌드 전에 scratchpad 에서 이 소스를 MC 의 `libkz-llvm.a` 사본에 넣어 링크한 점검 프로그램으로 잰 값이고, 근거가 아니다. 빌드 뒤의 도구로 다시 잰다): A 행 127/127/68/66/66 → 63/63/25/66/66(빠짐 64/64/43/0/0, `left out:` 줄 98/98/64/0/0, 소스 대조 모두 맞음); 남는 A 행 가운데 partition 이 바뀐 것 38/38/22/26/26, 생성자 집합이 바뀐 것 32/32/19/23/23; B·C 행은 다섯 빌드 모두 그대로. C1 관계 1 의 행(키 `17975444d83f6589`)은 남고 partition {3}, 생성자 둘(BP, SCH_LAB), 생성자 callee `OS_CountSemCreate` 의 상수 가운데 실패 등급이 아닌 것은 `0 [OS_SUCCESS]` 하나 [I]. 앞의 계획에 든 경로 A 행 A22, A23(name_entry 의 `OS_remove`)은 빠지고, A50, A51(같은 호출의 virtual_mountpt 행)은 생성자 없음이 된다 [I].
- **다시 뽑기 다섯 빌드**(`extract.sh`: `env/redesign/one_build.sh` 를 빌드마다, 새 디렉터리 `/home/user/work/redesign/obs1/relrev7_extract/<b>/`): `env/redesign/compare_build.py` 로 T8CF `<b>/NP` 와 파일마다 비교, 허용 파일 `rev7_extract_allow.tsv`(이 선언과 함께 커밋): `basis.kz`, `facts.kz`(컨테이너 머리의 도구 해시와 거기서 따라 바뀌는 머리 칸; 섹션 레코드는 BASIS 의 `extract.units_from`(basis artifact 해시를 담음) 밖에서 모두 같아야 한다), `basis.txt`(첫 줄), `extract.txt`(첫 줄), `relations.txt`, `sha256.txt`, 한쪽에만 있는 `facts.txt`·`status.txt`·`space_guard.log`. 그 `relations.txt` 와 `keys` 는 위 `rerun.sh` 의 것과 둘째 줄(facts 해시) 밖에서 바이트 동일해야 한다. 추출 코드는 바뀌지 않으므로 [I] facts 의 레코드는 같다.
  - 2026-11-16 검토 뒤 정정: §6.7 `builds-4`. 이 키 비교는 §2.7 에서 덧붙임 없이 빠졌다. §6.0 (가) 1 에서 이 단계의 도구 `038cd9e3` 을 보존 사본에서 풀어 다시 뽑은 facts 로 키를 냈고, 다섯 빌드 모두 `keys.txt` 가 `relrev7_check/<b>/keys.txt` 와 둘째 줄 밖에서 바이트 동일했다(`records/review-fixes-20261116/keys_check/keys_check.txt`, 26 OK).
- **계획 다섯 빌드**(`plans.sh`, 새 도구, 앞 계획과 같은 인자: `records/obs1-20261116-plangen/plans.sh`): 다섯 모두 rc 0; manifest 가 런타임의 호스트 파서로 읽힘(scratchpad 의 작은 점검 프로그램, `libkzrt-manifest.a` 링크); c1_pre_tx 와 c5_641 의 재생성 점검(`env/tools/plan_regen.sh`, 같은 도구) PASS. 앞 계획(`/home/user/work/redesign/obs1/plangen/<b>/`)과의 차이를 `plan_diff.py`(이 선언과 함께 커밋)로 잰다: 계획된 행, 층과 처분이 바뀐 행, 규칙별 site, need·gap. 측정이고 판정이 아니다. 계획 생성기의 코드는 바뀌지 않으므로 차이는 모두 relations 행의 차이에서 온다 [I].
- **시험**: 전체 ctest 246 → 248(새: `static_fixture_compile_relcallback`, `static_golden_relcallback_np`). 라벨: static +2, golden +1. golden 갱신 전 첫 실행에서 실패는 여덟이어야 한다: 바뀌는 golden 여섯, golden 이 아직 없는 `static_golden_relcallback_np`, `test_static_plan`(계획 golden 두 줄). `test_static_cli`(확인 864 개 [I: 위 점검 프로그램과 같은 방법으로 scratchpad 에서 컴파일해 잼]), `test_static_reasons` 는 처음부터 통과한다. 갱신 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). lint 7/7(빌드 전 MC `kz_lint` `5f62ec20` 으로 이 트리에서 7/7 [M]). 단위 점검 `env/tools/tests/run_unit.sh` 15/15, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤 `env/tools/ledger_check.sh` 전체(빌드 전 45 행, 실패 0 [M])와 `env/tools/tools_check.sh --deep`(빌드 전 31 행, 실패 0 [M]). 모두 같아야 한다. 새 출력은 장부 행으로 더한다.
- **공간**: 시작 전 여유 ≥ 하한 314,572,800 + 필요량. 필요량 = 앞 kz-static 빌드의 dip 219,463,680(`records/obs1-20261116-plangen/commands.txt` §4) + 출력 60 MB(relations 약 6 MB, 다시 뽑기 다섯 약 45 MB, 계획 약 1 MB, 여유) + 한 시간 흐름 13 MB = 292,463,680 B(감시 문턱 607,036,480). 감시(`env/tools/space_guard.sh --kind kz-static`, 2초마다)가 하한을 넘으면 멈추고 보고한다. 재실행·다시 뽑기·계획도 감시 아래 돌린다.
- **비교 스크립트**(빌드 전에 이 선언과 함께 커밋, 모두 `python3 -I`): `compare_declared.py`, `callback_check.py`, `plan_diff.py`; 실행 스크립트 `rerun.sh`, `extract.sh`, `plans.sh`; 허용 파일 `rev7_extract_allow.tsv`.

### 2.1 바뀐 것 [F]

- 커밋 `d778b7f`(선언, 코드, fixture, 시험, 문서, 비교 스크립트를 함께, 빌드 전). 규모: `lib/llvm/relations.cpp` +552/−16 남짓, `relations.hpp` +6, `tests/static/test_static_cli.cpp` +71, `tests/static/fixtures/relcallback.c`(새, 175줄), `docs/static_facts.md` §5o.
- 빌드 뒤: golden 일곱 갱신(relations 여섯, 계획 하나), 새 golden `relcallback_np.golden`. 이 절과 기록.

### 2.2 빌드 [M]

- 빌드 전(`pre_mc.txt`, 10:28:59Z): 도구 일곱이 계획 생성기 빌드의 `post_mc.txt` 와 같다. HEAD `d778b7f`, `kairos` 미커밋 0줄.
- MC 제자리 `make -j4 all`(10:29:05Z–10:31:41Z), 공간 감시 아래. rc 0, 첫 번에 됐다. 링크 14줄: `libkz-llvm.a`, kz-static, `kz-llvm` 을 링크하는 시험 여덟, `test_static_reasons`, `test_static_plan`, `static_golden`, `test_static_cli`. 빌드 시작 뒤 정체성 경로의 파일은 바뀌지 않았다(`find -newer` 0).

| 도구 | 전 | 뒤 |
|---|---|---|
| kz-static | `de1315b359af279c46585d80e8a71db6143a19ded1ca164e4b88bc7106a3abdc` | `038cd9e36af4d4ded9f8801346405f0ceaa38025a2d603ac35974ea48097d0ce` (184,095,248 → 185,172,504 B) |
| kz-static tool_sha256 | `c3677e76…` | `a3a6cb31da61e6edce2193d02700d95dad3f702da921ca46fcf3ef3be8fe4f6c` |
| kz-static tool_source | `5640d5d6…` | `e8778114cc825ea7328f25a423df9540707dafdabf7f0590fb47c14c1fdce691` |
| core_source | `f6e3c7b4…` | 같음 |
| kz / kz-dump / kz-rt-dump / kz-phi / kz_lint / kz-instr-clang | `931dd39c` / `05623054` / `4390d4f1` / `7d34e6e2` / `5f62ec20` / `28db4017` | 모두 같음(다시 링크되지 않음) |
| kz, kz-phi, kz-dump 의 정체성 정의 | | 같음 |
| `CMakeCache.txt` | `bc2f43b4…` | 같음 |
| flags.make 목록 해시 | `c92ab3c1…` | `7d2a0717…` (kz-static 의 도구 소스 정의) |

- 선언대로다.

### 2.3 시험 [M]

- golden 갱신 전 전체 ctest(`ctest_first.log`): 248 개 가운데 실패 8, 선언한 여덟 그대로다: `static_golden_{relations,relations_np,relpaths_np,relsound_np,relincomplete_np,relincomplete_cut_np}`, golden 이 없던 `static_golden_relcallback_np`, `test_static_plan`. `test_static_cli`, `test_static_reasons` 는 이때 이미 통과했다.
- `KZ_GOLDEN_UPDATE=1` 로 여덟을 쓰고(`golden_update.log`) 비교했다(`golden_compare.txt`): relations golden 여섯 모두 `compare_declared.py --golden` 에서 선언한 갈래 안(facts 부분 바이트 동일, 빠진 행 0, partition·생성자 집합이 바뀐 행 0); 계획 golden 은 두 줄(manifest 머리, sidecar `rules`)의 "revision 6 → 7" 뿐이고 그 둘을 되돌리면 바이트 동일. 새 `relcallback_np.golden`(377 줄)을 읽었다: 이름 공간 둘(`Rec.name`, `Rec.path`), 행 넷, `left out:` 셋(경로 lookup 이 이름 공간에서, 이름 lookup 과 생성이 경로 이름 공간에서), 소속이 풀리지 않은 행 둘(본문 없는 함수에서 온 비교), partition `{1}`/`{0}` 과 그 옆의 `not a table selector: 2 [LOCK_SHARED]`.
- 그 밖의 golden(`tests/static/data` 의 나머지, `tests/golden/data`, `tests/phi/data`)은 바뀌지 않았다(`git status`).
- 갱신 뒤 전체 ctest(`ctest_full.log`): 248 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음; `skip_reason.log`). 라벨: golden 61, static 53, legacy-case 51, unit 84, runtime 66, lint 7(앞 단계에서 golden +1, static +2).
- `test_static_cli` 확인 864 개(앞 837, +27: 새 블록), `test_static_reasons` 655 개, `test_static_plan` 182 개, `lint_selftest` 109 경우 실패 0, lint 7/7.
- 2026-11-16 검토 뒤 정정: §6.7 `builds-8`. 위의 확인 수 864, 655, 182, 109 는 콘솔에서 읽어 `commands.txt` 에만 적었고 도구 출력으로 남기지 않았다. 그 시험 프로그램은 뒤 빌드가 덮어 다시 잴 수 없다.
- 단위 점검: `env/tools/tests/run_unit.sh` 15/15, `env/redesign/step2a_tests_unit.sh` 7/7.

### 2.4 바이트 동일성 [M]

- **relations 다섯 빌드**(10:33Z, 새 디렉터리 `/home/user/work/redesign/obs1/relrev7_check`): `compare_declared.py` 가 다섯 모두 rc 0, 모든 차이가 선언한 갈래 안이다(`byte_identity.txt`, `compare/report_<b>.txt`). 남는 행의 키와 키 글은 모두 같다. 둘째 줄 `facts` 는 같고 `relations.log` 는 빈 파일이다. 빠진 (사용, 이름 공간) 짝의 소스 대조(`callback_check.py`, `compare/callback_<b>.tsv`): 98/98/64/0/0 짝, 모두 MATCH(넘긴 callback 이 소스의 그 호출의 그 번째 인자이고 이름 공간의 비교 함수와 다르다).

| 빌드 | A 행 전 → 뒤 (빠짐) | `left out:` 줄 | 소스 대조 | partition 이 바뀐 남는 A 행 | 생성자 집합이 바뀐 남는 A 행 | B·C 행 | relations.txt |
|---|---|---|---|---|---|---|---|
| c1_pre_tx | 127 → 63 (64) | 98 | 98/98 | 38 | 32 | 304 그대로 | `63176ff6…`, 7254 줄 |
| c1_post_tx | 127 → 63 (64) | 98 | 98/98 | 38 | 32 | 304 그대로 | `99d9acf1…`, 7254 줄 |
| r2_core | 68 → 25 (43) | 64 | 64/64 | 22 | 19 | 270 그대로 | `0186b38e…`, 5112 줄 |
| c5_641 | 66 → 66 (0) | 0 | — | 26 | 23 | 112 그대로 | `d8eccd78…`, 3643 줄 |
| c5_642 | 66 → 66 (0) | 0 | — | 26 | 23 | 109 그대로 | `c9f03cc4…`, 3591 줄 |

- 선언한 갈래 안의 주석 두 종류: c1 의 두 행(옛 번호 A26, A30)은 생성자 집합이 바뀌어 `layers:` 의 `P0-unverified(...)` 에서 `dlsym` 이 빠졌다(시작 스크립트 행위자의 생성자가 빠짐); c5 의 13 행은 새 줄이 이름 대는 파일(`psp/fsw/pc-linux/src/cfe_psp_start.c`)의 basis 편차 항목을 `layers:` 에 더했다. 둘 다 선언한 갈래다.
- **다시 뽑기 다섯 빌드**(10:34Z–10:35Z, `/home/user/work/redesign/obs1/relrev7_extract/<b>/`): 다섯 모두 PASS(`extract_run.txt`). `compare_build.py` 와 허용 파일로 T8CF `<b>/NP` 와: 다섯 모두 허용 밖 0(`extract_compare.txt`, `compare/extract_<b>.tsv`). `facts.kz` 는 머리의 도구 해시·basis 해시·부모와 STRINGS crc32, BASIS 레코드 `extract.units_from` 만 다르고, 다른 섹션 레코드는 모두 같다. 그 `relations.txt` 는 위 재실행의 것과 둘째 줄 밖에서 바이트 동일하다. 추출이 바뀌지 않았음이 다섯 빌드에서 확인되었다.
- 2026-11-16 검토 뒤 정정: §6.7 `builds-4`. 선언(§2.0)의 다시 뽑기의 “keys” 비교는 이 단계에서 하지 않았다(§2.7). §6.0 (가) 1 로 했고 다섯 빌드 모두 선언대로였다(§2.0 의 정정 줄).
- **누적 목록**: `ledger_check.sh` 48 행, 실패 0(새 행 셋: `OBS_relrev7_check`, `OBS_relrev7_extract`, `OBS_relrev7_plans`). `tools_check.sh --deep` 32 행, 실패 0. 바깥 이름 점검(`--tree`)은 세 출력 디렉터리에서 깨끗하다.
- **도구 보존**: 등록부에서 `de1315b3` 은 in-place+copy → copy(사본은 빌드 전 왕복 확인; 남은 출력은 새 도구가 다시 만들지 못하므로 "대체됨"이 아니고, 그 출력은 남김). `038cd9e3` 은 in-place 행으로 더했다. 새 도구의 사본은 두지 않았다: 이 단계의 계획은 부팅에 닿지 않는다. 다음 제자리 빌드 전에 사본을 만든다(PI 의 답).

### 2.5 측정 [M]

**경로 A 의 이름 공간마다 행 수**(전 → 뒤; `compare/table_<b>.tsv` 가 키마다의 전후 표):

| 이름 공간 | c1_pre_tx | r2_core |
|---|---|---|
| `OS_filesys_internal_record_t.virtual_mountpt` | 90 → 35 | 55 → 19 |
| `OS_common_record_t.name_entry` | 15 → 7 | 10 → 4 |
| `OS_impl_task_internal_record_t.id` | 2 → 1 | 2 → 1 |
| `BP_FlowTblEntry_t.Name`, `CFE_TBL_RegistryRec_t.Name`, `CFE_TBL_CmdHandlerTblRec_t.CmdCode`, `OS_stream_internal_record_t.stream_name` | 9, 9, 1, 1 그대로 | (없음), (없음), 1 그대로, (없음) |

- 빠진 행은 모두 A2 검색(OSAL 의 `OS_ObjectIdFindNextMatch`)의 것이다. 넘긴 callback(c1_pre_tx, 98 짝): `OS_ObjectNameMatch` 를 `osapi-idmap.c:1170:23`(생성의 중복 검사)에서 54, `:950:12`(이름 조회)에서 7; `OS_FileSys_FindVirtMountPoint` 를 `osapi-filesys.c:727:19`(경로 변환)에서 33, `:491:19`, `:549:19` 에서 1 씩; `OS_TaskIdMatchSystemData_Impl` 을 `osapi-task.c:440:19` 에서 2. 곧 이름 조회·생성은 경로 이름 공간에서, 경로를 쓰는 파일 연산(`OS_remove`, `OS_stat`, `OS_unmount`, `OS_FileSysStatVolume` 등)은 이름 이름 공간에서 빠졌다. 소속이 풀리지 않은 행은 c1·r2 에 하나씩: `OS_ObjectIdGetBySearch` 안의 `OS_ObjectIdFindNextMatch` 호출(`osapi-idmap.c:917:19`, callback 이 그 함수의 매개변수라 사용 밖을 따라가지 않음). c5(OSAL 4.1)에는 A2 검색이 없어 행이 빠지지 않았다.
- **partition**: c1 의 이름 조회 행은 `{3}`(`LOCAL_OBJID_TYPE`; `osapi-idmap.c:320:5` 에서 `OS_object_token_t.obj_type` 에 저장되고 `:546:69` 에서 첨자로 읽힘). 경로 이름 공간의 `OS_remove` 행(옛 A22 의 같은 호출, 새 A31)은 `{1,11}` → `{11}` 이고 `1 [OS_LOCK_MODE_GLOBAL]` 은 `not a table selector`(계획 문서의 예상 "A22 partition {1,11} → {11}" 은 그 행이 name_entry 에서 빠지고 virtual_mountpt 쪽 행에서 맞았다). c5 의 `CFE_SB_PipeD_t.PipeId` 이름 공간 26 행은 `{4,0}`/`{0}`/`{1}` → `{}`: 메시지 한도(`CFE_SB_DEFAULT_MSG_LIMIT`)와 범위(`CFE_SB_GLOBAL`, `CFE_SB_LOCAL`)는 표를 고르지 않는다. 그래서 partition 으로 갈리던 생성자가 짝이 되어 23 행의 생성자 집합이 바뀌었다.
- **새 사유의 행 수**(c1_pre_tx / c1_post_tx / r2_core / c5_641 / c5_642): `writer_set_incomplete(zero writers under the rules)` 3/3/1/2/2, `writer_set_incomplete(key writes reachable through unresolved indirect calls)` 5/5/4/5/5, `link_unlinked(namespace membership unresolved)` 1/1/1/0/0. 생성자가 없는 lookup 행의 까닭(c1_pre_tx, 59 행, 한 행에 여럿): 키 매개변수 없이 쓰이는 키 54, writer 집합이 완전하지 않음 54, 풀리지 않은 간접 호출이 닿을 수 있음 5, 다른 partition 의 생성자만 4, writer 없음 1, 소속이 풀리지 않음 1.
- **C1 관계 1 의 행**(키 `17975444d83f6589`, 새 번호 A21 그대로; `c1_rows.txt`): partition `{3}`, `membership:` `OS_ObjectNameMatch` 를 `osapi-idmap.c:950:12` 에서 넘김. 생성자 둘(BP `bp_flow.c:132:18`, SCH_LAB `sch_lab_app.c:168:16`)은 그대로이고 dependency·feasibility·flip 은 같다. 새 줄: lookup 의 out 포인터 `arg 0 [&CF_AppData.engine.channels[i].sem_id]`(저장 `OS_ObjectIdFindByName` `osapi-idmap.c:978:9`), BP 생성자의 `arg 0 [&BP_GlobalData.Throttles[index].Sem]`, SCH_LAB 의 `arg 0 [&SCH_LAB_Global.TimingSem]`(저장 `OS_ObjectIdFinalizeNew` `:860:9`), 생성자 callee `OS_CountSemCreate` 의 상수 줄(값·이름이 다른 것 10, 그 가운데 실패 등급이 아닌 것은 `0 [OS_SUCCESS]` 하나; 따라가지 못한 반환 둘: `OS_NotifyEvent` 의 간접 호출 결과, `OS_ObjectIdFinalizeNew` 의 매개변수 반환), 키 필드·활성 필드·BP 키 이미지 필드(`BP_Throttle_t.Name`, writer 없음)의 `writers:` 줄. 설명 스크립트의 규칙(`env/obs1/explain.py:286` 의 상수 꼴, 실패 등급)을 이 줄에 그대로 적용하면 생성자의 성공 상수는 `0 [OS_SUCCESS]` 하나로 선다(`creator_success.txt`). 그래서 결과 문서 §7.11.7 의 열린 점 (b)(생성자 상수가 행에 없음)는 relations 쪽에서 채워졌다. 설명 기록은 다시 내지 않았다(부팅 없음; 스크립트가 읽는 것은 다음 단계에서 확인).
- **BP 의 존재 탐침 행 A20**(키 `35f07539e7606e2e`): 같은 행위자 대조(판정 규칙 개정 2 §8.6)에 쓸 줄이 생겼다: lookup 과 생성자의 out 포인터가 같은 `&BP_GlobalData.Throttles[index].Sem`, 생성자 callee 의 성공 상수 `0 [OS_SUCCESS]` 하나. 행은 여전히 층 3 이라 이 계획에는 들지 않는다.
- 2026-11-16 검토 뒤 정정: §6.7 `rules-01`, `rules-03`. 위 두 항목의 생성자 성공 상수 `0 [OS_SUCCESS]` 는 서지 않는다. `OS_CountSemCreate` 의 `0 [OS_SUCCESS]` 줄 넷(`relrev7_check/c1_pre_tx/relations.txt` 의 A20 519, 522, 523, 527 줄, A21 560, 563, 564, 568 줄)은 모두 `OS_ObjectIdAllocateNew` 사슬이고, 그 결과는 성공이면 `osapi-countsem.c:103` 과 `:106` 에서 다시 정의되어 `:109` 의 반환에 실패 값만 닿는다(`:95`; 묶음 1df262a). 그 줄은 흐름을 보지 않는 도달 정의에서 나왔다. 실제 성공 반환은 `OS_ObjectIdFinalizeNew` 의 매개변수 반환(`osapi-idmap.c:863`)이고, 같은 행이 그것을 `returns not followed` 로 적었다. `creator_success.txt` 의 `(0, OS_SUCCESS)` 도 그 줄에서 나왔다. relations 규칙 개정 9 는 그런 줄에 `not shown to reach the return` 을 달고 그 생성자 callee 를 `callee-check (set): … not established` 로 적으며, 설명 스크립트 개정 3 으로 다시 재면 A20·A21 모두 성공 상수가 없다(§6.5). 또 A20 은 생성자를 남기지 않아(`no creator left after exclusions`) 판정 규칙 8.2 의 범위로는 같은 행위자 대조의 쌍이 생기지 않는다. 그 범위는 8.12 (ㄴ)에 정했다.

**c1_pre_tx 계획에서 바뀐 것**(`plans/plan_diff_c1_pre_tx.txt`; 앞 계획 `/home/user/work/redesign/obs1/plangen/c1_pre_tx`, 같은 인자):

| | 앞 (`de1315b3`) | 뒤 (`038cd9e3`) |
|---|---|---|
| 행 | 431 | 367 (빠진 키 64, 새 키 0) |
| 층 1 / 2 / 3 | 12 / 306 / 113 | 8 / 278 / 81 |
| 계획된 행 | A21 A22 A23 A50 A51, B13 B73 B81 B99 B113 B118 B119 | A21, 같은 B 일곱 |
| site | 301 | 259 (빠진 42, 새 0; 남는 259 의 키는 모두 앞 계획에 있음) |
| 규칙별 site | creator-call 40, key-text 4, use-call 193 | creator-call 4, key-text 2, use-call 189 (그 밖의 규칙은 같음) |
| gap `record_insufficient(key text)` | 10 | 0 |
| need: event-send / lock-result / wait-result / budget | 5 / 12 / 12 / 419 | 1 / 8 / 8 / 359 |

- 빠진 계획 행: A22(`8d4cf478…`), A23(`07c71974…`)은 name_entry 의 `OS_remove` 호출(`cf_cfdp.c:1600:17`, `:1604:17`)이고 행이 없어졌다(경로 변환의 callback 을 넘김). A50(`5a9acfc1…`), A51(`ad0b0b74…`)은 같은 두 호출의 virtual_mountpt 행이고 남았으나(새 A31, A32) 생성자가 빠져 처분 D07 → D06("no creator in another actor"), 층 1 → 3. 그 넷이 부르던 생성자 site 36 개(`OS_OpenCreate`, `OS_ModuleLoad`, `OS_mount`, `OS_mv`, `CF_WrappedOpenCreate`), key-text 2, use-call 4 가 빠졌다. 관계 1 의 행(A21)의 site 14 개는 키가 모두 같다(번호만 당겨짐).
- 그 밖의 남는 행: 25 행이 생성자를 잃어 순서 등급 RELOC → NOPAIR(층 2 → 3 이 23, 1 → 3 이 2).
- 다른 빌드의 계획(`plans/plan_diff_<b>.txt`): c1_post_tx 계획된 A 행 넷(A22 A23 A50 A51) → 0, site 287 → 245; r2_core 계획된 행 그대로(B 셋), site 42 그대로, 14 행 RELOC → NOPAIR; c5_641·c5_642 계획된 행과 site 그대로, 13 행 NOPAIR → RELOC(층 3 → 2; partition 이 없어져 짝이 생김).
- 다섯 계획 모두 rc 0, 런타임의 호스트 파서로 읽힘(`parse_check.txt`: site 259/245/42/147/139), c1_pre_tx·c5_641 재생성 점검 PASS(`regen_c1_pre_tx.txt`, `regen_c5_641.txt`).

### 2.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 이 단계 처음 | 3,701,456,896 | 시작 |
| 10:29:05 | 3,255,271,424 | 빌드 시작(감시 문턱 607,036,480; 그 사이 scratchpad 점검 프로그램 약 0.74 GB) |
| 10:31:41 | 3,245,293,568 | 빌드 끝(가장 낮은 3,211,595,776, dip 43,675,648) |
| 10:33:51 | 3,236,028,416 | relations 다섯(dip 7,610,368) |
| 10:35:34 | 3,188,674,560 | 다시 뽑기 다섯(dip 47,054,848) |
| 10:36:03 | 3,187,101,696 | 계획 다섯(dip 987,136) |
| 10:39:09 | 3,629,264,896 | scratchpad 의 이 단계 작업 파일을 지운 뒤 |

- 하한을 넘지 않았다. 빌드 dip 43.7 MB 는 선언한 필요량보다 작다(새 시험 프로그램이 없음). 남긴 출력: relations 7,564,932 B, 다시 뽑기 46,861,749 B, 계획 886,441 B. 지운 것: scratchpad 의 이 단계 작업 파일(점검 프로그램, 그 출력, golden 사본)뿐이다.

### 2.7 계획과 다르게 한 것

- **O4 의 const 한정 예외를 두지 않았다.** 설계 연구의 규칙은 const 이고 cast·union hit 이 없는 필드에는 writer 0 사유를 붙이지 않는 것이었다. facts 에 필드 타입(const 한정)이 없다(필드 타입 표는 뒤의 schema 개정). 그래서 writer 0 이면 언제나 사유를 단다(보수 쪽).
- **"생성자가 없는 까닭"에 "다른 partition 의 생성자만" 을 더했다.** 설계 연구의 다섯((a)–(e)) 가운데 가장 흔한 실제 까닭이 그것이고(c1 4 행), 다섯의 어느 것에도 들지 않는다. "closure 밖 단위" 는 facts 에 closure 밖 목록이 없어(basis 에만 있음) `writers:` 줄에 "읽지 않음" 으로만 적었다.
- **생성자 callee 의 상수는 RET_CONSTS 와 호출 결과를 돌려주는 반환을 따라간 집합이다.** `OS_CountSemCreate` 의 성공 반환은 지역 변수를 거쳐 `OS_ObjectIdFinalizeNew` 의 매개변수 반환으로 온다(`osapi-countsem.c:106`, `osapi-idmap.c:863`); facts 는 그 지역의 정의를 잇지 않는다. 그래서 `0 [OS_SUCCESS]` 는 같은 함수가 부르는 `OS_ObjectIdAllocateNew` 쪽 사슬에서 온 것이고, 그 줄의 뜻은 "돌려줄 수 있는 상수" 다(증명 아님). 따라가지 못한 반환은 `callee-check (returns not followed):` 줄에 적었다. 설명 스크립트의 성공 상수 규칙(실패 등급이 아닌 상수 하나)은 이 집합에서 선다.
- 2026-11-16 검토 뒤 정정: §6.7 `rules-01`. 위 항목의 “돌려줄 수 있는 상수”는 틀렸다: `0 [OS_SUCCESS]` 는 `OS_CountSemCreate` 가 돌려줄 수 없는 줄이다(성공 갈래에서 다시 정의된 값; §2.5 의 정정 줄). 설명 스크립트의 성공 상수 규칙은 이 집합에서 서지 않는다(판정 규칙 8.12 (ㄱ)).
- **소속 판정은 사용 자신의 인자에서 멈춘다.** callback 이 사용의 매개변수로 들어오면 그 위의 호출자는 따라가지 않고 "풀리지 않음" 이다(c1·r2 에 한 행씩).
- **다시 뽑기의 키 목록은 없다.** 선언은 다시 뽑기의 `relations.txt` 와 "keys" 를 재실행의 것과 비교한다고 적었으나, 단일 빌드 실행기는 `--keys` 를 쓰지 않는다. relations 글만 비교했다(같음). 키 목록은 재실행의 것이 기록이다.
- 2026-11-16 검토 뒤 정정: §6.7 `builds-4`. 위 항목은 선언의 비교를 덧붙임 없이 뺀 것이다. 단일 빌드 실행기가 `--keys` 를 쓰지 않아도 같은 도구로 키를 낼 수 있었다. §6.0 (가) 1 에서 돌렸다.
- **새 도구의 사본을 두지 않았다**(2.4). 계획 문서는 생성기의 사본을 두라 하지만 그 규칙은 부팅에 닿는 계획의 생성기에 대한 것이다.

### 2.8 열린 것

- 설명 스크립트(`env/obs1/explain.py` 개정 2)가 새 줄(`membership:`, `id holder:`, 생성자의 `callee-check:`, `writers:`)을 담은 행을 읽는지는 이 단계에서 돌려 보지 않았다(부팅 없음, 스크립트 변경 없음). 생성자의 성공 상수는 그 스크립트의 규칙으로 서는 것만 확인했다(`creator_success.txt`). 생성자의 out id 를 실제로 읽으려면 계획이 생성자 호출의 out 을 after-call 에서 기록해야 한다(계획 생성기의 일).
- 판정 규칙 개정 2 의 이름 결합 입력 가운데 이 단계가 준 것: 종류별 표 선택(partition 은 표 선택 상수만), 소속(같은 비교 함수), 생성자 성공 상수. 남은 것: 이름 공간 항목(공통 의미 단계), 다른 이름 대응이 없음의 근거(종류마다), 삭제·회수 site, 이름 수명.
- 2026-11-16 검토 뒤 정정: §6.7 `rules-01`, `rules-07`, `rules-10`. 위 항목의 “이 단계가 준 것”을 이렇게 고쳐 읽는다. 생성자 성공 상수는 주지 않았다(§2.5 의 정정 줄). 종류별 표 선택은 조건 (라)의 정적 반쪽만이다: (라)는 정적 partition 과 실행의 두 쪽 종류 값이 모두 있어야 선다(판정 규칙 8.12 (ㅂ)). 판정 규칙 8.7 셋째 줄이 이 단계에 맡긴 키 필드 writer 의 종류별 나눔과 그 완전성(조건 (다))은 주지 않았고, 뒤 추출 단계가 맡는다(8.12 (ㅈ)).
- 계획에서 빠진 경로 A 행 넷은 같은 실행에서 기록이 있던 행이다(앞 C1 부팅의 계획). 그 기록의 행 번호·키는 앞 계획의 sidecar 가 가진다. 새 계획으로 다음 부팅을 하면 경로 A 의 계획 행은 A21 하나다.
- c5 에서 partition 이 없어져 생긴 짝 13 행(층 2 로 올라감)의 순서 근거는 다음 단계(순서 간선)의 몫이다.

## 3. 순서 간선: kinds 와 전제를 가진 `kz-static order` (kz-static 빌드 하나)

기록: `../records/order-rules-rev1-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/order_rev1/`(order 다섯 빌드), `/home/user/work/redesign/obs1/orderrev1_relcheck/`(relations 다섯 빌드), `/home/user/work/redesign/obs1/order_rev1_extract/`(다섯 빌드 다시 뽑기), `/home/user/work/redesign/obs1/order_rev1_on_extract/`(다시 뽑은 facts 의 order), `/home/user/work/redesign/obs1/order_rev1_plans/`(계획 다섯 빌드), 모두 새 디렉터리. T8CF(`/home/user/work/redesign/step2a/tests_8c_fix`)와 앞선 출력 디렉터리에는 쓰지 않는다. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 3.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다.

- **무엇을 하는가.** 새 명령 `kz-static order <facts> [-o OUT] [--edges TSV]`(규칙은 `kairos/docs/static_facts.md` §5p, 개정 문자열 "order rules revision 1 (2026-11-16)"). 간선: HB-spawn(spawn 호출에서 그것이 시작하는 스레드의 시작으로; 생성 기본 연산까지의 사슬, 상태 조건, child without success), HB-seq(spawn 호출 앞의 사건, 호출 안의 사건, 스레드의 시작, table walk 의 반복 차례), L0-script(시작 스크립트 줄 차례의 spawn 호출), HB-lock(relations 경로 C 가 다른 차례를 proven-impossible 로 매긴 행의 쓰기에서 읽기로), HB-await(인식된 대기 술어가 읽는 필드의 다른 스레드 쓰기에서 대기 뒤 사용으로). 종류 hard / conditional / observed-only, 간선마다 조건·전제·범위·인스턴스 키. 대기마다 탈출을 조건·예산 부분으로 나누고 의존하는 사용마다 예산 탈출의 결과(skip / continue / no-budget / inside / unknown). relations 행마다 `order.static`(hard_before, conditional_before, fixed_early_proved, fixed_early_candidate, unordered, unknown)과 짝·경로·스레드를 시작한 간선·relations 의 다른 차례 실현 가능성. hard 닫힘은 hard 간선만 따라간다.
- **1차 근거.** relations 행에는 간선이 없다: 차례는 한 (쓰기, 읽기) 짝의 실현 가능성 등급으로만 나온다(`kairos/lib/llvm/relations.cpp` 의 `path_c`, `path_a`; 경로 A 는 언제나 `undecidable: ... creation order does not fix the order of initialization`). facts 가 주는 순서 재료: LOCK_PATHS after-spawn(spawn 호출 뒤에 CFG 로 닿는 사건, `kairos/include/kz/schema/kz_facts.h` 의 KZ_FACT_LOCKPATH_FIELDS 주석), SPAWN_WRAP script-entry 의 `via = startup.entry[k]`, LOOP_ITERS, FACT_LOOPS 의 budget·cond·pred, LOOP_EXITS, AWAIT_L1, FIELD_STATUS, INIT, RETURNS·RET_CONSTS. 호출 행에는 guard 가 없다(KZ_FACT_CALL_FIELDS). 스레드 생성의 순서 의미는 P0 항목(POSIX.1-2017 `pthread_create`, XBD 4.12)이고 원문 대조 상태는 P0 표의 것이다.
- **코드**(`kairos/`): 새 `lib/llvm/order.hpp`, `lib/llvm/order.cpp`; `lib/llvm/CMakeLists.txt`(소스 하나와 머리 주석 두 줄); `tools/kz-static/cmd_show.cpp`(`cmd_order`), `commands.hpp`, `main.cpp`(사용법 한 줄, 분기 한 줄, 머리 주석 한 줄), `tools/kz-static/CMakeLists.txt`(머리 주석 한 줄); `tests/static/fixtures/order.c`(새, 사례 이름 없음), `tests/static/static_golden.cpp`(경우 하나와 `order` 갈래), `tests/static/CMakeLists.txt`(fixture·golden 목록, 머리 주석), `tests/static/test_static_cli.cpp`(새 블록 하나); `docs/static_facts.md` §5p(새)와 §5e 의 가리킴 한 문장.
  - 판정 규칙 3.5 의 어휘와 어휘 자료(`tests/static/data/reason_vocabulary.txt`)는 바꾸지 않는다: order 는 `incomplete:` 줄을 쓰지 않고, `unknown` 의 까닭은 값의 글이다(사유 어휘가 아님).
  - 하지 않는 것: relations 의 규칙과 글(개정 7 그대로; `relations.cpp`, `relations.hpp`, `rel_facts.*` 를 고치지 않음), facts schema(`include/`), 추출(`ast_*`, `argflow`, `finalize`, `serialize`, `lockset`, `target_rules`), 계획 생성기(`plan_gen.*`, `cmd_plan.cpp`), `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `instrument/`, `tests/lint/`, 2a 드라이버와 그 허용표, 설명 스크립트(`env/obs1/explain.py`).
- **도구 해시와 정체성**(`cmake/KzSourceIdentity.cmake` 의 규칙: kz-static 의 정체성 경로는 `tools/kz-static`, `lib/llvm`, `lib/core`, `include`, `cmake`, `CMakeLists.txt`):
  - kz-static: 파일 sha256, `--identity` 의 `tool_sha256`, `tool_source` 가 바뀐다. `core_source` `f6e3c7b4…` 는 그대로다 [I].
  - 다시 링크되지 않아야 하는 것(빌드 전 sha256, `pre_mc.txt` 에 다시 잼): kz `931dd39c6bdbe12a3b38750a0350d8f1aac620dc3de8bf60bb0a72e272ba2248`, kz-dump `05623054296bbb993a66308c9fd8e710873f064e7672318e495eabba74fbaf43`, kz-rt-dump `4390d4f13b5bcae3145ddd4b3decb089790d18d6ebbdc3104de99beb66e5063a`, kz-phi `7d34e6e20934c62b91f006c20712b7a096692b231d318257b096eb22dc68075f`, kz-instr-clang `28db40172dbdb06faa0e607b9a494805b19768ca888e865d0a5c3859dab8bcaf`, kz_lint `5f62ec20d2d26e74501cf7469649761df662d673659835ec9bb0902ed6acd018`. kz, kz-phi, kz-dump 의 정체성 정의는 그대로다. kzrt-manifest 는 다시 컴파일되지 않는다(`runtime/` 그대로).
  - 다시 링크되는 시험 프로그램: `kz-llvm` 을 링크하는 여덟, `test_static_reasons`, `test_static_plan`(라이브러리), `static_golden`, `test_static_cli`(소스).
  - CMake 는 `lib/llvm`, `tools/kz-static`, `tests/static` 의 `CMakeLists.txt` 가 바뀌어 다시 구성된다. `CMakeCache.txt` `bc2f43b4…` 는 그대로다 [I](계획 생성기 빌드에서 같은 꼴의 바뀜으로 그대로였다: 결과 문서 `plan/observation_path_results_20261116.md` §4.2). flags.make 목록의 해시는 바뀐다(kz-static 의 정의).
- **도구 보존.** 이 빌드는 kz-static `038cd9e3` 를 제자리에서 덮는다. 그 사본이 아직 없으므로(§2.7) 빌드 전에 압축 사본을 새 디렉터리 `/home/user/work/preserve/tools-20261116-d/kz-static.038cd9e3.xz` 에 만들고 왕복 확인한다(`xz -t`, 스트림 sha256, 제자리 파일과 `cmp`; `tool_copy/ROUNDTRIP.txt`), 공간 감시 아래. 새 도구는 `038cd9e3` 의 relations 다섯과 키 목록을 바이트 동일하게 다시 내야 하고(아래), 계획은 생성기 정체성 줄만 다르다. 그러나 `038cd9e3` 의 계획(relrev7_plans)은 정체성 줄이 다르므로 "대체됨"으로 하지 않고 사본으로 남긴다. 등록부: `038cd9e3` 행 in-place → copy(사본과 그 근거), 새 도구 행 in-place. 장부: 사본 디렉터리 행 `TOOLS_20261116d`.
- **golden**:
  - 새 하나: `order_np.golden`(`KZ_GOLDEN_UPDATE=1` 로 만들고 읽어 본다).
  - 그 밖의 golden(`tests/static/data` 의 나머지, `tests/golden/data`, `tests/phi/data`)은 바뀌지 않는다. 빌드 전에 scratchpad 의 점검 프로그램(아래)으로 기존 fixture golden 23 개가 모두 맞음을 쟀다 [M, 점검 프로그램이라 근거 아님].
- **relations 다섯 빌드**(`rerun.sh`, 새 도구, T8CF 의 NP facts 그대로): `relations.txt` 와 `keys.txt` 가 `038cd9e3` 의 출력(`/home/user/work/redesign/obs1/relrev7_check/<b>/`)과 **바이트 동일**, `relations.log` 는 빈 파일(`compare.sh relations`).
- **order 다섯 빌드**(`order.sh`, T8CF 의 NP facts): 다섯 모두 rc 0, `order.log` 빈 파일, 빌드마다 두 번 돌려 `order.txt` 와 `edges.tsv` 가 바이트 동일(`compare.sh order`; 둘째 실행은 비교 뒤 지운다). 행 수는 relations 의 행 수와 같다(367/367/295/178/175). 측정이고 판정이 아니다.
- **기대**(측정 전 [I]; 빌드 전에 scratchpad 에서 이 소스를 MC 의 `libkz-llvm.a` 사본에 넣어 링크한 점검 프로그램으로 잰 값이고, 근거가 아니다. 빌드 뒤의 도구로 다시 잰다):
  - 행 값(c1_pre_tx / c1_post_tx / r2_core / c5_641 / c5_642): A unknown 60/60/24/53/53, unordered 3/3/1/13/13; B unknown 8/8/43/16/15, unordered 162/162/74/55/54, hard_before 0/0/34/2/2, fixed_early_proved 0/0/4/0/0; C unknown 7/7/1/0/0, unordered 127/127/114/21/2, hard_before 0/0/0/18/36. 경로 A 의 unknown 은 대부분 다른 스레드의 생성자가 없는 lookup 행이다.
  - 간선(나열): HB-spawn 69/69/40/14/14, L0-script 8/8/0/8/8, HB-lock 0/0/0/18/48, HB-await 0/0/0/0/0; 대기 11/12/8/7/8(인식된 술어 5/5/3/0/1); child without success possible 8/8/6/5/5.
  - C1 관계 1 의 행(c1_pre_tx A21, 키 `17975444d83f6589`): unordered, 짝 둘(BP, SCH_LAB 의 생성자 → CF 의 lookup), CF 와 BP 를 시작한 HB-spawn(스크립트 줄 `startup.entry[5]`, `[6]`)과 L0-script 간선, 생성자에서 lookup 으로 가는 간선 없음. 계획 문서의 예상 "A21 unordered" 와 같은 쪽 [I].
  - c5_642 C30(쓰기 `cfe_es_apps.c:652`, 읽기 `cfe_es_api.c:800`): hard_before(HB-lock), 적용 조건에 `OS_TaskCreate returns 0 [OS_SUCCESS]` 와 child without success possible [I]. c5_641 C31: unordered [I]. c1 의 `CFE_ES_GetTaskFunction` 대기(`cfe_es_apps.c:508`): 술어 인식 안 됨, 기다리는 위치 이름 없음 [I]. c1_post_tx 의 CF 재시도 대기(`cf_cfdp.c:1001`, count-bounded): 예산 부분 `(not (lt local:j 25))`, 루프 뒤 사용은 continue, 기다리는 위치 이름 없음(조건이 지역만 읽음) [I].
- **다시 뽑기 다섯 빌드**(`extract.sh`: `env/redesign/one_build.sh` 를 빌드마다, 새 디렉터리 `/home/user/work/redesign/obs1/order_rev1_extract/<b>/`): `env/redesign/compare_build.py` 로 T8CF `<b>/NP` 와 파일마다 비교, 허용 파일 `order_extract_allow.tsv`(이 선언과 함께 커밋; §2.0 의 허용과 같은 갈래): `basis.kz`, `facts.kz`(컨테이너 머리의 도구 해시와 거기서 따라 바뀌는 머리 칸; 섹션 레코드는 BASIS 의 `extract.units_from` 밖에서 모두 같아야 한다), `basis.txt`(첫 줄), `extract.txt`(첫 줄), `relations.txt`, `sha256.txt`, 한쪽에만 있는 `facts.txt`·`status.txt`·`space_guard.log`. 그 `relations.txt` 는 위 `rerun.sh` 의 것과 둘째 줄(facts 해시) 밖에서 바이트 동일해야 한다. 다시 뽑은 facts 로 order 를 돌려(`order.sh <extract 루트> order_rev1_on_extract`) `order.txt` 는 위 order 출력과 둘째 줄(facts 해시) 밖에서 바이트 동일, `edges.tsv` 는 바이트 동일해야 한다(`compare.sh extract`).
- **계획 다섯 빌드**(`plans.sh`, 새 도구, 앞 계획과 같은 인자): 다섯 모두 rc 0. `038cd9e3` 의 계획(`/home/user/work/redesign/obs1/relrev7_plans/<b>/`)과 `compare_plans.py` 로 파일마다: sidecar 둘째 줄(생성기의 `tool_sha256`, `tool_source`, `core_source`), manifest 둘째 줄(sidecar sha256), module 표와 실행 파일 목록의 첫 줄(sidecar 경로와 sha256), `args.txt` 의 출력 디렉터리 밖에서 같아야 한다(`compare.sh plans`). 생성기 코드와 relations 가 바뀌지 않으므로 행·site·층은 같다 [I].
- **시험**: 전체 ctest 248 → 250(새: `static_fixture_compile_order`, `static_golden_order_np`). 라벨: static +2, golden +1. golden 갱신 전 첫 실행에서 실패는 하나여야 한다: golden 이 아직 없는 `static_golden_order_np`. `test_static_cli`(확인 908 개 [I: 위 점검 프로그램과 같은 방법으로 scratchpad 에서 컴파일해 잼], 앞 864 + 44), `test_static_reasons`, `test_static_plan` 은 처음부터 통과한다. 갱신 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). lint 7/7(빌드 전 MC `kz_lint` `5f62ec20` 으로 이 트리에서 7/7 [M]). 단위 점검 `env/tools/tests/run_unit.sh` 15/15, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤 `env/tools/ledger_check.sh` 전체(빌드 전 48 행, 실패 0 [M])와 `env/tools/tools_check.sh --deep`(빌드 전 32 행, 실패 0 [M]). 모두 같아야 한다. 새 출력은 장부 행으로 더한다.
- **공간**: 시작 전 여유 ≥ 하한 314,572,800 + 필요량. 필요량 = 앞 kz-static 빌드 dip 의 최대 219,463,680(`records/obs1-20261116-plangen/commands.txt` §4) + 출력 90 MB(order 다섯 약 7 MB 와 둘째 실행 7 MB, relations 약 8 MB, 다시 뽑기 다섯 약 47 MB, 그 facts 의 order 약 14 MB, 계획 약 1 MB, 여유) + 한 시간 흐름 13 MB = 322,463,680 B(감시 문턱 637,036,480). 사본은 따로 감시(필요량 60,000,000). 감시(`env/tools/space_guard.sh`, 2초마다)가 하한을 넘으면 멈추고 보고한다.
- **비교 스크립트**(빌드 전에 이 선언과 함께 커밋, Python 은 `python3 -I`): `compare.sh`, `compare_plans.py`; 실행 스크립트 `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`; 허용 파일 `order_extract_allow.tsv`.

#### 3.0a 첫 빌드 뒤의 고침 (둘째 빌드 전에 적고 커밋함) [D][M]

첫 빌드(kz-static `b1bf545d`, 11:24Z–11:27Z) 뒤의 선언한 비교는 모두 선언 안이었다(시험, relations, order 두 번, 다시 뽑기, 계획; 그 기록은 §3 의 뒤 절). 그러나 다섯 빌드의 order 출력을 읽다가 child without success 규칙의 결함 둘을 찾았다 [M]:

1. **spawn 연산 사슬이 멈춤.** 안쪽 spawn 호출을 spawn-op 표시가 있는 호출로만 찾았다. C1 의 `OS_TaskCreate` 가 부르는 `OS_TaskCreate_Impl` 은 어휘가 아닌 고정 entry 연산이라(facts 의 SPAWN_WRAP `fixed-entry` 행, 표시 없음) 사슬이 거기서 멈추고 `unknown: OS_TaskCreate makes no spawn call in its body` 가 되었다(c1_pre_tx 의 spawn 호출 5 곳). 고침: 그 함수의 SPAWN_WRAP 행(script-entry 밖의 모든 종류)의 spawn 호출도 안쪽 spawn 호출이다.
2. **안쪽 실패 아래의 반환을 늘 뺌.** 반환의 조건이 안쪽 호출의 성공 값 아래 거짓이면 뺐다. 이것은 안쪽 연산이 실패를 돌려줄 때 스레드가 없을 때만 옳다. 안쪽 연산 자신이 스레드를 만든 뒤 실패를 돌려줄 수 있으면(그 표시가 `not found` 가 아니면), 그 실패를 제 상수로 바꿔 돌려주는 바깥 반환도 child without success 다. 고침: 안쪽 표시가 `not found` 일 때만 뺀다; 아니면 그 반환을 `possible` 로 세고 괄호에 안쪽 호출을 적는다. 덧붙여 스레드 생성 기본 연산의 결과를 그대로 돌려주는 연산의 상태 조건에 그 기본 연산의 성공 값 0 을 든다(첫 빌드는 `not determined` 였다).

- **코드**: `lib/llvm/order.cpp`(`child_without_success`, `consts_of`)만. fixture `order.c` 에 경우 하나(`start_checked` 위에서 안쪽 실패를 제 상수로 바꾸는 `start_mapped`, 그것으로 spawn 하는 `mapped_spawn`), `test_static_cli` 의 확인 셋 더(908 → 911 [I: scratchpad 점검 프로그램으로 잼]), `docs/static_facts.md` §5p 의 두 문장. 그 밖은 첫 빌드와 같다.
- **정체성·다시 링크**: §3.0 과 같다(kz-static 만 바뀌고, 다시 링크되지 않아야 하는 여섯의 sha256 은 §3.0 의 값). `CMakeCache.txt` 그대로 [I](CMakeLists 는 바뀌지 않는다).
- **도구 보존**: 이 빌드는 첫 빌드의 kz-static `b1bf545defe081a0de513ede65d2c6021388ddf2b46490983e2bc26a4831fa60` 을 덮는다. 그 출력(order_rev1, orderrev1_relcheck, order_rev1_extract, order_rev1_on_extract, order_rev1_plans)은 남기므로 빌드 전에 압축 사본을 `/home/user/work/preserve/tools-20261116-d/kz-static.b1bf545d.xz` 에 만들고 왕복 확인한다(`tool_copy/ROUNDTRIP_first.txt`). 등록부 행 copy.
- **golden**: `order_np.golden` 은 fixture 가 바뀌어 다시 쓴다(위치가 밀리고 행 C5 와 그 간선이 더해진다). 첫 ctest 에서 실패는 `static_golden_order_np` 하나여야 한다. 갱신한 golden 은 읽어 보고, 새 행 C5 의 child without success 가 `possible` 이고 안쪽 호출을 적음을 확인한다. 그 밖의 golden 은 바뀌지 않는다(scratchpad 점검 프로그램으로 기존 23 개 맞음 [M, 근거 아님]).
- **둘째 빌드의 출력**(모두 새 디렉터리, 스크립트는 출력 디렉터리를 환경 변수로 받게 고쳤고 기본값은 첫 빌드의 것): relations `orderrev1b_relcheck`(relrev7_check 와 바이트 동일), order `order_rev1b`(두 번 같은 바이트; 첫 빌드의 `order_rev1` 과 `compare_order.py` 로: 줄 수 같고, 다를 수 있는 줄은 HB-spawn 간선의 `create primitive:`, `status condition:`, `child without success:` 줄, 머리의 `summary spawn calls with resolved threads` 줄, HB-lock 간선의 `condition: the write runs:` 줄, `; applies when the write runs:` 앞이 같은 행의 짝 줄, `edges.tsv` 에서 조건 칸만 다른 HB-lock 줄뿐), 다시 뽑기 `order_rev1b_extract` 와 그 order `order_rev1b_on_extract`(§3.0 과 같은 비교), 계획 `order_rev1b_plans`(relrev7_plans 와 §3.0 의 선언한 줄 밖에서 같음).
- **기대**(측정 전 [I], scratchpad 점검 프로그램으로 잼): child without success possible / not found / unknown: c1_pre_tx 8/4/1 → 12/1/0, c1_post_tx 같음, r2_core 6/4/1 → 9/2/0, c5_641·c5_642 5/1/0 그대로; 다른 줄 수 c1 71, r2 39, c5 3. 행 값·간선 수·대기는 그대로. C1 의 가장 아래 단계(`OS_Posix_InternalTaskCreate_Impl`, `os-impl-tasks.c:543` 의 `pthread_create` 뒤 `:547` 의 `return OS_ERROR`)가 `possible` 로 나오는데, 소스를 읽으면 그 반환은 `pthread_create` 가 실패한 갈래다(`:544` `if (return_code != 0)`; 성공 뒤에는 `:562` `return OS_SUCCESS` 뿐이고 `:551-554` 의 주석이 그렇게 적는다) [M: 묶음 1df262a 의 소스]. 조건의 지역 `return_code` 가 정의를 여럿 가져(`:475` 부터) RETURNS 의 조건에서 펼쳐지지 않으므로 규칙은 그 반환을 뺄 수 없다: 이것은 규칙이 적는 과대 근사(`possible` 은 일어난다는 말이 아님)이고, 고침 2 가 그것을 위 단계로 올린다. 결과에 그렇게 적는다.
- **공간**: §3.0 과 같은 필요량과 감시.

### 3.1 바뀐 것 [F]

- 커밋 `f3ebf65`(선언, 코드, fixture, 시험, 문서, 비교 스크립트를 함께, 첫 빌드 전). 규모: `lib/llvm/order.cpp` 1,315 줄(새), `order.hpp` 51 줄(새), `tests/static/fixtures/order.c`(새), `test_static_cli.cpp` +75, `static_golden.cpp` +21, `cmd_show.cpp` +40, `docs/static_facts.md` §5p.
- 커밋 `6ae9a41`(첫 빌드의 기록, 첫 빌드의 golden, 덧붙임 §3.0a, `order.cpp` 의 child without success 고침, fixture 의 경우 하나, 확인 셋, 문서 두 문장; 둘째 빌드 전). 스크립트는 출력 디렉터리를 환경 변수로 받게 고쳤고(기본값은 첫 빌드의 것) `compare_order.py` 와 `compare.sh first` 를 더했다.
- 둘째 빌드 뒤: `order_np.golden` 갱신, 이 절과 기록, 장부·등록부 행.

### 3.2 빌드 [M]

| | 첫 빌드 | 둘째 빌드 |
|---|---|---|
| 때 (UTC) | 11:24:39–11:27:07 | 11:43:24–11:45:57 |
| rc, 링크 줄 | 0, 14 (`libkz-llvm.a`, kz-static, `kz-llvm` 을 링크하는 시험 여덟, `test_static_reasons`, `test_static_plan`, `static_golden`, `test_static_cli`) | 0, 13 (`static_golden` 은 소스가 바뀌지 않아 다시 링크되지 않음) |
| kz-static | `038cd9e3…` → `b1bf545defe081a0de513ede65d2c6021388ddf2b46490983e2bc26a4831fa60` (190,536,096 B) | → `b52db538769e6bc8fecd9a7e3daec63fd785885c1873629c0dac244afa50b2f9` (190,578,904 B) |
| tool_sha256 / tool_source | `31b0bfe7…` / `a8e00a05…` | `0c45b0b8ae991308ca0005e4121cb5da5728fcde69a9e64aa9f359e67020ad0d` / `816f41771a951db8c35dfaf1642f724c0715e1203e380092484577840501eef8` |
| core_source | `f6e3c7b4…` 같음 | 같음 |
| kz / kz-dump / kz-rt-dump / kz-phi / kz_lint / kz-instr-clang | §3.0 의 sha256 그대로 | 그대로 |
| kz, kz-phi, kz-dump 의 정체성 정의, `CMakeCache.txt` `bc2f43b4…` | 같음 | 같음 |
| flags.make 목록 해시 | `7d2a0717…` → `7a538b6e…` | → `321b751c…` |
| 빌드 시작 뒤 바뀐 정체성 경로의 파일 (`find -newer`) | 0 | 0 |

- 기록: `pre_mc.txt`, `post_mc.txt`, `make.log`, `df_build.log`(첫 빌드), `pre_mc_build2.txt`, `post_mc_build2.txt`, `make_build2.log`, `df_build2.log`(둘째 빌드). 선언대로다. 둘째 빌드가 `static_golden` 을 다시 링크하지 않은 것은 선언(§3.0a 의 "§3.0 과 같다")보다 적은 바뀜이다.

### 3.3 시험 [M]

- 첫 빌드: golden 갱신 전 전체 ctest(`ctest_first.log`) 250 개 가운데 실패 1, 선언한 `static_golden_order_np`(golden 없음) 그대로. `KZ_GOLDEN_UPDATE=1` 로 쓴 `order_np.golden`(453 줄)은 scratchpad 점검 프로그램의 것과 `cmp` 같다. 갱신 뒤(`ctest_full.log`) 250 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음; `skip_reason.log`). `test_static_cli` 908, `test_static_reasons` 655, `test_static_plan` 182, `lint_selftest` 109 경우 실패 0, lint 7/7. 라벨 golden 62, static 55(+1, +2). 단위 점검 15/15, 7/7.
- 둘째 빌드: 첫 ctest(`ctest_first_build2.log`) 실패 1, 선언한 `static_golden_order_np`(fixture 가 바뀜) 그대로. 갱신한 golden(491 줄, `golden_update_build2.log`)을 읽었다: 행 C5(`mapped_spawn`, hard_before, HB-lock)가 더해지고 그 spawn 간선의 child without success 가 `possible: start_mapped returns -12 [ERR_MAPPED] at order.c:190:16 after order.c:189:12 (the inner call order.c:189:12 may report a failure after its thread started: possible)`, `start_plain` 의 상태 조건이 `start_plain returns 0 [pthread_create created the thread (P0 pthread_create)]`; 나머지 행 값은 첫 빌드와 같다(A1 hard_before, A2 fixed_early_proved, A3 unordered, B1 unordered(짝 hard_before 1, unordered 1), B2 conditional_before(짝 conditional_before 1, hard_before 1), C1·C3·C4 hard_before, C2 unordered). 갱신 뒤(`ctest_full_build2.log`) 250 개, 실패 0, 건너뜀 1; `test_static_cli` 911, `test_static_reasons` 655, `test_static_plan` 182, lint 7/7, selftest 109; 단위 점검 15/15(`unit_tools_build2.txt`), 7/7(`unit_step2a_build2.txt`). 그 밖의 golden 은 바뀌지 않았다(`git status`).
- 2026-11-16 검토 뒤 정정: §6.7 `builds-8`. 위의 확인 수(`test_static_cli` 908/911, `test_static_reasons` 655, `test_static_plan` 182, selftest 109)는 콘솔에서 읽어 `commands.txt` 에만 적었고 도구 출력으로 남기지 않았다. 그 시험 프로그램은 뒤 빌드가 덮어 다시 잴 수 없다.

### 3.4 바이트 동일성 [M]

| 비교 | 첫 빌드 (`b1bf545d`) | 둘째 빌드 (`b52db538`) |
|---|---|---|
| relations·키 목록 다섯 빌드 vs `relrev7_check` | 바이트 동일, `relations.log` 빈 파일 (`compare_relations.txt`, 15 OK) | 같음 (`compare_relations_build2.txt`, 15 OK) |
| order 두 번 실행 | 다섯 모두 `order.txt`·`edges.tsv` 같은 바이트, `order.log` 빈 파일 (`compare_order.txt`) | 같음 (`compare_order_build2.txt`) |
| 둘째 빌드 order vs 첫 빌드 | — | 다섯 모두 선언한 줄만 다름 (`compare_first_build2.txt`: order.txt 71/71/39/3/3 줄, edges.tsv 0 줄) |
| 다시 뽑기 vs T8CF (`compare_build.py`, 허용 파일) | 허용 밖 0 (다섯) | 허용 밖 0 (다섯) |
| facts.kz | 머리의 artifact·basis·부모·도구 해시, STRINGS crc32, BASIS `extract.units_from` 만 다름 | 같은 갈래 |
| 다시 뽑은 facts 의 order vs T8CF facts 의 order | `order.txt` 둘째 줄 밖 같음, `edges.tsv` 같음; 다시 뽑기의 `relations.txt` 는 relations 재실행의 것과 둘째 줄 밖 같음 (`compare_extract.txt`, 15 OK) | 같음 (`compare_extract_build2.txt`, 15 OK) |
| 계획 다섯 빌드 vs `relrev7_plans` | 선언한 줄만 (sidecar 둘째 줄, manifest 둘째 줄, module 표·실행 파일 목록 첫 줄; `compare_plans.txt`) | 같음 (`compare_plans_build2.txt`) |

- **누적 목록**: `ledger_check.sh` 59 행, 실패 0(새 행 열하나: `OBS_orderrev1_order`, `OBS_orderrev1_relcheck`, `OBS_orderrev1_extract`, `OBS_orderrev1_onextract`, `OBS_orderrev1_plans`, 같은 다섯의 `…1b…`, `TOOLS_20261116d`). `tools_check.sh --deep` 34 행, 실패 0. 바깥 이름 점검(`--tree`)은 열 출력 디렉터리와 사본 디렉터리에서 깨끗하다.
- 2026-11-16 검토 뒤 정정: §6.7 `builds-6`. 누적 목록과 등록부는 첫 빌드 전(`commands.txt:19`)과 둘째 빌드 뒤(`:55`)에만 돌렸고, 첫 빌드와 둘째 빌드 사이에는 돌리지 않았다(빌드마다 다시 본다는 규칙에 어긋남). 끝의 실행은 통과했다. 검토 단계(§6)는 빌드 전, 빌드 뒤, 끝의 출력을 따로 남겼다.
- **도구 보존**: `038cd9e3` 사본(`tools-20261116-d/kz-static.038cd9e3.xz`, 41,217,284 B)은 첫 빌드 전, `b1bf545d` 사본(`kz-static.b1bf545d.xz`, 42,426,196 B)은 둘째 빌드 전에 만들고 왕복 확인했다(`tool_copy/ROUNDTRIP.txt`, `ROUNDTRIP_first.txt`: `xz -t` OK, 스트림 sha256 같음, `cmp` 같음). 등록부: `038cd9e3` in-place → copy(그 relations·키 목록은 두 후속 도구가 바이트 동일하게 다시 냈으나 계획은 생성기 정체성 줄이 달라 "대체됨"으로 하지 않음), `b1bf545d` copy(출력이 남고, 둘째 빌드가 child without success 줄을 바꾸므로 대체되지 않음), `b52db538` in-place. 새 도구의 사본은 두지 않았다: 이 단계의 계획은 부팅에 닿지 않는다. 다음 제자리 빌드 전에 사본을 만든다.

### 3.5 측정 [M]

둘째 빌드의 출력(`/home/user/work/redesign/obs1/order_rev1b/<b>/`)이다. 행 값·간선·대기의 수는 첫 빌드와 같고, 다른 것은 child without success 뿐이다. 선언의 [I] 값(scratchpad 점검 프로그램)과 모두 같다.

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| 행 (= relations 행) | 367 | 367 | 295 | 178 | 175 |
| A: unknown / unordered | 60 / 3 | 60 / 3 | 24 / 1 | 53 / 13 | 53 / 13 |
| B: unknown / unordered / hard_before / fixed_early_proved | 8 / 162 / 0 / 0 | 8 / 162 / 0 / 0 | 43 / 74 / 34 / 4 | 16 / 55 / 2 / 0 | 15 / 54 / 2 / 0 |
| C: unknown / unordered / hard_before | 7 / 127 / 0 | 7 / 127 / 0 | 1 / 114 / 0 | 0 / 21 / 18 | 0 / 2 / 36 |
| 나열한 간선: HB-spawn / L0-script / HB-lock / HB-await / HB-seq; 그 가운데 observed-only | 69 / 8 / 0 / 0 / 402; 4 (L0-script) | 같음 | 40 / 0 / 0 / 0 / 477; 4 (HB-seq 반복) | 14 / 8 / 18 / 0 / 51; 4 (L0-script) | 14 / 8 / 48 / 0 / 51; 4 (L0-script) |
| 닫힘이 따라간 HB-seq 걸음 | 30,956 | 30,956 | 15,067 | 8,763 | 8,760 |
| spawn 호출, child without success possible / not found / unknown | 13: 12 / 1 / 0 | 13: 12 / 1 / 0 | 11: 9 / 2 / 0 | 6: 5 / 1 / 0 | 6: 5 / 1 / 0 |
| 첫 빌드의 같은 칸 | 13: 8 / 4 / 1 | 같음 | 11: 6 / 4 / 1 | 6: 5 / 1 / 0 | 6: 5 / 1 / 0 |
| 대기 (poll, count-bounded; 술어 인식) | 11 (10, 1; 5) | 12 (10, 2; 5) | 8 (8, 0; 3) | 7 (6, 1; 0) | 8 (7, 1; 1) |
| order.txt 바이트 | 1,982,237 | 1,986,709 | 1,433,164 | 559,526 | 687,260 |

(나열한 HB-seq 는 행의 나열한 짝이 쓴 hard 걸음, table walk 반복 간선과 그 observed-only 시작 차례; 머리의 `summary edges listed` 가 규칙·종류별로 센다. 경로 A 의 unknown 은 다른 스레드의 생성자가 없는 lookup 행이고(`pairs: none`), B·C 의 unknown 은 root 가 닿지 않는 함수의 짝이다.)

**계획 문서의 예상(모두 [I])과 측정**:

| 예상 [I] | 측정 [M] |
|---|---|
| c5_642 C30 → `hard_before`, 조건 `OS_TaskCreate status = OS_SUCCESS`, 전제 = 행의 rests-on | `hard_before`(짝 넷: 쓰기 `cfe_es_apps.c:652:18` → 읽기 `cfe_es_api.c:800:41`, 스레드 CI_Lab·SAMPLE·SCH_Lab·TO_Lab), 경로 HB-lock 하나; 적용 조건 `OS_TaskCreate returns 0 [OS_SUCCESS]` 와 쓰기의 WRITERS guard, 전제 = 잠금 #9(정적 인스턴스)와 행의 rests-on; child without success `possible`: `OS_TaskCreate` 가 `osapi.c:645:19` 의 `pthread_create` 뒤 `:657`, `:672`, `:684` 에서 `OS_ERROR` 를 돌려줌. 소스(`osal-4.1.1-release/src/os/posix/osapi.c`)를 읽으면 `:657` 은 `pthread_create` 가 실패한 갈래(스레드 없음)이고 `:672`(`pthread_detach` 실패), `:684`(`pthread_attr_destroy` 실패)는 스레드가 이미 있는 갈래다 [M] |
| c5_641 C31 → `unordered`, 반대 "possible: lock window" | `unordered`(짝 넷), `opposite: possible` |
| c1_pre_tx A21 → `unordered`, HB-spawn(ES→CF), HB-spawn(ES→BP), ES 스레드의 HB-seq(스크립트 줄 차례), BP 의 생성으로 가는 간선 없음 | `unordered`(짝 둘: BP `bp_flow.c:132:18`, SCH_LAB `sch_lab_app.c:168:16` → CF `cf_cfdp.c:993:19`), 어느 쪽 경로도 없음. CF·BP 를 시작한 HB-spawn 은 `cfe_es_apps.c:768:13`(`startup.entry[5]`, `[6]`; 그 아래 `:588:16`, `os-impl-tasks.c:543:19`, `:586:19` 의 단계도), L0-script `E02cf58c34fe36b45`(entry[5] CF → entry[6] BP, hard, L0 전제)와 `E8f0661325db43c8e`(entry[4] SCH_LAB → entry[5]), 시작 차례는 observed-only(`Ef406c0372ad4e25e`, `E9a8bef6c7e446327`). c1_post_tx 의 A21 도 같다 |
| c1_pre_tx loop 177 → "predicate not recognised" | `cfe_es_apps.c:508:5` `CFE_ES_GetTaskFunction` poll: 술어 인식 안 됨(지역 `ReturnCode`), 조건 부분 `(eq local:ReturnCode 0)`, 예산 부분 `(le local:Timeout 0)`(같은 break), 지연 `OS_TaskDelay` 인자 50, 기다리는 위치 이름 없음, 의존하는 사용은 모두 대기 안 |
| c1_post_tx CF 재시도 대기(`cf_cfdp.c:1001`) → 예산 탈출 뒤 사용은 성공 아래에서만, BP 생성 → 그 사용 hard(reads-from 전제), lookup 자체는 unordered | count-bounded, 예산 지역 `j`, trip 25, 지연 인자 100; 탈출 0 예산 부분 `(not (lt local:j 25))`, 탈출 1 조건 부분 `(ne local:ret -17)`; 술어 인식 안 됨(조건이 지역만 읽음), 기다리는 위치 이름 없음 → HB-await 없음; 루프 뒤 읽기와 호출은 `continue`. 예상한 hard 간선은 나오지 않았다: 조건이 읽는 `ret` 이 lookup 의 결과를 담은 지역이고, 이 facts 는 그 지역과 기다리는 객체를 잇지 않는다 |

- **HB-await 0**(다섯 빌드): 술어가 인식된 대기(5/5/3/0/1)는 기다리는 필드와 다른 스레드의 writer 를 가진다(`awaited_build2.txt`: 예 `CFE_ES_Global_t.SystemState` writer 5). 그러나 그 함수 안에서 루프 뒤의 읽기·호출이 없고, 의존하는 사용은 대기 안의 것과 그 함수의 호출(호출자 쪽 사용)뿐이다. 이 개정은 호출자 쪽 사용으로 간선을 내지 않는다(호출의 조건이 facts 에 없음). fixture 에서는 hard 둘(skip), conditional 둘(continue)이 나온다.
- **C1 의 child without success 12 possible**: 고침 1 로 사슬이 `OS_TaskCreate → OS_TaskCreate_Impl → OS_Posix_InternalTaskCreate_Impl → pthread_create` 까지 간다. 가장 아래 단계가 `possible`(`os-impl-tasks.c:547:9` 의 `return OS_ERROR`, `:543:19` 뒤)이고 고침 2 가 위로 올린다. 소스를 읽으면 `:547` 은 `pthread_create` 실패 갈래이고 성공 뒤에는 `:562` 의 `return OS_SUCCESS` 뿐이다(§3.0a) [M]: 이 `possible` 은 규칙의 과대 근사(조건의 지역 `return_code` 가 정의를 여럿 가져 펼쳐지지 않음)이고, C1 의 스레드가 성공 없이 남는다는 근거가 아니다. c5 의 `possible` 에는 `:672`, `:684` 처럼 스레드가 이미 있는 갈래의 반환이 들어 있다.
- **r2_core B 의 hard_before 34, fixed_early_proved 4**: 쓰기와 사용 사이에 spawn 호출이 있는 짝(쓰기가 spawn 앞이면 hard_before, 사용이 다른 스레드의 spawn 앞이면 fixed_early_proved)이다. 측정이고 판정이 아니다.
- 기록: `summary_build2.txt`(다섯 빌드의 머리 줄), `named_rows_build2.txt`(위 행과 대기의 글 그대로), `awaited_build2.txt`, `spawn_flags.txt`(첫 빌드의 spawn 간선 표시).
- 2026-11-16 검토 뒤 정정: §6.7 `semantics-1`. 위 표의 B·C 의 짝과 나열한 간선에는 닫힘이 공유 spawn 호출에서, 그 호출에 닿은 스레드와 무관하게 그 호출의 모든 자식으로 가서 생긴 경로가 들어 있다(`order.cpp` 빌드 전 판 `:957-961`, `:963-967`). 예: c1_pre_tx 의 BP 쓰기(`cfe_es_syslog.c:219:9`) → EVS 사용(`cfe_evs_log.c:207:17`) hard_before 의 경로는 “BP 의 `CFE_ES_CreateChildTask` 호출이 BackgroundTask 를 시작하고 BackgroundTask 의 호출이 EVS 를 시작”인데, 두 걸음 모두 소스로 틀렸다(BP 의 그 호출은 `BP_CLA_*` 만 만들고, BackgroundTask 는 ES 가, EVS 는 main 이 만든다). r2 의 TIME 쓰기 → BackgroundTask 사용 둘과 B37 의 “BackgroundTask 가 `CFE_TIME_TaskMain` 을 시작” 걸음도 같다. order rules revision 2 로 다시 잰 값(§6.5): 행 값은 다섯 빌드 모두 그대로, 모든 짝의 hard_before 5,023 → 1,525(c1 두 빌드), 1,660 → 1,117(r2), fixed_early_proved 2,549 → 1,276, 1,348 → 825, c5 그대로; 나열한 간선 c1 479 → 308, r2 517 → 498; 닫힘이 따라간 HB-seq 걸음 c1 30,956 → 30,824, r2 15,067 → 15,030. 검토가 든 짝은 모두 unordered 다.
- 2026-11-16 검토 뒤 정정: §6.7 `semantics-2`. 위 표의 child without success `possible` 12/12/9/5/5 와 위 항목의 “c5 의 `possible`”은 사실로 적힌 표지가 거짓 증거에 섰다. C1 의 가장 아래 `os-impl-tasks.c:547` 은 `pthread_create` 실패 갈래이고(성공 뒤 반환은 `:562` 뿐), `OS_TaskCreate` 의 `osapi-task.c:202` 반환은 `:199` 의 `OS_ObjectIdFinalizeNew` 값이며(`:196` 의 `AllocateNew` 값은 다시 정의됨), `CFE_ES_CreateChildTask` 의 `CFE_ES_BAD_ARGUMENT`(`cfe_es_api.c:1332`)는 `:1327` 의 조건이 spawn 을 건너뛸 때만의 값이다. C5 의 첫 증거 `osapi.c:657` 도 `pthread_create` 실패 갈래다. 도구 출력에는 단서가 없었다. order rules revision 2 로 다시 잰 값: possible 0/0/0/0/0, unknown (position-based) 12/12/9/5/5, not found 1/1/2/1/1(§6.5). c5 의 `:672`, `:684`(스레드가 있는 갈래)도 조건이 지역을 읽어 위치 근거로 내려간다(덜 주장하는 쪽).

### 3.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 이 단계 처음 | 3,627,954,176 | 시작 |
| 11:22:19 | 3,147,370,496 | 첫 사본 시작(그 사이 scratchpad 점검 프로그램 약 0.49 GB) |
| 11:24:16 | 3,106,140,160 | 사본 `038cd9e3` 끝(dip 41,230,336) |
| 11:27:07 | 3,071,279,104 | 첫 빌드 끝(dip 53,276,672; 감시 문턱 637,036,480) |
| 11:32:44 | 2,998,980,608 | 첫 빌드의 relations(dip 7,610,368), order(15,544,320), 다시 뽑기(47,054,848), 그 order(15,548,416), 계획(987,136) 뒤 |
| 11:43:04 | 2,936,905,728 | 사본 `b1bf545d` 끝(dip 42,430,464) |
| 11:45:57 | 2,936,463,360 | 둘째 빌드 끝(dip 26,386,432) |
| 11:51:09 | 2,864,209,920 | 둘째 빌드의 relations·order·다시 뽑기·order·계획 뒤(dip 같은 크기) |

- 하한을 넘지 않았다. 남긴 출력: 첫 빌드 70,771,546 B, 둘째 빌드 70,830,014 B(열 디렉터리, 장부 행), 사본 둘 83,643,480 B. 지운 것: 비교 뒤의 둘째 실행(`again/`), 그리고 이 단계 끝에 scratchpad 의 이 단계 작업 파일(점검 프로그램, 그 출력, golden 사본)뿐이다.

### 3.7 계획과 다르게 한 것

- **빌드가 둘이다.** 첫 빌드의 선언한 비교는 모두 선언 안이었으나 출력을 읽다가 child without success 규칙의 결함 둘을 찾았다(§3.0a). 고침과 덧붙임을 둘째 빌드 전에 커밋했고(`6ae9a41`), 첫 빌드의 도구는 사본으로, 출력은 그대로 남겼다. 빌드 전 점검 프로그램이 첫 빌드 뒤 한때 MC 라이브러리의 옛 `order.cpp.o` 를 링크하고 있었다(같은 이름의 멤버가 둘); 고친 뒤 다시 쟀다(`commands.txt` §2).
- **"사용이 쓰기보다 앞" 의 값 이름.** 설계 연구의 값 목록(hard_before, conditional_before, fixed_early_proved, fixed_early_candidate, unordered, unknown)에 뜻이 적혀 있지 않아, fixed_early_proved / fixed_early_candidate 를 "사용이 쓰기·생성보다 앞(hard / conditional 경로)" 으로 정했다.
- **HB-seq 는 facts 가 주는 꼴로만.** 한 스레드 안의 일반 프로그램 순서(CFG 차례)는 facts 에 spawn 호출 뒤 도달(LOCK_PATHS)밖에 없다. 그래서 같은 스레드의 두 사건은 `unknown` 이고, 쓰기에서 같은 스레드의 다른 쓰기로 가는 차례(예: 데이터 쓰기 뒤 플래그 쓰기)는 쓰지 않는다.
- **전제는 글 줄이다.** 설계 연구는 기계가 읽는 전제 목록(`writer-set:<field>:<rule>` 꼴)을 적었다. 이 개정은 간선마다 `premise:` 줄과 TSV 의 `; ` 로 이은 칸이다.
- **HB-lock 은 relations 경로 C 의 판정에 기댄다.** 같은 잠금 규칙을 다시 세우지 않고 `proven-impossible` 행에서 간선을 만들며, 잠금은 spawn 과 읽기에서 함께 잡힌 정적 인스턴스로 적는다.
- **대기의 의존하는 사용은 그 함수의 루프 뒤 사건과 그 함수의 호출까지.** 호출자 쪽의 결과 시험은 `unknown(the conditions of its calls are not in these facts)` 이고 HB-await 간선은 호출자 쪽 사용으로 가지 않는다(다섯 빌드의 HB-await 0 의 까닭). 예산 탈출 뒤 사용 앞의 종료 호출은 읽지 않는다.
- **행마다 값별 짝 줄 6 개**(출력 한도; 모든 짝을 계산하고 `pairs:` 줄이 센다). 경로 B 의 행 하나가 짝 5,240 개(그 행의 글만 2.5 MB)를 가져 c1_pre_tx 의 글이 11.6 MB 가 되었기 때문이다(점검 프로그램).
- **L0-script 의 fixture 경우가 없다.** 시작 스크립트 진입은 basis 의 `startup.entry` 와 symbol lookup 사슬이 있어야 나오고 fixture 는 basis 없이 추출한다. 실제 빌드(C1·C5 의 8 개)에서만 나왔다.
- **child without success 는 위치로 "뒤" 를 읽는다**(문서에 적은 과대 근사). C1 의 가장 아래 단계에서 그것이 거짓 `possible` 을 낸다(§3.5).
- 2026-11-16 검토 뒤 정정: §6.7 `semantics-1`, `semantics-2`. 위 항목의 “문서에 적은 과대 근사”는 도구 출력에 단서 없이 사실로 적혔고(§3.5 의 정정 줄), 닫힘이 공유 spawn 호출에서 스레드를 가리지 않은 것은 이 절에 적지 않은 결함이었다. 둘 다 order rules revision 2 에서 고쳤다(§6.0 (나), §6.5).

### 3.8 열린 것

- 호출의 guard(facts schema 개정 11 의 호출 행에 없음): 예산 탈출 뒤 호출자 쪽 사용의 결과, HB-await 의 호출자 쪽 간선, `skip` 의 판정이 그것을 기다린다. 조건의 지역을 그 정의에 잇는 사실(reaching definitions)이 있으면 child without success 의 위치 근사도 좁힐 수 있다(C1 의 거짓 `possible`).
- 2026-11-16 검토 뒤 정정: §6.7 `semantics-1`, `semantics-2`. 위 항목의 거짓 `possible` 은 이제 `unknown (position-based)` 로 나오고, 공유 spawn 호출의 스레드는 호출자에 기대는 규칙으로 가린다(order rules revision 2). 지역의 정의를 호출 결과에 잇는 사실이 오면 위치 근거의 unknown 을 다시 잴 수 있다(§6.9).
- 지역 플래그 술어(loop 177, CF 재시도 대기의 `ret`)와 2 세대 잠금의 handle 단위 정체성(C1 의 HB-lock 0): 추출과 잠금 정체성 단계의 일.
- 인스턴스 키는 글이다. 런타임 1.4 는 생성 토큰을 기록하지 않으므로 HB-spawn 을 실행의 스레드에 잇는 것은 생성 인자 결합(C1)이거나 없음(C5)이다. order 출력을 계획(manifest 개정 4)과 설명 스크립트가 읽는 것은 뒤 단계다. 설명 스크립트(`env/obs1/explain.py` 개정 2)는 바꾸지 않았고 앞 단계에서 넘어온 `explain.py:970-973` 의 refuted 갈래도 그대로다.
- 경로 A 의 unknown 60/60/24/53/53 은 다른 스레드의 생성자가 없는 lookup 행이다(§2.5 의 생성자 없음의 까닭). 이름 결합 입력이 채워지면 다시 잰다.

## 4. 스키마 개정 12: 새 섹션만, 뒤 작업의 공개 구조를 한 번에 (모든 도구, 목표별 빌드)

기록: `../records/schema-rev12-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `schema12_readback`(읽기 시험의 kz-dump 판), `schema12_regress`(옛·새 kz 의 convert·replay), `schema12_c1boot_convert`(관측 주기 C1 부팅 dump 의 재변환), `schema12_relcheck`, `schema12_order`, `schema12_extract`, `schema12_on_extract`, `schema12_plans`(다섯 빌드). 도구 사본: 새 디렉터리 `/home/user/work/preserve/tools-20261116-e/`. T8CF 와 앞선 출력 디렉터리에는 쓰지 않는다. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 4.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다.

- **무엇을 하는가.** 스키마 개정 12(`KZ_SCHEMA_REVISION` 11 → 12). 새 섹션만 더하고, 뒤 작업이 쓸 레코드와 공개 구조를 `include/` 에 한 번에 둔다(`kairos/docs/schema.md` §20).
  - 추출 섹션 여덟(추출 레코드 개정 10): OBJECT_ACCESS 137, FIELD_TYPES 138, ARG_TYPES 139(둘째 추출 범위를 채움), LOOP_DEFS 140, SEMANTICS 141, RESULT_FLOW 142, RESULT_EFFECT 143, LOCK_HANDLES 144(셋째 추출 범위 140–159 를 엶). 레코드 크기 72/60/44/48/72/76/72/56 B, 모두 버전 1.
  - 중립 층 섹션 일곱(필드 단위 직렬화 범위 안): NACTOR_TOKEN 112, NLIFE 113, NBUILD 114, NSUFFICE 115, NCALLEND 116, NORDER 117, NORDER_OBS 118. 레코드 크기 80/56/104/44/28/60/48 B, 모두 버전 1. actor·사건·site·레코드 번호로 잇는 곁 표다.
  - 새 열거형 50(추출 29, 중립 21), 값 0 은 모두 보수 쪽 또는 빈 집합.
  - 공개 구조: `include/kz/trace/neutral.hpp` 의 `NeutralRun` 에 곁 표 일곱(벡터와 문자열 구조, 기본값 함수 여섯), `lib/trace/neutral.cpp` 의 `check_neutral` 검사, `read_neutral` 의 선택 섹션 읽기, `add_neutral_sections` 의 "레코드가 있을 때만" 쓰기; 새 `include/kz/trace/run_analysis.hpp`(같은 빌드 판정, 토큰 결합, 충분성, 수명, 순서 인스턴스의 입력·결과 구조; 함수 없음).
  - `--identity`: kz 와 kz-phi 에 새로, kz-dump 와 kz-static 에 `schema_revision=`·`schema_fingerprint=` 두 줄(앞 네 줄 그대로).
- **1차 근거.** 컨테이너 읽기는 섹션마다 레코드 크기와 섹션 버전이 등록부와 다르면 거부한다(`kairos/lib/core/container_reader.cpp` 섹션 표 검사, `record_size` 와 `schema_version` 비교). 기존 섹션의 둘을 바꾸지 않으므로 옛 컨테이너는 그대로 읽힌다. `include/` 는 모든 도구의 소스 identity 에 들어가고(`kairos/cmake/KzSourceIdentity.cmake` 의 `kz_tool_identity`: `include`, `cmake`, 최상위 `CMakeLists.txt`), `include/kz/schema` 는 libkz-core 의 identity(`KZ_CORE_SOURCE_PATHS`)에 들어가며 `KZ_SCHEMA_REVISION` 과 모든 레코드·열거형·섹션·형식이 `layout_text()` 에 들어가 schema fingerprint 가 된다(`kairos/lib/core/dump.cpp` `layout_text`). 그래서 이 개정은 도구 넷의 정체성을 바꾸고, 뒤 작업의 공개 구조를 여기 모아 `include/` 를 한 번만 바꾼다. 같은 꼴의 앞선 예: 개정 6 의 NOWNER("레코드가 있을 때만 쓰는" 새 섹션; `kairos/include/kz/schema/kz_neutral.h` 의 NOWNER 주석과 `kairos/lib/trace/neutral.cpp` `add_neutral_sections`, `plan/c5_execution_basis_20261028.md` 의 2026-10-29 메모).
- **코드**(`kairos/`): `include/kz/schema/kz_schema.h`(개정 번호와 그 글, 범위 주석), `kz_facts.h`(섹션 id·목록, 레코드 여덟, 열거형 29, 형식), `kz_neutral.h`(같은 꼴, 레코드 일곱, 열거형 21), `include/kz/trace/neutral.hpp`, 새 `include/kz/trace/run_analysis.hpp`, `lib/trace/neutral.cpp`, `tools/kz/main.cpp`, `tools/kz-phi/main.cpp`, `tools/kz-dump/main.cpp`, `tools/kz-static/main.cpp`; 시험 `tests/unit/test_core_schema.cpp`(크기 표 15 줄), `tests/unit/test_core_kzdump.cpp`(두 줄 확인), 새 `tests/unit/check_tool_schema_revision.cmake` 와 `tests/unit/CMakeLists.txt` 의 시험 `tool_identity_schema_revision`, 새 `tests/cfs-free/test_trace_neutral_rev12.cpp`; 문서 `docs/schema.md` §20.
  - 하지 않는 것: 새 섹션을 쓰는 코드(어느 추출·변환도 쓰지 않는다), 추출(`lib/llvm` 전체), relations·order·계획 생성기, `runtime/`, `instrument/`, `cmake/`, 최상위 `CMakeLists.txt`, `tests/lint/`, 판정 규칙의 어휘와 어휘 자료, 설명 스크립트(`env/obs1/explain.py`), 2a 드라이버.
- **도구 해시와 정체성**:
  - 바뀌는 것: kz-static, kz, kz-dump, kz-phi 의 파일 sha256, `--identity` 의 `tool_sha256`, `tool_source`, `core_source`, schema fingerprint(`6d1471a2…` → 다른 값; scratchpad 의 점검 빌드에서 `6fbff6c6c6044d07158109057b05cbcd358bd319e79219f259cfac656b3c8ebd` [I, 근거 아님]). 빌드 전 값: kz-static `b52db538769e6bc8fecd9a7e3daec63fd785885c1873629c0dac244afa50b2f9`, kz `931dd39c6bdbe12a3b38750a0350d8f1aac620dc3de8bf60bb0a72e272ba2248`, kz-dump `05623054296bbb993a66308c9fd8e710873f064e7672318e495eabba74fbaf43`, kz-phi `7d34e6e20934c62b91f006c20712b7a096692b231d318257b096eb22dc68075f`.
  - 다시 링크되는 것: 링크 줄에 `libkz-core.a` 가 있는 75 개 모두(`relink_expected.sha256`, 빌드 전 sha256; 도구 넷과 시험 프로그램 71)와 다시 묶이는 정적 라이브러리 여덟(`libkz-core.a`, `libkz-llvm.a`, `libkz-predict.a`, `libkz-trace.a`, `libkz-anchor.a`, `libkz-loader.a`, `libkz-plan.a`, `libkz-testsupport.a`; 그 목적 파일이 바뀐 머리말을 읽는다).
  - 2026-11-16 검토 뒤 정정: §6.7 `builds-5`. 위의 “도구 넷과 시험 프로그램 71”은 틀렸다: `relink_expected.sha256` 의 75 는 `libkz-core.a`(1 줄), 시험 프로그램 70, 도구 넷이다. `libkz-core.a` 는 다음의 정적 라이브러리 여덟에도 들어 두 번 셌다.
  - 다시 링크되지 않아야 하는 것 29 개(`not_relink.sha256`, 빌드 전 sha256): kz-rt-dump `4390d4f1…`, kz_lint `5f62ec20…`, kz-instr-clang `28db4017…`, 런타임 라이브러리 다섯과 그 시험 프로그램 열일곱, `libkz-hold.a`, 시험 대상 프로그램 셋. 빌드 뒤 sha256 이 같아야 한다.
  - `CMakeCache.txt` `bc2f43b4…` 는 그대로 [I](새 시험 파일이 glob 으로 잡혀 다시 구성된다). flags.make 목록 해시와 도구 넷의 `KZ_TOOL_SOURCE_SHA256` 은 바뀐다.
- **빌드**(MC 제자리, 목표별, 각각 공간 감시 아래, kind 를 따로 적는다): `make kz`(kind kz, 이 dip 이 뒤 kz 빌드의 선언 필요량이 된다), `make kz-dump`, `make kz-phi`, `make kz-static`, 그리고 `make all`(시험과 나머지). 감시가 도중에 멈추면 MC 는 개정 11·12 가 섞인 도구 모음이다. 그것을 기록하고, 모든 도구가 `--identity` 로 개정 12 를 적을 때까지(`identity_all.sh`) 어떤 출력도 내지 않으며, 필요하면 사본을 되돌린다.
- **도구 보존.** 덮이는 도구 넷 가운데 사본이 없는 셋(kz `931dd39c`, kz-phi `7d34e6e2`, kz-static `b52db538`)을 빌드 전에 `/home/user/work/preserve/tools-20261116-e/` 에 xz 로 사본하고 왕복 확인한다(`xz -t`, 스트림 sha256, 제자리 파일과 `cmp`; `tool_copy/ROUNDTRIP.txt`), 공간 감시 아래. kz-dump `05623054` 는 이미 사본이 있다(`/home/user/work/preserve/tools-20261109/kz-dump.05623054.xz`, 등록부 in-place+copy): 빌드 전에 그 왕복을 다시 확인한다. 새 도구는 옛 도구의 출력을 도구 정체성 줄 밖에서 같게 다시 내지만 정체성 줄이 다르므로 넷 모두 "대체됨"이 아니라 사본으로 남긴다. 등록부: kz·kz-phi·kz-static in-place → copy, kz-dump in-place+copy → copy, 새 도구 넷 in-place. 장부: 사본 디렉터리 행 `TOOLS_20261116e`.
- **golden**: `tests/golden/data/layout.golden`(첫 줄 revision 과 마지막 줄 fingerprint 가 바뀌고, 새 줄 644 가 더해지며, 옛 줄은 순서대로 그대로), `records.golden`(새 레코드 30 줄, 패턴으로 채운 참조가 가리키는 섹션이 섹션 목록이 늘어난 만큼 밀린 26 줄)만 바뀐다 [I: scratchpad 점검 빌드에서 잼, 근거 아님]. 갱신 전 첫 ctest 의 실패는 `golden_layout`, `golden_records` 둘이어야 한다. 갱신한 둘은 읽어 보고, records 의 바뀐 줄이 참조의 섹션(이름과 번호)만 다른지 확인한다. 그 밖의 golden(`container_*`, `render`, `tool_hash`, `tests/static/data/*.golden`, `tests/cfs-free/golden`, `tests/phi/data`)은 바뀌지 않는다.
- **시험**: 전체 ctest 250 → 252(새: `tool_identity_schema_revision`, `test_trace_neutral_rev12`). golden 갱신 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). `test_trace_neutral_rev12` 113 확인 [I], `test_static_cli` 911, `test_static_reasons` 655, `test_static_plan` 182, lint 7/7, `lint_selftest` 109(바뀌지 않음). 단위 점검 `env/tools/tests/run_unit.sh` 15/15, `env/redesign/step2a_tests_unit.sh` 7/7. `identity_all.sh MC`: 도구 넷이 같은 개정 12 와 같은 fingerprint.
- **읽기 시험(보존 컨테이너, 종류마다)**: `readback.sh` 가 컨테이너 열(T8CF c1_pre_tx 의 facts·basis, T8CF c5_641 facts, W6 G-I run.kzt, W6 F0 변환 run.kzt, F0 원 기록 `env/logs/s1r2_p1.kzt`, C5 641 run.kzt, W6 replay 컨테이너, C5 641 replay 컨테이너, 관측 주기 C1 부팅의 run.kzt `faa410e1`)마다 파일 sha256, `--check` rc, `--hash`, 정규 dump 텍스트의 sha256·줄 수·바이트 수를 적는다. 빌드 전 kz-dump `05623054` 의 결과(`readback_pre.txt`, 이 선언과 함께 커밋)와 새 kz-dump 의 결과(`readback_post.txt`)가 첫 줄(도구 줄) 밖에서 바이트 동일해야 한다. dump 텍스트는 저장하지 않는다(sha256 으로 비교; C5 641 과 C1 부팅의 dump sha256 은 보존된 `run.kzt.txt` 의 것과 같다).
- **layout**: `kz-dump --layout` 의 옛 판(빌드 전)과 새 판을 `layout_check.py` 로: 첫 줄 revision 11 → 12, fingerprint 줄만 바뀌고, 옛 줄 3,070 이 모두 순서대로 남고, 새 줄은 더한 것뿐(새 섹션 15).
- **옛·새 kz**(`env/c5/regress_eid1.sh <옛> <새> schema12_regress`; 옛 = 사본에서 푼 kz `931dd39c` 와 kz-dump `05623054`): R2 F0 기록의 replay·convert, 런타임 시험 dump 셋의 convert, W6 G-I dump 아홉의 convert 와 replay. 모든 비교에서 "with the identity hash lines masked: 0 differ" 여야 한다. 바이트 차이: convert 컨테이너는 머리의 tool hash 와 header crc(121–156), replay 컨테이너는 그에 더해 머리의 해시와 부모·REF_LISTS 의 부모 해시뿐이어야 한다. 새 컨테이너에 NOWNER 와 개정 12 섹션이 없어야 한다(곁 표를 채우는 코드가 없다).
- **C1 부팅 dump 의 재변환**(`convert_c1boot.sh` 새 kz, `schema12_c1boot_convert`): 보존된 `c1pre_convert` 와 dump 텍스트·convert 로그가 tool hash 줄(머리의 `tool_sha256=`, `artifact_sha256=`, 로그의 `wrote … artifact_sha256`) 밖에서 같고, 바이트는 121–156 만 다르다 [I: 빌드 전 kz `931dd39c` 로 돌리면 `faa410e1` 과 바이트 동일, scratchpad 점검 빌드로는 121–156 만 다름].
- **relations 다섯 빌드**(`rerun.sh`): `relations.txt`·`keys.txt` 가 `orderrev1b_relcheck` 와 `relrev7_check` 의 것과 바이트 동일, `relations.log` 빈 파일.
- **order 다섯 빌드**(`order.sh`): 두 번 돌려 같은 바이트, `order_rev1b` 의 `order.txt`·`edges.tsv` 와 바이트 동일, `order.log` 빈 파일.
- **다시 뽑기 다섯 빌드**(`extract.sh`, `env/redesign/one_build.sh`): `check_extract.py` 가 T8CF `<b>/NP` 와 레코드 단위로 본다. 허용: basis.kz·facts.kz 머리의 `tool_sha256`·`artifact_sha256`·`basis_sha256`·facts 의 `parent[0]`, 같은 count 의 STRINGS 섹션 줄 crc32, BASIS `extract.build_identity`·`front_end.build_identity` 의 `schema=`·`source=` 값, facts BASIS `extract.units_from` 의 basis 해시; basis.txt 의 `basis <hash>` 줄과 `front_end.build_identity` 줄(같은 두 값); extract.txt 1 줄; sha256.txt 의 basis.kz·facts.kz·kz-static 줄. 그 밖의 줄은 모두 같고 줄 수도 같아야 한다. 파일 단위로는 `compare_build.py` 와 `extract_allow.tsv`(앞 단계의 관행). 다시 뽑기의 `relations.txt` 는 `schema12_relcheck` 의 것과 둘째 줄 밖에서 같고, 다시 뽑은 facts 의 order(`schema12_on_extract`)는 `schema12_order` 의 것과 `order.txt` 둘째 줄 밖에서 같으며 `edges.tsv` 는 바이트 동일해야 한다.
- **계획 다섯 빌드**(`plans.sh`): `compare_plans.py order_rev1b_plans/<b> schema12_plans/<b>` 로, sidecar 둘째 줄(생성기의 `tool_sha256`, `tool_source`, `core_source`), manifest 둘째 줄(sidecar sha256), module 표와 실행 파일 목록의 첫 줄, `args.txt` 의 출력 디렉터리 밖에서 같아야 한다.
- **누적 바이트 동일성**: 빌드 뒤 `env/tools/ledger_check.sh` 전체(빌드 전 59 행)와 `env/tools/tools_check.sh --deep`(빌드 전 34 행), 모두 실패 0. 새 출력은 장부 행으로 더한다.
- **공간**: 각 빌드와 사본은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 필요량 = 앞 kz-static 빌드 dip 의 최대 219,463,680 + 새 출력 약 160 MB(다시 뽑기 다섯 약 47 MB, order 둘과 둘째 실행 약 21 MB, relations 약 8 MB, 계획 약 1 MB, 회귀 약 10 MB, 여유) + 한 시간 흐름 13 MB = 392,463,680 B(감시 문턱 707,036,480). kz·kz-dump·kz-phi 의 dip 은 아직 재지 않았다 [미확인]; 같은 필요량을 쓴다. 사본은 필요량 150,000,000.
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `readback.sh`, `layout_check.py`, `identity_all.sh`, `convert_c1boot.sh`, `check_extract.py`, `compare.sh`, `compare_plans.py`, `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `extract_allow.tsv`; 목록 `relink_expected.sha256`, `not_relink.sha256`, `readback_pre.txt`.

### 4.1 바뀐 것 [F]

- 커밋 `2f24260`(선언, 스키마 머리말, 공개 구조, `--identity` 줄, 시험, 문서, 비교 스크립트, 빌드 전 읽기 표를 함께, 빌드 전). 규모: `kz_facts.h` 레코드 여덟·열거형 29·형식 61 줄, `kz_neutral.h` 레코드 일곱·열거형 21·형식 54 줄, `neutral.hpp` 구조 일곱과 기본값 함수 여섯, `neutral.cpp` +249 줄(기본값·검사·읽기·쓰기), 새 `run_analysis.hpp` 105 줄, 도구 넷의 `main.cpp`, 새 시험 `test_trace_neutral_rev12.cpp` 444 줄, `check_tool_schema_revision.cmake`, `docs/schema.md` §20.
- 빌드 뒤: `tests/golden/data/layout.golden`, `records.golden` 갱신, 이 절과 기록, 장부·등록부 행(커밋 `2d5f0e7`). 그 사이 12:54Z 에 스냅숏 커밋 `e6f0135` 이 자동으로 만들어졌다(내 명령이 아님; 이 기록 디렉터리의 파일과 갱신한 golden 둘만 담음).

### 4.2 빌드 [M]

목표별로 MC 에서 제자리, 각각 공간 감시 아래, 모두 첫 시도에 rc 0.

| 목표 | 때 (UTC) | dip (B) | 링크 줄 |
|---|---|---|---|
| `kz` | 12:40:10–12:41:30 | 36,175,872 | 7 (`libkz-core.a`, `libkz-anchor.a`, `libkz-predict.a`, `libkz-trace.a`, `libkz-plan.a`, `libkz-loader.a`, kz) |
| `kz-dump` | 12:41:35–12:41:37 | 106,496 | 1 |
| `kz-phi` | 12:41:37–12:41:43 | 1,556,480 | 1 |
| `kz-static` | 12:41:48–12:43:40 | 75,079,680 | 2 (`libkz-llvm.a`, kz-static) |
| `all` | 12:43:45–12:46:56 | 48,418,816 | 72 (시험 프로그램 71 과 `libkz-testsupport.a`) |

- 링크된 것 83 은 선언의 목록(`relink_expected.sha256` 75 와 정적 라이브러리 여덟)에 새 시험 `test_trace_neutral_rev12` 하나를 더한 것과 같다(이름 집합 비교). 75 가운데 68 은 sha256 이 바뀌었고 7 은 다시 링크되었으나 바이트가 같다(`test_hold_plan`, `test_hold_ptrace`, `test_hold_sequence`, `test_rt_plan_convert`, `test_core_crc32`, `test_core_phi_parser`, `test_core_sha256`: libkz-core 에서 끌어오는 목적 파일이 바뀌지 않음). 선언은 "다시 링크된다(sha256 이 바뀔 수 있다)" 였으므로 선언 안이다.
- 2026-11-16 검토 뒤 정정: §6.7 `builds-5`. 위 셈은 84 가 되어 `libkz-core.a` 를 두 번 센다. 링크된 83 은 `relink_expected.sha256` 의 75(도구 넷, 시험 70, `libkz-core.a`), 그 밖의 정적 라이브러리 일곱, 새 시험 `test_trace_neutral_rev12` 다. 표의 `all` 줄 “시험 프로그램 71”은 70 과 새 시험이라 맞다.
- 다시 링크되지 않아야 하는 29 는 모두 sha256 이 같다(`sha256sum -c not_relink.sha256` 29 OK): kz-rt-dump `4390d4f1…`, kz_lint `5f62ec20…`, kz-instr-clang `28db4017…`, 런타임 라이브러리와 그 시험, `libkz-hold.a`, 시험 대상 프로그램 셋.
- 도구(`post_mc.txt`): kz-static `b52db538…` → `5ae220d2a0963097611688a5e55f77be34bebc0c40a1d98a6581af70ac3fc3cf`, kz `931dd39c…` → `913fcd99064b5aeca3b2c52da6f6f9e72584680ea783e6edb2f959eeb22eccfa`, kz-dump `05623054…` → `32a96ae7138fd71c9efd63d0439021cae044c4045b6a9557b659e1e1911c0438`, kz-phi `7d34e6e2…` → `df79fb82f962e69fdd29493cf9fd664feae38e370aa3578697227109cf1d59e2`.
- `--identity`(`identity_all.txt`): 넷 모두 `schema_revision=12`, `schema_fingerprint=6fbff6c6c6044d07158109057b05cbcd358bd319e79219f259cfac656b3c8ebd`(옛 `6d1471a2…`), `core_source=568e5061e6092557602716e123e6ad63e124336904dcbf34d94eaba432119920`(옛 `f6e3c7b4…`). tool_sha256 / tool_source: kz-static `783c7cd7…` / `f73da08c…`, kz `c19937fd…` / `fb7f09f9…`, kz-dump `c5e70435…` / `21cc60f2…`, kz-phi `7792f19b…` / `4a234a89…`. kz 의 tool hash 는 scratchpad 점검 빌드의 것과 같다(같은 소스, 다른 트리 위치).
- `CMakeCache.txt` `bc2f43b4…` 그대로, flags.make 목록 해시 `321b751c…` → `e8f06cbe…`. 빌드 시작 뒤 바뀐 소스 파일(`find -newer`, kairos 의 lib·tools·include·cmake·probe·instrument·runtime·tests) 0.
- 혼합 상태는 생기지 않았다: 모든 목표가 감시 안에서 끝났고, 출력은 `identity_all.sh` 가 넷 모두 개정 12 를 적은 뒤에만 냈다.

### 4.3 시험 [M]

- 첫 ctest(`ctest_first.log`): 252 개 가운데 실패 2, 선언한 `golden_layout`, `golden_records` 그대로.
- golden 갱신(`golden_update.log`): 두 파일이 scratchpad 점검 빌드로 미리 만든 것과 `cmp` 같다. `layout.golden` 3,715 줄(옛 3,070 줄이 순서대로 남고 644 줄이 더해짐, revision·fingerprint 두 줄 바뀜), `records.golden` 218 줄(새 레코드 30 줄, 바뀐 26 줄은 참조가 가리키는 섹션의 이름·번호만 다름: 섹션 참조를 가린 비교에서 다른 줄 0).
- 갱신 뒤(`ctest_full.log`): 252 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). 라벨 golden 62, static 55, unit 86, lint 7, runtime 66.
- 확인 수(`ctest_counts.log`): `test_trace_neutral_rev12` 113, `test_core_schema` 34,017, `test_core_kzdump` 35, `test_static_cli` 911, `test_static_reasons` 655, `test_static_plan` 182, `lint_selftest` 109 경우 실패 0, lint 7/7, `tool_identity_schema_revision` "4 tools report schema revision 12". 단위 점검 15/15(`unit_tools.txt`), 7/7(`unit_step2a.txt`).

### 4.4 바이트 동일성과 읽기 시험 [M]

| 비교 | 결과 |
|---|---|
| 보존 컨테이너 열의 읽기(`readback_pre.txt` kz-dump `05623054` vs `readback_post.txt` 새 kz-dump) | 첫 줄 밖 바이트 동일(`readback_compare.txt` 빈 파일). 열 모두 `--check` rc 0, dump rc 0, 같은 artifact hash 와 같은 dump 텍스트 sha256·줄 수 |
| `kz-dump --layout` 옛·새(`layout_check.txt`) | 옛 줄 3,070 이 모두 순서대로 남음, 더한 줄 644(record 15, field 205, enum·flags 50, item 244, section 15, format 115), revision 11 → 12, fingerprint `6d1471a2…` → `6fbff6c6…` |
| 옛·새 kz(`regress_eid1.record`, 옛 = 사본에서 푼 kz `931dd39c`, kz-dump `05623054`) | 텍스트 비교 47 모두 "identity hash lines masked: 0 differ". convert 컨테이너(R2 F0 기록, 런타임 시험 dump 셋, W6 G-I 아홉)는 바이트 121–156(머리의 tool hash 와 header crc)만, replay 컨테이너는 그에 더해 머리의 해시와 REF_LISTS 의 부모 해시만 다름. 새 컨테이너에 NOWNER·개정 12 섹션 없음 |
| W6 G-I 컨테이너 vs 옛 kz 가 같은 dump 로 만든 것(`w6_vs_old.txt`) | 아홉 모두 121–156 만 다름(W6 의 kz 와 `931dd39c` 의 tool hash). 회귀 기록의 "the old tool's container is the W6 one: NO" 는 이 tool hash 차이다 |
| C1 부팅 dump 의 재변환(`convert_c1boot.txt`) | dump 텍스트 3,823 줄과 convert 로그가 tool hash 줄 밖에서 같음, `run.kzt` 는 121–156(36 B)만 다름, 개정 12 섹션 없음 |
| relations 다섯 빌드 vs `orderrev1b_relcheck`, `relrev7_check` (`compare_relations.txt`) | 바이트 동일, `relations.log` 빈 파일(25 OK) |
| order 다섯 빌드 두 번, vs `order_rev1b` (`compare_order.txt`) | 같은 바이트, `order_rev1b` 와 바이트 동일, `order.log` 빈 파일(25 OK) |
| 다시 뽑기 다섯 빌드 vs T8CF (`compare_extract.txt`, `compare/check_extract_<b>.txt`, `compare/extract_<b>.{tsv,log}`) | `check_extract.py`: 다섯 모두 선언한 줄만 다름. 예 c1_pre_tx: facts.kz dump 161,911 줄 가운데 머리 4, STRINGS crc32 1, `extract.build_identity` 1, `extract.units_from` 1; basis.kz 518 줄 가운데 머리 2, STRINGS crc32 1, `front_end.build_identity` 1; basis.txt 의 `basis` 줄과 `front_end.build_identity` 줄; extract.txt 1 줄; sha256.txt 세 줄. `compare_build.py` 허용 밖 0. 다시 뽑기의 relations.txt 는 relations 재실행과 둘째 줄 밖 같음, 다시 뽑은 facts 의 order 는 둘째 줄 밖 같고 edges.tsv 는 같음(25 OK) |
| 계획 다섯 빌드 vs `order_rev1b_plans` (`compare_plans.txt`) | 다섯 모두 선언한 줄만(sidecar 둘째 줄, manifest 둘째 줄, module 표·실행 파일 목록 첫 줄, args.txt 의 출력 디렉터리) |

- **누적 목록**: `ledger_check.sh` 68 행, 실패 0(새 행 아홉: `OBS_schema12_readback`, `OBS_schema12_regress`, `OBS_schema12_c1boot_convert`, `OBS_schema12_relcheck`, `OBS_schema12_order`, `OBS_schema12_extract`, `OBS_schema12_onextract`, `OBS_schema12_plans`, `TOOLS_20261116e`). `tools_check.sh --deep` 38 행, 실패 0. 바깥 이름 점검(`--tree`)은 여덟 출력 디렉터리, 사본 디렉터리, 기록 디렉터리에서 깨끗하다.
- **도구 보존**(`tool_copy/ROUNDTRIP.txt`): kz `931dd39c`(17,478,556 B), kz-phi `7d34e6e2`(6,666,968 B), kz-static `b52db538`(42,449,724 B)의 사본을 빌드 전에 `/home/user/work/preserve/tools-20261116-e/` 에 만들고 왕복 확인했다(`xz -t` OK, 스트림 sha256 같음, `cmp` 같음). kz-dump `05623054` 의 기존 사본(`tools-20261109/kz-dump.05623054.xz`)의 왕복도 다시 확인했다. 등록부: 넷 모두 copy(대체되지 않음: 출력의 도구 정체성 줄이 다르다; kz-static 의 relations·키 목록·order 는 새 도구가 바이트 동일하게 다시 냈고, kz 의 변환은 tool hash 줄 밖에서 같다), 새 도구 넷 in-place. 회귀의 옛 쪽은 이 사본에서 푼 바이너리로 돌렸다(sha256 `931dd39c…`, `05623054…`).

### 4.5 측정 [M]

- 새 섹션의 크기와 수는 선언대로다(§4.0; `layout_check.txt`, `test_trace_neutral_rev12` 의 섹션 줄 확인). 실제 빌드의 어느 컨테이너에도 새 섹션은 없다(다시 뽑기 다섯, 변환 열하나, replay 열).
- kz 의 dip 36,175,872 B 는 뒤 kz 빌드(manifest 개정 4 이후)의 선언 필요량의 실측 값이다. 이번 목표별 dip 의 최대는 kz-static 75,079,680 B 다.

### 4.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 이 단계 처음 | 3,356,196,864 | 시작 |
| scratchpad 점검 빌드 뒤 | 1,701,556,224 | 감시 없음(§4.7), 빌드 중 최저는 재지 않음 |
| 점검 빌드 지운 뒤 | 3,352,174,592 | |
| 12:39:51 | 3,284,942,848 | 사본 셋(dip 66,625,536) |
| 12:46:56 | 3,166,244,864 | 목표별 빌드 다섯 끝 |
| 12:54 | 3,079,839,744 | 회귀(dip 12,926,976), C1 재변환, relations, order, 다시 뽑기(47,058,944), 그 order, 계획 뒤 |

- 하한을 넘지 않았다. 남긴 출력 84,197,659 B(여덟 디렉터리, 장부 행), 사본 셋 66,595,248 B. 지운 것: compare.sh 가 비교 뒤 지운 둘째 실행(`again/`), 그리고 이 단계 끝에 scratchpad 의 이 단계 작업 파일(점검 빌드, 옛 도구를 푼 바이너리, golden 사본, layout 사본)뿐이다.

### 4.7 계획과 다르게 한 것

- **scratchpad 점검 빌드를 감시 없이 돌렸다.** 선언 전에 LLVM 없는 구성으로 트리 전체를 scratchpad 에 빌드해 시험했다(1,642,433,500 B). 그때 여유는 빌드 뒤 1,701,556,224 B 였고 빌드 중의 최저는 재지 않았다. 증거로 쓰지 않았고 지웠다. 다음부터 점검 빌드도 감시 아래 둔다.
- **kz-dump 는 새로 사본하지 않았다.** 이미 검증된 사본(PV)이 있어 그 왕복을 다시 확인했다.
- **읽기 시험을 넓혔다.** 계획의 다섯 종류(T8CF facts, W6 run.kzt, C5 641 run.kzt, replay, C1 부팅 run.kzt)에 basis, F0 원 기록, F0 변환 기록, 둘째 replay, C5 facts 를 더해 열이다. 그리고 dump 로만 읽지 않고 kz 의 convert·replay(옛·새)와 kz-static 의 relations·order·계획·다시 뽑기로도 읽었다.
- **공개 구조의 범위.** 계획은 "이 단계가 뒤에 더할 모든 공개 구조"를 이 `include/` 바뀜에 둔다고 했다. 정의한 것: 추출 섹션 여덟(객체 접근·형, 의미 표, 결과 흐름·효과, 루프 정의, 핸들 잠금 정체성), 중립 곁 표 일곱(토큰과 actor 끝·root, 수명, 같은 빌드 판정, 충분성, 반환 없는 호출, 순서 인스턴스, 관측 순서), 그 입력·결과 구조. 정의하지 않은 것: 런타임의 새 기록(SPAWN, SPAWN_RET, TBEGIN, AEXIT 의 끝 종류)과 manifest 개정 4 의 구조. 그것들은 `runtime/` 에 있어 `include/` 가 아니고(`runtime/include/kz_rt_format.h`, `runtime/tools/manifest`), 그 바뀜은 어차피 kz 의 정체성을 바꾼다(`lib/trace/CMakeLists.txt` 의 kzrt-decode·kzrt-manifest 링크). 함수 선언은 두지 않았다: 채우는 함수는 뒤 작업이 `lib/trace` 안에 둔다. 레이아웃이 뒤 작업의 쓰기에 맞는지는 쓰는 코드가 생길 때까지 [I] 다. 맞지 않으면 그때 선언한 `include/` 바뀜이 하나 더 생긴다.
- **core_source 도 바뀐다.** 계획의 정체성 규칙 글은 도구 넷의 tool identity 를 말한다. `include/kz/schema` 는 libkz-core 의 identity 에도 들어가므로 넷의 `core_source` 가 함께 바뀌었다(선언에 적음).
- **W6 컨테이너의 재현.** 옛 kz `931dd39c` 는 W6 의 G-I 컨테이너를 tool hash(121–156) 밖에서 바이트 동일하게 다시 낸다. W6 를 만든 kz 와 같은 바이트는 아니다(회귀 기록의 `NO`).

### 4.8 열린 것

- 새 섹션을 쓰는 코드는 없다. 각 레이아웃이 쓰는 쪽에 맞는지는 객체 접근·형(뒤 추출 단계), 의미 표, 결과 흐름·효과, 루프 정의와 잠금 정체성, 변환의 토큰 결합·같은 빌드 판정, 기록 분석(충분성, 수명, 순서 인스턴스)이 생길 때 잰다.
- 런타임의 생성 토큰 기록과 manifest 개정 4 는 `runtime/` 의 바뀜이다(kz, kz-rt-dump, kz-hold, kz-instr-clang 과 시험의 재링크).
- kz 의 dip 36,175,872 B 를 뒤 kz 빌드의 필요량에 쓴다.

## 5. 추출의 기본 경로 고침: 읽기 옵션의 대조, 잘린 이미지의 소비자 둘, 이름 없는 레코드 형의 이름 (kz-static 빌드 하나)

기록: `../records/extract-fixes-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `extfix_pre_onextract`(빌드 전, 옛 도구의 참조), `extfix_relcheck`, `extfix_order`, `extfix_extract`, `extfix_on_extract`, `extfix_plans`, `extfix_plans_on_extract`. 도구 사본: 새 디렉터리 `/home/user/work/preserve/tools-20261116-f/`. T8CF 와 앞선 출력 디렉터리에는 쓰지 않는다. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 5.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다(§7.12 의 1 의 규칙).

- **무엇을 하는가.** 세 고침과 relations 규칙 개정 8(`kairos/docs/static_facts.md` §5q).
  1. **읽기 옵션의 대조.** extract 가 제 읽기 옵션(단위마다 주는 인자, `-resource-dir`, `--extra-arg`)과 프런트엔드를 basis 의 `front_end.arguments_added`·`front_end` 와 대조한다. 같으면 facts BASIS `extract.reading_options_vs_basis = same as basis front_end.arguments_added and front_end`(정책 unset), basis 가 없으면(`--source`) `not compared: no basis (--source)`, 다르면 거부(종료 코드 1, facts 없음); `--accept-option-difference` 이면 추출하고 `differs (--accept-option-difference): …` 를 정책 deviation 으로 적는다. 두 글은 한 함수가 만든다(basis 와 extract 가 같은 함수). relations 는 그 항목이 `differs` 이면 모든 행에 `reading_options_differ(extraction options differ from the basis)` 와 layers 의 `reading-options-differ(…)` 를 붙이고, 머리에 그 항목을 한 줄 적는다(있을 때).
  2. **잘린 이미지의 소비자 둘.** 펼치기: 원소 이미지에 표시 잎이 있고 찾는 필드가 없으면 인자·callee 의 글이 그 까닭을 적고, guard 가 그런 필드를 읽어 residual 인 호출 행은 행의 text 칸에 까닭을 둔다(`kz-static show` 가 그 칸을 괄호로 보인다; 앞판은 그 칸이 늘 비었다). PTR_FIELDS: 읽기 뿌리 객체의 이미지에 표시 잎이 있으면 `resolved` 대신 `may`.
  3. **이름 없는 레코드 형의 이름.** 파일 범위 태그·typedef 가 없는 레코드에 쓰임에서 온 이름(레코드 안의 태그, 블록 안의 태그·typedef `이름@파일:줄`, 멤버의 형 `<바깥>::<멤버>`, 객체의 형 `<객체 키>::type`, 그 밖 `<anonymous@파일:줄:열>`; 위치의 `.` 는 `_`)을 주고, 그 이름을 이름 표에 넣는다. C11 이름 없는 멤버는 규칙 그대로(감싼 레코드의 필드). 이름 있는 멤버의 형인 태그 없는 레코드는 이제 제 형이다(앞판은 감싼 레코드의 필드로도 맞춤). unknown-target 글은 "… among the record names of the units read".
- **1차 근거.**
  - 읽기 옵션: 앞판은 extract 의 옵션을 적기만 했다(`kairos/docs/static_facts.md` §5k "읽기 옵션의 기록", `tools/kz-static/cmd_extract.cpp` 의 `extract.resource_dir`·`extract.extra_arg` 항목). basis 의 글은 `lib/llvm/basis.cpp` `Builder::front_end` 가, 프런트엔드가 받는 인자는 `lib/llvm/ast_run.cpp` `adaptations` 가 따로 만들었다. 이제 둘 다 `reading_arguments_text`(`ast_run.cpp:155`)를 쓰고(`basis.cpp:406`, `:415`), 대조는 `cmd_extract.cpp:316` 부터다. 판정 규칙 어휘의 `reading_options_differ`(`plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` 3.5, 2026-11-16 덧붙임)가 이 경우의 사유다; 새 글은 그 아래의 세부 사유다(`lib/llvm/relations.cpp` 의 사유 표, `kReadingOptions`, `:510`, `:588`).
  - 잘린 이미지: §5j 의 "펼치기(`unroll.cpp`), PTR_FIELDS 정적 초기값(`argflow.cpp`) … 고치지 않았다". 고친 곳 `lib/llvm/unroll.cpp:334-353`, `lib/llvm/argflow.cpp:511`, `:601`.
  - 이름: 앞판 `TuReader::record_name` 은 이름이 없으면 `<anonymous>` 를 주고 이름 표는 파일 범위 선언만 담았다(§5k "규칙 대상" 의 결함, T8CF 의 `<anonymous>` FACT_LOCS 행 3/3/2/0/0). 고친 곳 `lib/llvm/ast_tu.cpp:112`(record_name), `:163`(name_unnamed_records), `:301`(targets_of_field 가 올라가는 조건). 형 이름에 점이 없어야 하는 까닭: `lib/llvm/spec.cpp:10` `parse_field_ref` 가 `Type.field` 를 첫 점에서 나누고 둘째 점을 거부한다.
- **코드**(`kairos/`): `lib/llvm/{ast_run.cpp,ast_basis.cpp,basis.cpp,extract.hpp,serialize.hpp,serialize.cpp,unroll.cpp,argflow.cpp,model.hpp,facts_text.cpp,ast_tu.cpp,ast_reader.hpp,finalize.cpp,relations.cpp}`, `tools/kz-static/{cmd_extract.cpp,main.cpp}`; 시험 `tests/static/test_static_cli.cpp`(Inner.ix 의 확인 바뀜, 개정 8 의 세 줄, 새 확인: 읽기 옵션 넷, C11 과 멤버 형, 잘린 원소·뿌리 이미지), 새 fixture `tests/static/fixtures/anonrec.c` 와 golden 사례 `anonrec_np`(`tests/static/static_golden.cpp`, `tests/static/CMakeLists.txt`); 문서 `docs/static_facts.md` §5q(와 §2, §5j, §5k 의 뒤 참조), `docs/schema.md` §19 끝의 한 항목.
  - 하지 않는 것: `include/`, `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `instrument/`, `tests/lint/`, schema 개정 번호(12 그대로), 판정 규칙의 어휘 자료(`tests/static/data/reason_vocabulary.txt` 그대로: 새 것은 세부 사유), order 규칙, 계획 생성기의 규칙, 설명 스크립트, 2a 드라이버와 `one_build.sh`.
- **도구 해시와 정체성**:
  - 바뀌는 것: kz-static 의 파일 sha256(빌드 전 `5ae220d2a0963097611688a5e55f77be34bebc0c40a1d98a6581af70ac3fc3cf`), `--identity` 의 `tool_sha256`(빌드 전 `783c7cd7…`)·`tool_source`(빌드 전 `f73da08c…`). `core_source`(`568e5061…`), `schema_revision=12`, `schema_fingerprint`(`6fbff6c6…`)는 그대로 [I: `include/` 를 바꾸지 않음, `cmake/KzSourceIdentity.cmake` 의 `kz_tool_identity`].
  - 다시 링크되는 것 14(`relink_expected.sha256`, 빌드 전 sha256): `libkz-llvm.a`, kz-static, `libkz-llvm.a` 를 링크하는 시험 프로그램 열(`test_static_{reasons,plan,unroll,flow,elf32,wrappers,gitrev,link,recipe,expr}`), 소스가 바뀐 `test_static_cli`·`static_golden`. sha256 은 바뀔 수 있다.
  - 다시 링크되지 않아야 하는 것 110(`not_relink.sha256`, 빌드 전 sha256: MC 의 ELF 실행 파일과 정적 라이브러리 가운데 위의 14 밖 모두; kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz_lint `5f62ec20…`, kz-instr-clang `28db4017…`, libkz-core.a 와 그 시험들, 런타임 라이브러리와 그 시험). 빌드 뒤 sha256 이 같아야 한다.
  - `CMakeCache.txt` `bc2f43b4…` 는 그대로 [I](`tests/static/CMakeLists.txt` 가 바뀌어 다시 구성된다). kz-static 의 flags.make(도구 정체성 define)는 바뀐다.
- **빌드**(MC 제자리, 목표별, 각각 공간 감시 아래): `make kz-static`, 그리고 `make all`(시험과 나머지). 감시가 도중에 멈추면 그것을 적고, kz-static 이 새 `--identity` 를 적을 때까지 어떤 출력도 내지 않는다.
- **도구 보존.** 빌드 전에 kz-static `5ae220d2` 를 `/home/user/work/preserve/tools-20261116-f/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`: `xz -t`, 스트림 sha256, 제자리 파일과 `cmp`), 공간 감시 아래. 새 도구의 출력은 옛 도구의 것과 다르므로(사유·이름) "대체됨"이 아니라 사본으로 남긴다. 등록부: `5ae220d2` in-place → copy, 새 kz-static in-place. 장부: 사본 디렉터리 행 `TOOLS_20261116f`.
- **golden**: 바뀌는 것(빌드 전 scratchpad 점검 빌드로 미리 만듦 [I, 근거 아님]): `tests/static/data/` 의 relations 일곱(`relations`, `relations_np`, `relpaths_np`, `relsound_np`, `relincomplete_np`, `relincomplete_cut_np`, `relcallback_np`: 각각 `rules relations rules revision 7` → `8` 한 줄과 머리 줄 `  extract.reading_options_vs_basis = not compared: no basis (--source)` 하나 더), `order_np`(`relations relations rules revision 7` → `8` 한 줄), `imgbudget_np`(네 줄 `field=<anonymous>.a|b|c|d` → `field=ImgUnion::s.a|b|c|d`), `plangen_np.plan.golden`(두 줄: 머리의 relations 개정, `rules` 줄), 새 `anonrec_np.golden`. 갱신 전 첫 ctest 의 실패는 `static_golden_{relations,relations_np,relpaths_np,relsound_np,relincomplete_np,relincomplete_cut_np,relcallback_np,order_np,imgbudget_np,anonrec_np}` 열과 `test_static_plan` 하나여야 한다. 갱신한 것은 scratchpad 점검 빌드의 것과 `cmp` 같아야 한다. 그 밖의 golden(`tests/golden/data`, `tests/static/data` 의 나머지, `tests/cfs-free/golden`, `tests/phi/data`)은 바뀌지 않는다.
- **시험**: 전체 ctest 252 → 254(새: `static_fixture_compile_anonrec`, `static_golden_anonrec_np`). golden 갱신 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). `test_static_cli` 911 → 953 확인 [I], `test_static_reasons` 655 → 더 많음(사유 표의 줄 하나) [I], `test_static_plan` 182, lint 7/7, `lint_selftest` 109. 단위 점검 `env/tools/tests/run_unit.sh` 15/15, `env/redesign/step2a_tests_unit.sh` 7/7.
- **relations 다섯 빌드, T8CF facts**(`rerun.sh` → `extfix_relcheck`): `relations.txt`·`keys.txt` 가 `schema12_relcheck` 의 것과 3 번째 줄(`rules relations rules revision 8 (2026-11-16)`) 밖에서 바이트 동일, `relations.log` 빈 파일. T8CF facts 에는 `extract.reading_options_vs_basis` 가 없어 머리 줄이 더해지지 않는다.
- **order 다섯 빌드, T8CF facts**(`order.sh` → `extfix_order`): 두 번 같은 바이트, `order.txt` 는 `schema12_order` 와 3 번째 줄(`relations relations rules revision 8 (2026-11-16)`) 밖에서 같고, `edges.tsv` 바이트 동일, `order.log` 빈 파일.
- **다시 뽑기 다섯 빌드**(`extract.sh` → `extfix_extract`, `env/redesign/one_build.sh`), `schema12_extract` 와(`compare.sh extract`):
  - `basis.kz`: 레코드 모두 같고 머리의 `tool_sha256`·`artifact_sha256` 만 다름(`check_head.py --whole`). `basis.txt`: `basis <hash>` 줄만. 이것이 basis 의 `front_end.arguments_added` 글이 한 함수로 바뀐 뒤에도 같다는 확인이다.
  - `facts.kz` 머리: `tool_sha256`·`artifact_sha256`·`basis_sha256`·`parent[0]` 만(`check_head.py`).
  - facts 의 섹션: 새 도구의 `kz-static show` 두 판(옛 facts 의 호출 행 text 칸은 비어 보기가 옛 도구와 같다)을 `compare_classes.py` 와 `facts_allow.tsv` 로. 허용 갈래: (가) 이름 없는 레코드 형의 새 이름(`<anonymous>` 자리; 새 이름을 `<anonymous>` 로 쓰면 옛 줄과 같음: `renamed`), 새 이름을 단 새 행(`added-named`: 그 형의 필드에 이제 맞는 writer·A-TYPE·FACT_INIT·FIELD_STATUS·FUNC_WRITES·LOCKSET·LOCK_PATHS·CALL_TARGETS 행, 규칙 대상), `<anonymous>` 행의 사라짐(`removed-anon`: 갈라지거나 다른 이름으로 감); (나) 행이 더해져 움직이는 번호: site 번호, LOCK_PATHS 가 이르는 LOCKSET 행 번호, 섹션 머리의 행 수, "(and N more)"; (다) BASIS: `count.{sites,field_status,lockset,lock_paths,targets_rule,targets_rule_t1,targets_rule_t2,targets_rule_both,targets_rule_t1_unshared_only,targets_rule_t2_unshared_only,indirect_calls_incomplete}` 의 값, `extract.units_from` 의 basis 해시, 새 항목 `extract.reading_options_vs_basis = same as …` 하나; (라) c1_pre_tx·c1_post_tx 의 LOCK_PATHS: `CF_CmdGetSetParam` 의 entry-release 행 가운데 "through the call" 이 `cf_cmd.c:1020:17 CFE_EVS_SendEvent` 에서 `cf_cmd.c:1014:18` 로 바뀐 것(더해짐 8 이하, 빠짐 7 이하). (라)의 까닭: 그 함수의 지역 객체 `item`(태그 없는 struct, `cf_cmd.c:945`)의 memset(`:950`)이 이제 그 형 필드 `fn` 의 writer 로 맞아, `:1014:18` 의 간접 호출 `item.fn(...)` 의 대상이 `incomplete-source` 가 되고, 그래서 잠금을 놓을 수 있는 첫 호출이 그 간접 호출이다 [I: scratchpad 점검 빌드에서 봄, 근거 아님]. 그 밖은 모두 같아야 한다.
  - `extract.txt`: `extract_txt_allow.tsv`: 첫 줄(facts 해시), `rule targets`, `indirect calls`, `untyped write sites`, `unknown-effect passes`, `out-pointer summaries` 줄의 수와 (가)의 `location` 줄.
  - `relations.txt`: `relations_allow.tsv`: facts 해시 줄, rules 줄(7 → 8), 머리 줄 하나(`extract.reading_options_vs_basis = same …`), 행이 인용하는 "풀리지 않은 간접 호출"의 수(`calls whose targets are not resolved (N in these facts`, `creators may be in code not read (N indirect call`), `[B]`·`[C]` 머리의 수, (가)의 새 이름. 행의 수와 판정 칸은 바뀌지 않는다 [I].
  - `sha256.txt`: basis.kz, facts.kz, kz-static 줄의 해시만. `args.txt`, `relations.log` 같음.
  - FACT_LOCS·FIELD_STATUS 의 이름 바뀐 행을 빌드마다 앞뒤로 적는다(`compare/names_<b>.txt`, 측정 표).
- **다시 뽑은 facts 위의 order 와 키**(`order.sh extfix_extract extfix_on_extract`): 두 번 같은 바이트; `order.txt` 는 `schema12_on_extract` 와 `order_allow.tsv`(facts 해시 줄, relations 개정 줄, 레코드 전체 쓰기의 짝 줄이 대괄호에 적는 레코드 형: 그 쓰기가 담은 형 가운데 정렬로 첫 것이라 `<anonymous>` 가 아닌 이름이 앞에 설 수 있다)로; `edges.tsv` 바이트 동일; `relations.txt` 는 다시 뽑기의 것과 바이트 동일. **옛 키 → 새 키 표**: `key_map.py` 가 `extfix_pre_onextract/<b>/keys.txt`(옛 도구, 빌드 전) 와 `extfix_on_extract/<b>/keys.txt` 를 키 글로 잇는다(`key_map_<b>.tsv`; 새 이름만 다른 키 글은 renamed 로 이음). 예상: 다섯 빌드 모두 바뀌는 키 0 [I: scratchpad 점검 빌드; relations 행의 키 글에 이름 없는 형이 들지 않음]. 키가 바뀌면 그 행은 표에 옛 키와 새 키로 남고, 그것이 이 고침의 결과다(선언 안).
- **계획 다섯 빌드**(`plans.sh`): T8CF facts 위(`extfix_plans`)는 `schema12_plans` 와 `compare_plans.py t8cf`(생성기 정체성 줄, relations 개정이 든 두 줄, sidecar sha256 줄, module 표·실행 파일 목록 첫 줄, args.txt 의 출력 디렉터리); 다시 뽑은 facts 위(`extfix_plans_on_extract`)는 빌드 전 옛 도구의 계획(`extfix_pre_onextract/<b>/plan`)과 `compare_plans.py extract`(그에 더해 facts·basis 입력과 artifact 줄, manifest 의 `source build rev=basis-…`, args.txt 의 facts·basis 경로). site 와 필요 줄은 같아야 한다.
- **누적 바이트 동일성**: 빌드 뒤 `env/tools/ledger_check.sh` 전체(빌드 전 68 행, 실패 0)와 `env/tools/tools_check.sh --deep`(빌드 전 38 행, 실패 0). 새 출력은 장부 행으로 더한다.
- **공간**: 각 빌드·사본·실행은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 빌드 필요량 = 앞 kz-static 빌드 dip 의 최대 219,463,680 + 새 출력 약 120 MB(다시 뽑기 약 47 MB, order 셋과 둘째 실행 약 40 MB, relations 약 20 MB, 계획 약 2 MB, 여유) + 한 시간 흐름 13 MB = 352,463,680 B. 사본은 필요량 100,000,000.
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `pre_ref.sh`, `plan_one.sh`, `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `compare.sh`, `compare_classes.py`, `check_head.py`, `compare_plans.py`, `key_map.py`, 허용 파일 `facts_allow.tsv`, `extract_txt_allow.tsv`, `relations_allow.tsv`, `order_allow.tsv`, `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`; 빌드 전 참조 `extfix_pre_onextract.sha256`(옛 도구 `5ae220d2` 가 `schema12_extract` facts 위에 낸 relations·키·계획; 그 relations 는 `schema12_extract` 의 것과 바이트 동일, `pre_ref_check.txt`), `pre_ref_run.txt`, `df_pre_ref.log`; 장부·등록부의 빌드 전 점검 `tools_check_pre.txt`, `ledger_check_pre.txt`.
- **[I] 로 둔 기대(scratchpad 점검 빌드, LLVM Release 구성, 감시 아래; 근거 아님, 지움)**: 키 바뀜 0; c1 두 빌드에서 이름 없는 형의 FACT_LOCS 대상 3 → 4(`CFE_SB_MemParams_t::Partition.Data`, `CFE_TBL_BufParams_t::Partition.Data` 로 갈라짐, `PSP_SoftTimebase_Global@…::type.sys_timebase_id` 에 memset writer 하나 더, `UnSock_Global@…::type.State` 는 DISABLED), r2 2 → 3, c5 0; relations 행 수와 판정 칸 그대로; 계획의 site 그대로.

#### 5.0a 첫 빌드 뒤의 고침 (둘째 빌드 전에 적고 커밋함) [D][M]

- **선언 밖의 실패 하나** [M]. golden 갱신 뒤의 전체 ctest(`ctest_full.log`, 254 개)에서 `test_static_reasons` 가 실패했다(663 확인 가운데 1, `tests/static/test_static_reasons.cpp:133` `heads != 0`). 그 시험은 `kz-relations 1` 을 담은 golden 마다 행이 하나 이상 있기를 요구한다. 새 `anonrec_np.golden` 은 relations 를 담는데(사례의 `relations` 칸을 참으로 둠) fixture `anonrec.c` 에 스레드가 없어 행이 0 이다. 선언(§5.0)은 `test_static_reasons` 가 통과한다고 적었다. 그래서 여기서 멈추고 이 덧붙임을 먼저 커밋한다.
- **고침**: golden 사례 `anonrec_np` 에서 relations 를 뺀다(`tests/static/static_golden.cpp` 의 그 줄, `relations` 칸 거짓). 이 사례가 고정하려는 것은 이름과 writer·상태(facts 의 섹션)이고, relations 의 행은 아니다. `test_static_reasons` 의 규칙(relations golden 은 행을 가짐)은 그대로 둔다.
- **둘째 빌드**(MC 제자리, 공간 감시 아래, 필요량 100,000,000): `make static_golden`, 그리고 `make all`. 다시 링크되는 것: `tests/static/static_golden` 하나(sha256 이 바뀔 수 있음). 그 밖은 모두 첫 빌드 뒤의 sha256 그대로여야 한다(`after_build1.sha256`: kz-static `c6aa4811…`, `libkz-llvm.a` 와 시험 프로그램 열하나, 그리고 `not_relink.sha256` 의 110). kz-static 의 정체성은 첫 빌드 뒤 그대로다(`tool_sha256=464d62b3…`).
- **golden**: `anonrec_np.golden` 만 바뀐다: 첫 빌드 뒤 갱신한 판의 1–73 줄(facts 의 보기)과 같고, 74 줄부터의 `# relations` 부분이 없다. 다른 golden 은 바뀌지 않는다.
- **시험**: 그 뒤 전체 ctest 254, 실패 0, 건너뜀 1. 그 밖의 선언(§5.0)은 그대로다. 다섯 빌드의 출력은 둘째 빌드 뒤의 kz-static(첫 빌드와 같은 바이트)으로 낸다.

#### 5.0b 다시 뽑기 비교 뒤의 덧붙임 (그 비교를 다시 돌리기 전에 적고 커밋함) [D][M]

- **선언 밖의 차이 하나** [M]. `compare.sh extract` 의 facts.kz 머리 비교(`check_head.py`)가 다섯 빌드 모두에서 한 줄을 더 보았다: kz-dump 의 둘째 줄 `container abi=1 kind=facts size=N` 의 N(컨테이너의 바이트 수). 선언(§5.0)은 머리에서 `tool_sha256`·`artifact_sha256`·`basis_sha256`·`parent[0]` 만 허용했다. 그 밖의 비교(basis.kz 전체, facts 의 섹션 보기, extract.txt, relations.txt, basis.txt, sha256.txt, args.txt, relations.log)는 선언 안이었다(`compare_extract.txt`). 그래서 여기서 멈추고 이 덧붙임을 먼저 커밋한다.
- **까닭**: 컨테이너 크기는 내용의 바이트 수다. 선언이 허용한 내용의 차이(BASIS 항목 하나, 이름 바뀐 형의 새 행, 새 이름 문자열)가 크기를 바꾼다. 앞 단계(§4)의 다시 뽑기는 내용 길이가 같아 이 줄이 같았고, 이번 선언은 그 허용 목록을 그대로 옮겼다. 잰 크기(`facts_sizes.txt`, 옛 → 새, 차이): c1_pre_tx 10,980,952 → 10,996,136(+15,184), c1_post_tx 10,983,800 → 10,999,024(+15,224), r2_core 7,188,992 → 7,195,288(+6,296), c5_641 4,823,448 → 4,824,488(+1,040), c5_642 4,831,960 → 4,833,000(+1,040).
- **고침**: `check_head.py` 에 `--allow-size`(facts 머리에서만): `container abi=1 kind=facts size=N` 의 N 만 다른 줄을 허용한다. basis.kz 는 그대로(`--whole`, 크기 줄 허용 없음; 다섯 모두 크기가 같다). 이 덧붙임 뒤 `compare.sh extract` 를 다시 돌린다(같은 출력 디렉터리, 출력은 바꾸지 않음; 비교 기록만 새로 씀).

### 5.1 바뀐 것 [F]

- 커밋 `6b8c784`(선언, 코드, fixture, 시험, 문서, 비교 스크립트와 허용 파일, 빌드 전 참조, 빌드 전). 규모: `lib/llvm` 열넷과 `tools/kz-static` 둘(+약 330 줄: 대조 블록·공유 함수 둘·`deviation_notes`, 이름 주기 약 90 줄, 잘린 이미지 약 30 줄, relations 사유·계층·머리 약 20 줄), `test_static_cli.cpp` +약 110 줄(확인 42 더), 새 `anonrec.c`, `docs/static_facts.md` §5q.
- 커밋 `9a02643`(첫 빌드의 기록, golden 열하나, 덧붙임 §5.0a, `static_golden.cpp` 의 한 줄), 그 사이 13:57Z 에 스냅숏 커밋 `9cfd23c` 이 자동으로 만들어졌다(내 명령이 아님; 이 기록 디렉터리의 파일, 등록부, 갱신 중인 golden 하나만 담음). 커밋 `172fc74`(둘째 빌드와 다섯 빌드 실행의 기록, 덧붙임 §5.0b, `check_head.py --allow-size`). 커밋 `8195754`(이 절, 비교 기록, 키 표, 장부·등록부 행).

### 5.2 빌드 [M]

| 빌드 | 목표 | 때 (UTC) | dip (B) | 링크 줄 |
|---|---|---|---|---|
| 1 | `kz-static` | 13:46–13:48 | 123,981,824 | 2 (`libkz-llvm.a`, kz-static) |
| 1 | `all` | 13:48–13:50 | 62,271,488 | 12 (`libkz-llvm.a` 를 링크하는 시험 열, `static_golden`, `test_static_cli`) |
| 2 | `static_golden` | 13:53 | 778,240 | 1 |
| 2 | `all` | 13:53 | 12,288 | 0 |

- 빌드 1: 링크된 14 가 `relink_expected.sha256` 과 같고 14 모두 sha256 이 바뀌었다. 다시 링크되지 않아야 하는 110 은 모두 같다(kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz_lint `5f62ec20…`, kz-instr-clang `28db4017…` 포함). 빌드 2: `static_golden` 만 바뀌었고 `after_build1.sha256` 13 과 110 은 같다.
- kz-static `5ae220d2…` → `c6aa4811612f13bd79b826e53e1f104e3e759d4d97123029c232558a98c73507`; `--identity` `tool_sha256` `783c7cd7…` → `464d62b3583c61c62d1a3e168913fab8a76cdcbf24bbc26acbf6346182810a39`, `tool_source` `f73da08c…` → `3a8a7b8d0e980ff7edfdfcb2e448bc008b0b1b2db3cd16b6dcf002f66234fe34`; `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 그대로.
- `CMakeCache.txt` `bc2f43b4…` 그대로, flags.make 목록 해시 `e8f06cbe…` → `95916686…`(kz-static 의 정체성 define). 두 빌드 모두 시작 뒤 바뀐 소스 파일 0(`find -newer`). 출력은 kz-static 이 새 `--identity` 를 적은 뒤에만 냈다.

### 5.3 시험 [M]

- 빌드 1 의 첫 ctest(`ctest_first.log`): 254 개 가운데 실패 11, 선언한 그대로(golden 열과 `test_static_plan`). 갱신한 열하나는 scratchpad 점검 빌드의 것과 `cmp` 같다(`golden_update.log`).
- 갱신 뒤 ctest(`ctest_full.log`): 실패 1, `test_static_reasons`(선언 밖): §5.0a 로 멈추고 고침.
- 빌드 2 뒤(`ctest2_first.log`): 실패 둘(`static_golden_anonrec_np` 선언대로, 그 golden 을 갱신하기 전이라 `test_static_reasons` 도). `anonrec_np.golden` 갱신(`golden2_update.log`): 72 줄, 빌드 1 판의 1–72 줄과 같다. 선언(§5.0a)은 "1–73 줄"이라 적었으나 73 줄은 relations 부분 앞의 빈 줄이고 그것도 relations 부분이다(`static_golden.cpp` 가 `view + "\n# relations\n" + rel` 로 붙임). 선언의 줄 번호가 하나 어긋났을 뿐 바뀐 내용은 선언한 그 부분이다(§5.8).
- 2026-11-16 검토 뒤 정정: §6.7 `builds-1`. 빌드 2 뒤 첫 ctest 의 `test_static_reasons` 실패는 선언 밖이었다: 덧붙임 §5.0a 는 첫 ctest 의 실패를 적지 않았고, 위 항목은 그 실패를 뒤에 와서 예상된 것이라 했다. 까닭은 아직 갱신하지 않은 `anonrec_np.golden` 이 행 없는 relations 부분을 담고 있었던 것이다. 그 ctest 전에 덧붙임은 없었다. 검토 단계의 선언(§6.0 (나))은 첫 ctest 의 실패를 이름으로 적었다.
- 최종(`ctest2_full.log`): 254 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). 확인 수: `test_static_cli` 953(911 + 42), `test_static_reasons` 660(앞 단계 655; 사유 표에 한 줄이 늘고 relations golden 의 줄이 둘 늘었다. 빌드 1 의 663 은 행 없는 relations golden 하나를 더 읽은 수), `test_static_plan` 182, `test_core_schema` 34,017, `lint_selftest` 109, lint 7/7. 단위 점검 15/15(`unit_tools.txt`), 7/7(`unit_step2a.txt`).
- 2026-11-16 검토 뒤 정정: §6.7 `builds-8`. 위의 `test_static_reasons` 660 과 빌드 1 의 663 은 `commands.txt` 에만 있었다(`ctest_counts.log` 는 빌드 1 의 것이고 reasons 가 없다). 660 은 같은 바이너리로 `records/review-fixes-20261116/counts_before_build.log` 에 다시 재 남겼다. 663 은 빌드 2 가 덮어 다시 잴 수 없다.

### 5.4 바이트 동일성과 다섯 빌드 [M]

| 비교 | 결과 |
|---|---|
| relations, T8CF facts (`compare_relations.txt`) | 다섯 빌드 `relations.txt`·`keys.txt` 가 `schema12_relcheck` 와 3 번째 줄(rules revision 8) 밖에서 바이트 동일, `relations.log` 빈 파일(15 OK) |
| order, T8CF facts (`compare_order.txt`) | 두 번 같은 바이트, `order.txt` 는 3 번째 줄 밖에서 같음, `edges.tsv` 바이트 동일, `order.log` 빈 파일(25 OK) |
| 다시 뽑기 (`compare_extract.txt`, `compare/`) | 다섯 빌드 45 OK: `basis.kz` 레코드 모두 같음(머리의 해시 둘만), `facts.kz` 머리는 해시 넷과 크기 줄(§5.0b), facts 의 보기·`extract.txt`·`relations.txt` 는 선언한 갈래만(`compare/facts_<b>.txt`, `extract_txt_<b>.txt`, `relations_<b>.txt`), `basis.txt`·`sha256.txt` 는 해시 줄만, `args.txt`·`relations.log` 같음 |
| 다시 뽑은 facts 위 (`compare_onextract.txt`) | order 두 번 같음, `order.txt` 는 선언한 갈래만(facts 해시, relations 개정, 레코드 전체 쓰기의 짝 줄 5: c1 둘·r2), `edges.tsv` 바이트 동일, `relations.txt` 는 다시 뽑기의 것과 바이트 동일, 키 표(30 OK) |
| 계획 (`compare_plans.txt`) | T8CF 위 다섯과 다시 뽑은 facts 위 다섯이 모두 "only declared differences"(site 와 필요 줄 같음) |

- 2026-11-16 검토 뒤 정정: §6.7 `builds-2`, `builds-9`, `builds-10`. 위 표의 다시 뽑기 45 OK 는 짝 없는 옛 `<anonymous>` 줄과 새 이름 줄을 짝 없이 받고(`compare_classes.py:160-168`, `:174-180`), relations 의 `[C]` 머리를 `(spawn calls ` 뒤 `.*` 로 받은 비교의 결과였다. §6.0 (가) 2 에서 짝짓기와 좁힌 머리로 다시 돌렸고(`records/review-fixes-20261116/extract_compare/`), 덧붙임 (가-1)(`extract.txt` 의 `location` 줄 짝) 뒤 다섯 빌드 45 OK 였다.
- 2026-11-16 검토 뒤 정정: §6.7 `builds-6`. 누적 목록과 등록부는 빌드 1 전(`commands.txt:14`)과 빌드 2 뒤(`:56`)에만 돌렸고, 빌드 1 과 빌드 2 사이에는 돌리지 않았다. 끝의 실행은 통과했다.
- **누적 목록**: `ledger_check.sh` 76 행, 실패 0(새 행 여덟: `OBS_extfix_pre_onextract`, `OBS_extfix_relcheck`, `OBS_extfix_order`, `OBS_extfix_extract`, `OBS_extfix_onextract`, `OBS_extfix_plans`, `OBS_extfix_plans_onextract`, `TOOLS_20261116f`). `tools_check.sh --deep` 39 행, 실패 0. 바깥 이름 점검(`--tree`)은 일곱 출력 디렉터리, 사본 디렉터리, 기록 디렉터리에서 깨끗하다.
- **도구 보존**(`tool_copy/ROUNDTRIP.txt`): kz-static `5ae220d2` 를 빌드 전에 `/home/user/work/preserve/tools-20261116-f/kz-static.5ae220d2.xz`(42,454,428 B)로 사본하고 왕복 확인했다(`xz -t` OK, 스트림 sha256 같음, `cmp` 같음). 등록부: `5ae220d2` copy, `c6aa4811` in-place. 대체되지 않음: 새 도구의 출력은 개정 줄과 이름에서 다르다.

### 5.5 측정: 영향 범위 [M]

**이름 없는 레코드 형.** 다섯 빌드 모두 다시 뽑았다. 새 이름의 수(그 facts 보기에 있고 옛 보기에 없는 형 이름): c1_pre_tx 25, c1_post_tx 25, r2_core 22, c5_641 5, c5_642 5. facts 보기의 줄 갈래(`compare/facts_<b>.txt` 의 합):

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| renamed(이름만 바뀐 줄) | 141 | 141 | 75 | 3 | 3 |
| added-named(새 이름으로 새로 맞은 행) | 276 | 276 | 123 | 38 | 38 |
| removed-anon(`<anonymous>` 행이 갈라지거나 사라짐) | 54 | 54 | 42 | 24 | 24 |
| BASIS 수·항목(change, insert) | 10, 2 | 10, 2 | 8, 1 | 2, 1 | 2, 1 |
| CF `CF_CmdGetSetParam` entry-release 행(더해짐/빠짐) | 8 / 7 | 8 / 7 | 0 | 0 | 0 |
| site 번호 등 움직인 번호만 다른 줄 | 43,564 | 43,577 | 21,255 | 117 | 117 |

- 2026-11-16 검토 뒤 정정: §6.7 `builds-2`, `builds-10`. 위 표의 갈래는 짝을 짓지 않은 셈이고(`removed-anon`, `added-named`), “site 번호 등”의 줄은 모두 `mask:site` 로 셌다(개정 1 이 두 줄을 다르게 남기는 첫 가림의 이름을 붙임). 짝을 짓고 가림을 나눈 새 표는 §6.5 에 있다: c1_pre_tx 로 renamed 249, successor 17, added-named 151, removed-anon-paired 54, 가림 site 41,919 / row 1,634 / count-head 10 / and-more 1. 줄의 합과 가림의 합은 같다.

FACT_LOCS 와 FIELD_STATUS 의 그 형 행(`compare/names_<b>.txt`; 옛 → 새):

| | c1_pre_tx, c1_post_tx | r2_core | c5_641, c5_642 |
|---|---|---|---|
| FACT_LOCS 행 | 3 → 4 | 2 → 3 | 0 → 0 |
| 그 가운데 writer-set-complete | 3 → 3 | 2 → 3 | — |
| 그 가운데 complete 이고 writers 0 | 2 → 0 | 1 → 0 | — |
| FIELD_STATUS 행 | 10 → 21 | 6 → 13 | 0 → 0 |
| 그 가운데 두 규칙 모두 complete | 10 → 11 | 6 → 8 | — |
| 그 가운데 두 규칙 complete 이고 writers 0 | 9 → 1 | 5 → 1 | — |

- C1 의 `<anonymous>.Data`(writers 0, complete)는 `CFE_SB_MemParams_t::Partition.Data` 와 `CFE_TBL_BufParams_t::Partition.Data` 둘로 갈라졌고, 각각 memset writer 하나(`cfe_sb_init.c:61:5`, `cfe_tbl_internal.c:52:5`)가 맞는다(FACT_LOCS complete writers 1; FIELD_STATUS 는 `array-element-stores-not-collected` 로 DISABLED, 앞의 규칙 그대로).
- 2026-11-16 검토 뒤 정정: §6.7 `semantics-3`. 위 두 행의 `writer-set-complete writers=1` 은 소스가 뒷받침하지 않는다. `CFE_SB_Global.Mem.Partition.Data` 는 배열에서 포인터로 바뀌어 `CFE_ES_PoolCreateEx` 에 `void *` 로 넘어가고(`cfe_sb_init.c:104`; TBL 은 `cfe_tbl_internal.c:105` 의 `CFE_ES_PoolCreate`), 풀 코드가 그것을 정수 주소 `BaseAddr` 로 두고(`cfe_es_mempool.c:313`) `(CFE_ES_GenPoolBD_t *)(BaseAddr + Offset)` 으로 그 안에 쓴다(`:84-85`). FACT_LOCS 는 그 넘김을 주소 탈출로 세지 않는다. 이 결함은 이름 고침 전부터 있었다(옛 `<anonymous>.Data` 도 complete). 추출의 고침은 하지 않았다(§6.9; `kairos/docs/static_facts.md` §5r 의 알려진 한계).
- `<anonymous>.sys_timebase_id` 는 `PSP_SoftTimebase_Global@psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase_c:44::type.sys_timebase_id` 이고 writer 가 1 → 2(memset `cfe_psp_soft_timebase.c:50:5` 가 더해짐). 앞판에서 빠진 까닭은 memset 의 대상 형을 이름 표로 찾는 `owners_contained_in`(`lib/llvm/ast_fn.cpp` 의 데이터 함수 처리)이 이름 표에 없는 형을 맞추지 못한 것이고, 이제 그 형이 이름 표에 있다.
- C1 의 `<anonymous>.State`(writers 0, complete)는 `UnSock_Global@…/unsock_intf_c:85::type.State` 이고 memset writer 1(`unsock_intf.c:231:5`)과 untyped 133·escape 1 hit 로 DISABLED 다.
- C5 에는 FACT_LOCS·FIELD_STATUS 의 그런 행이 없고, 바뀐 것은 FUNC_WRITES·LOCKSET·LOCK_PATHS 의 `record … .*` 이름이다(OSAL 4.1.1 의 이름 없는 레코드가 시스템 머리말 형 `__pthread_mutex_s::__elision_data` 등으로).
- 시스템 머리말 형도 이름을 얻는다(`siginfo_t::_sifields::_sigfault.si_addr` 등). 같은 정의는 모든 단위에서 같은 이름이다.
- 2026-11-16 검토 뒤 정정: §6.7 `semantics-4`. 위 “같은 정의는 모든 단위에서 같은 이름”은 한 추출 안의 말이다. 묶음 루트 밖 파일의 위치는 절대 경로로 쓰여(`TuReader::name_pos` → `pos_of`, `lib/llvm/ast_tu.cpp`), 같은 CF 소스의 형이 c1_pre_tx 에서 `item@/home/user/work/procrace/c1_build/pre/…/cf_cmd_c:945::type`, c1_post_tx 에서 `…/c1_build/post/…` 로 다른 이름을 얻는다. 오늘 relations 의 키는 이 이름을 담지 않는다(`key_map_<b>.tsv`: 바뀐 키 0). `kairos/docs/static_facts.md` §5q 에 날짜 붙은 줄을 두었다(§6.9).
- 간접 호출 하나의 해석이 바뀌었다(c1 두 빌드): `cf_cmd.c:1014:18` 의 `item.fn(...)` 이 `incomplete-source`(지역 객체 `item` 의 memset 이 writer 로 맞음). 그래서 relations 가 인용하는 "풀리지 않은 간접 호출"의 수가 62 → 63 이고(행 383 줄), LOCK_PATHS 의 entry-release 행 15 가 그 호출을 거친다.

**relations 와 키.** 다섯 빌드 모두 행 수와 판정 칸이 그대로다(relations 비교의 갈래는 머리 셋, 그 수 인용, `[B]`·`[C]` 머리, 이름 바뀐 줄 c1 2·r2 1). `[B]` 머리의 "left out: no writer / one actor only": c1 두 빌드 33/111 → 31/113, r2 11/17 → 9/19, c5 그대로. `[C]` 머리의 "record types written whole after a spawn … not paired": c1 380 → 383, r2 177 → 178, c5 102 그대로. **옛 키 → 새 키 표**(`key_map_<b>.tsv`): 다섯 빌드 모두 바뀌는 키 0(같음 367/367/295/178/175, renamed 0, 한쪽에만 0). relations 행의 키 글에 이름 없는 형이 든 행이 없기 때문이다. 표는 행마다 같은 키를 적는다.

**order 와 계획.** order 의 간선 목록은 다섯 빌드 모두 바이트 동일하고, `order.txt` 에서 바뀐 것은 레코드 전체 쓰기의 짝 줄 5(c1 두 빌드, r2: `[<anonymous>.*]` → `[CFE_PSP_ExceptionStorage_t.*]` 등 다른 레코드 이름이 첫 자리)뿐이다. 계획은 다섯 빌드 모두 site 와 필요 줄이 같다.

**읽기 옵션.** 다섯 빌드의 다시 뽑기는 모두 `extract.reading_options_vs_basis = same as basis front_end.arguments_added and front_end` 이다(`one_build.sh` 는 basis 와 extract 에 같은 옵션을 준다). basis.kz 가 레코드 단위로 같다는 것은 공유 함수가 앞판과 같은 글을 만든다는 측정이다. 다름·거부·`--accept-option-difference`·relations 의 사유와 계층 표시는 합성 시험(`test_static_cli`)에서만 보았다(실제 빌드는 다르지 않다).

**잘린 이미지.** 다섯 빌드는 표시 잎이 0 이라(§5j, §5m) 펼치기와 PTR_FIELDS 의 고침이 닿는 행이 없다: LOOP_ITERS 의 호출 행 text 칸은 모두 비고, PTR_FIELDS 의 `resolved`/`may` 는 바뀌지 않았다(facts 보기 비교에 그 갈래 없음). 고침은 합성 시험(`test_static_cli`, `--image-budget 2` 대 기본 예산)에서 보았다.

### 5.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 이 단계 처음 | 3,159,941,120 | 시작 |
| scratchpad 점검 빌드(감시 아래) | dip 261,656,576(첫 make), 뒤에 시험까지 887 MB | 근거 아님, 지움 |
| 13:41 | 3,147,706,368 | 빌드 전 참조(dip 8,589,312) |
| 13:46 | 3,104,301,056 | 사본(dip 42,475,520) |
| 13:50 | 3,037,876,224 | 빌드 1(kz-static dip 123,981,824, all 62,271,488) |
| 13:53 | 3,038,326,784 | 빌드 2 |
| 13:58 | 2,972,991,488 | 다시 뽑기(dip 48,410,624) |
| 14:1x | 2,956,251,136 | relations, order, 계획, 비교 뒤, scratchpad 정리 뒤 |

- 하한을 넘지 않았다. 남긴 출력: `extfix_pre_onextract` 8,452,020, `extfix_relcheck` 7,564,932, `extfix_order` 7,758,336, `extfix_extract` 46,902,429, `extfix_on_extract` 15,324,352, `extfix_plans` 886,411, `extfix_plans_on_extract` 886,913 B(합 87,775,393 B, 장부 행), 사본 42,454,428 B. 지운 것: compare.sh 가 비교 뒤 지운 둘째 실행(`again/`), facts 보기(scratchpad, 비교 뒤 바로), 그리고 이 단계의 scratchpad 작업 파일(점검 빌드, 점검 출력, golden 사본)뿐이다.
- 이번 kz-static 빌드의 dip 123,981,824 B 는 앞 단계의 75,079,680 B 보다 크다(`lib/llvm` 열넷이 다시 컴파일됨).

### 5.7 계획과 다르게 한 것

- **다섯 빌드 모두 다시 뽑았다.** 계획은 c1_post_tx 와 r2_core 를 공간이 허락할 때만 다시 돌린다고 했다. 공간이 넉넉해 다섯을 모두 돌렸고 영향 범위는 다섯 빌드 전체다(부분이 아님).
- **옛 키 → 새 키 표는 비었다.** 계획은 "이름이 바뀐 형의 행의 키가 바뀐다"고 했다 [I]. 잰 값은 0 이다: 다섯 빌드의 relations 행 어느 것도 키 글에 이름 없는 형을 담지 않았다(T8CF 의 relations 에도 `<anonymous>` 를 담은 행이 없다: `grep` 으로 그 글은 id holder 줄 둘에만 있다). 표는 행마다 같은 키로 남긴다.
- **relations 규칙 개정을 8 로 올렸다.** 새 세부 사유와 계층 표시는 규칙의 바뀜이라 개정 글을 올렸다. 그 때문에 relations·키·order·계획의 개정 줄과 golden 여덟이 바뀌었다(선언 안).
- **형 이름의 위치는 점 없이 쓴다**(파일의 `.` 를 `_`). 객체 키를 그대로 쓰면 `…cfe_psp_soft_timebase.c:44::type` 처럼 점이 들어가는데, `Type.field` 를 첫 점에서 나누고 둘째 점을 거부하는 곳(`spec.cpp:10`)이 있어 점 없는 쪽을 택했다. 객체 키(GLOBAL_IMAGES 의 객체 칸 등)는 그대로다.
- **이름 있는 멤버의 형인 태그 없는 레코드는 제 형이다.** 그 형을 `<Outer>::<member>` 로 부르면 앞판의 `Unn.q` 같은 맞춤(감싼 레코드의 필드로)이 없어진다. §5k 가 성질로 적은 것을 바꾼 것이고, 선언과 `test_static_cli` 에 적었다.
- **잘린 이미지의 펼치기 고침은 값이 아니라 글이다.** 찾지 못한 필드는 전에도 모름(상수 아님)이었다. 바뀐 것은 그 까닭(잘린 경로)을 적는 것이고, residual guard 의 까닭은 호출 행의 비어 있던 text 칸에 둔다(schema 개정 없이). `kz-static show` 가 그 칸을 보이도록 했다.
- **선언 밖의 일 셋**: (1) `test_static_reasons` 의 실패(행 없는 relations golden; §5.0a 로 멈추고 고침, 둘째 빌드); (2) facts.kz 머리의 크기 줄(§5.0b 로 멈추고 허용을 덧붙인 뒤 비교를 다시 돌림; 출력은 바꾸지 않음); (3) §5.0a 의 줄 번호가 하나 어긋남(73 이 아니라 72; 빈 줄도 relations 부분, §5.3). (1)·(2) 는 각각 덧붙임을 커밋한 뒤에 다음을 돌렸고, (3) 은 갱신 뒤에 보아 덧붙임 없이 여기 적는다.
- 2026-11-16 검토 뒤 정정: §6.7 `builds-1`. 선언 밖의 일이 하나 더 있다: 빌드 2 뒤 첫 ctest 의 `test_static_reasons` 실패(§5.3 의 정정 줄). 그 ctest 전에 덧붙임은 없었다. 뒤 단계의 덧붙임이 golden 을 바꾸면 첫 ctest 에서 나올 실패를 이름으로 적는다.
- **scratchpad 점검 빌드**는 감시 아래 돌렸다(앞 단계의 다짐대로). LLVM Release 구성으로 만들어 MC 의 것과 바이트가 다르다. 증거로 쓰지 않았고 지웠다.

### 5.8 열린 것

- 이름 없는 형이 FACT_LOCS 에서 writer-set-complete 이면서 그 배열 원소 쓰기는 FIELD_STATUS 만 본다(`CFE_SB_MemParams_t::Partition.Data`: FACT_LOCS complete writers 1, FIELD_STATUS DISABLED `array-element-stores-not-collected`). FACT_LOCS 의 규칙(배열 원소 쓰기를 세지 않음)은 앞판 그대로다.
- 2026-11-16 검토 뒤 정정: §6.7 `semantics-3`, `semantics-4`. 위 항목의 빈칸은 배열 원소 쓰기만이 아니다: 배열에서 포인터로 바뀌어 호출 인자로 넘어가는 탈출도 FACT_LOCS 가 세지 않아 `CFE_SB_MemParams_t::Partition.Data`, `CFE_TBL_BufParams_t::Partition.Data` 의 complete 는 소스가 뒷받침하지 않는다(§5.5 의 정정 줄). 또 위치가 붙은 형 이름은 빌드 사이에서 같지 않다(묶음 루트 밖 파일의 절대 경로). 이름 결합과 빌드 사이 단계는 이 이름을 그대로 쓰지 않는다(§6.9).
- 읽기 옵션의 대조는 글의 비교다. 같은 글이 같은 읽기를 준다는 것은 같은 도구 판 안의 전제다. 다른 도구 판의 basis 로 추출하면 `front_end` 가 달라 거부된다(`--accept-option-difference` 로 편차를 적고 진행).
- 이름 없는 형의 이름을 쓰는 다음 일: 객체 접근(OBJECT_ACCESS)의 바탕 객체와 형 단위 대신 객체 단위의 writer 집합(뒤 추출 단계). 이번 이름은 형을 갈랐을 뿐 같은 형의 두 객체를 가르지 않는다.

## 6. 검토 뒤 고침 (2026-11-16)

§1–§5 의 검토가 확인한 지적 스물여섯(판정 규칙 `rules-01`–`rules-10`, 빌드와 비교 `builds-1`, `builds-2`, `builds-4`–`builds-10`, 의미 `semantics-1`–`semantics-4`)을 고친다. 지적마다 무엇이 틀렸고 무엇을 바꿨고 무엇을 다시 돌렸는지는 §6.7 의 표에 둔다. 앞 절의 글은 지우지 않고, 고친 자리에 날짜 붙은 표시(“2026-11-16 검토 뒤 정정: §6.7 <id>”)를 단다. 기록: `../records/review-fixes-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `revfix_*`. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 6.0 선언 [D]

#### 6.0 (가) 빌드 전의 점검 둘 (도구를 바꾸기 전에 적고 커밋함)

둘 다 지금 MC 의 도구(kz-static `c6aa4811…`)와 보존 사본만 쓰고 Kairos 를 빌드하지 않는다. 아래 밖의 차이는 결함이다. 결함이면 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다.

1. **relations 규칙 개정 7 의 다시 뽑기의 키 목록**(`builds-4`; 선언 §2.0 의 “그 `relations.txt` 와 `keys` 는 위 `rerun.sh` 의 것과 둘째 줄 밖에서 바이트 동일”, §2.7 에서 덧붙임 없이 빠뜨림). `keys_check/keys_check.sh`: 그 단계의 도구 kz-static `038cd9e3` 을 보존 사본(`/home/user/work/preserve/tools-20261116-d/kz-static.038cd9e3.xz`)에서 scratchpad 로 풀고(스트림 sha256 = `038cd9e36af4…` 확인), 그 단계가 다시 뽑은 facts(`relrev7_extract/<b>/<b>/NP/facts.kz`)로 `relations --keys` 를 새 디렉터리 `revfix_relrev7_extract_keys/<b>/` 에 낸다. 선언: 다섯 빌드 모두 `keys.txt` 가 `relrev7_check/<b>/keys.txt` 와 둘째 줄(facts 해시) 밖에서 바이트 동일, `relations.txt` 는 `relrev7_check/<b>/relations.txt` 와 둘째 줄 밖에서 같고 `relrev7_extract/<b>/<b>/NP/relations.txt` 와 바이트 동일, `keys.txt` 의 둘째 줄은 다시 뽑은 facts 를 가리킴, `relations.log` 빈 파일. 공간: 감시 아래(필요량 60,000,000: 도구 185 MB 를 푼 것과 출력 약 8 MB; 사본에서 푼 바이너리는 끝에 지움). 장부 행 `OBS_revfix_relrev7_extract_keys`.
2. **추출 고침 단계의 다시 뽑기 비교를 다시 돌림**(`builds-2`, `builds-9`, `builds-10`). 앞 비교(`records/extract-fixes-20261116/compare.sh extract`, `compare_extract.txt`, `compare/`)는 그대로 두고, 새 비교를 `extract_compare/` 에 쓴다. 출력 디렉터리는 쓰지 않는다(읽기만).
   - `extract_compare/compare_classes.py` 개정 2: (ㄱ) 가린 줄은 그 줄에 작용한 가림(옛 줄이나 새 줄을 바꾸는 가림) 모두의 이름으로 센다(`builds-10`; 개정 1 은 두 줄을 다르게 남기는 첫 가림의 이름을 붙였다: `records/extract-fixes-20261116/compare_classes.py:119`). (ㄴ) 짝이 없는 옛 `<anonymous>` 줄은 같은 절 전체에서 그 뒤를 잇는 새 줄과 짝짓는다: 새 이름을 `<anonymous>` 로 쓰고 그 절의 `pair` 행이 이름 댄 칸을 양쪽에서 가렸을 때 같은 새 줄이 뒤를 잇는 줄이다. 뒤를 잇는 줄이 없는 옛 줄은 선언 밖(`other`)이다(`builds-2`; 개정 1 은 짝 없는 옛 `<anonymous>` 줄을 모두 `removed-anon` 으로, 새 이름을 단 줄을 모두 `added-named` 로 받았다: 같은 파일 `:160-168`, `:174-180`). 뒤를 잇지 않는 새 이름 줄은 `added-named`(새로 맞은 행, 선언 §5.0 (가))로 남는다. 짝은 `pairs …` 아래에 옛 줄과 뒤를 잇는 줄로 모두 적는다.
   - `extract_compare/facts_allow.tsv`: 앞 허용(`records/extract-fixes-20261116/facts_allow.tsv`)의 행 그대로에 `pair` 행 일곱. 선언하는 칸(§5.0 (가)의 “그 형의 필드에 이제 맞는 writer·A-TYPE·FACT_INIT·FIELD_STATUS 행”에서 따르는 칸만): FACT_LOCS(`[locs]`) 줄의 상태 낱말(`writer-set-complete`, `writer-set-incomplete`, `DISABLED`), `writers=N (direct N, data-fn N, out-pointer N)`, A-TYPE 수(`atype: untyped=… escape=… cast=… union=… unknown-effect=… init=…`); FIELD_STATUS(`[fstatus]`) 줄의 `per-field=… type-level=…`, `writers=N (… init N) instances=N <한 번 표지>`, `hits: escape …, cast …, union …, unknown …, other fields …;`, 꼬리의 `{표지}` 와 ` by …`. 위치(새 이름을 `<anonymous>` 로 쓴 뒤), 프로그램 전체의 `untyped sites N`, `unresolved-indirect=N` 은 그대로 비교한다. 그 밖의 절의 짝은 새 이름만 다른 줄이어야 한다.
   - `extract_compare/relations_allow.tsv`: 앞 허용 그대로에 `[C]` 머리 행만 바꿈(`builds-9`): 그 머리의 글을 낱말 그대로 두고 수만 `\d+`(앞 행은 `(spawn calls ` 뒤를 `.*` 로 받았다). 선언 §5.0 의 relations 갈래는 “`[B]`·`[C]` 머리의 수”다.
   - `extract_compare/compare_extract.sh`: 위 셋으로 facts 보기·`relations.txt` 를, 앞 단계의 `extract_txt_allow.tsv` 로 `extract.txt` 를 비교하고, 나머지 점검(basis.kz, facts.kz 머리, basis.txt, sha256.txt, args.txt, relations.log)은 앞 단계의 `check_head.py` 로 그대로 다시 돈다. facts 보기는 `extfix_extract` 를 낸 도구(`c6aa4811`, 스크립트가 먼저 확인)로 scratchpad 에 쓰고 비교 뒤 지운다.
   - 선언: 다섯 빌드 45 점검 모두 OK. 빌드 전 scratchpad 에서 c1_pre_tx 하나를 이 스크립트로 돌려 봤다 [I, 근거 아님]: 선언 밖 0, `[locs]` 의 옛 `<anonymous>` 줄 셋과 `[fstatus]` 의 열이 모두 짝을 얻음(`CFE_SB_MemParams_t::Partition.Data` 와 `CFE_TBL_BufParams_t::Partition.Data` 로 갈라짐 등), 가림 갈래 `mask:site`, `mask:row`, `mask:count-head`, `mask:and-more` 가 따로 셈.
3. **개수 기록**(`builds-8`): 이 점검과 함께 지금 MC 의 시험 프로그램(추출 고침 단계의 빌드 2 뒤의 바이너리, 그 sha256 은 `records/extract-fixes-20261116/after_build1.sha256` 과 `post_mc_build2.txt`)으로 `ctest -V -R '^test_static_reasons$'` 를 돌려 그 출력의 개수 줄을 `counts_before_build.log` 에 남긴다(그 단계의 결과 §5.3 의 660 을 기록으로 뒷받침). relations 개정 7, order 개정 1 단계의 시험 프로그램은 뒤 빌드가 덮어 다시 잴 수 없다.

#### 6.0 (가-1) 비교를 다시 돌린 뒤의 덧붙임 (그 비교를 다시 돌리기 전에 적고 커밋함) [D][M]

- **선언 밖의 차이** [M]. `compare_extract.sh` 의 첫 실행(`extract_compare/first/compare_extract.txt`, 15:00Z)에서 c1_pre_tx, c1_post_tx, r2_core 의 `extract.txt` 가 BAD 였다: 옛 `location <anonymous>.Data …`, `.State`, `.sys_timebase_id`(r2 는 둘) 줄이 뒤를 잇는 줄을 얻지 못했다(`first/compare/extract_txt_<b>.txt`). 나머지 42 점검은 OK 였다(facts 보기 다섯은 선언 밖 0). 그래서 여기서 멈추고 이 덧붙임을 먼저 커밋한다.
- **까닭**: `extract.txt` 의 `location` 줄은 FACT_LOCS 의 요약이고(facts 보기의 `[locs]` 줄과 같은 칸: writer 수와 그 나눔, A-TYPE 수, 상태), 선언 §5.0 의 `extract.txt` 갈래가 “(가)의 `location` 줄”을 이미 받았다. §6.0 (가)는 facts 보기에만 `pair` 행을 두고 `extract.txt` 에는 앞 단계의 허용 파일을 그대로 써서, 그 줄의 짝을 지을 칸을 적지 않았다. 고쳐진 비교(짝 없는 옛 `<anonymous>` 줄은 선언 밖)가 그 빈칸을 드러낸 것이다.
- **고침**: `extract_compare/extract_txt_allow.tsv`(새): 앞 단계의 행 그대로에 `pair` 행 하나: `location` 줄의 `writers N (direct N, data function N, out-pointer N, initial values N), A-TYPE hits: untyped N, escape N, cast N, union N, unknown effect N  -> <상태>`(facts 보기 `[locs]` 짝과 같은 칸). 위치는 새 이름을 `<anonymous>` 로 쓴 뒤 그대로 비교한다. `compare_extract.sh` 는 이 파일을 쓴다. 첫 실행의 기록은 `extract_compare/first/` 에 그대로 두고, 이 덧붙임의 커밋 뒤 같은 스크립트를 다시 돌린다. 선언: 45 점검 모두 OK.

#### 6.0 (나) kz-static 빌드 하나: order rules revision 2, relations rules revision 9 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다.

- **무엇을 하는가** (`kairos/docs/static_facts.md` §5r):
  1. **order rules revision 2 — 호출자에 기대는 spawn 호출**(`semantics-1`). spawn 연산의 사슬에 든 호출(spawn 호출, spawn-op 표시가 있는 호출, SPAWN_WRAP entry-argument·called-by-entry 행의 호출)이 부르는 함수 안의 spawn 호출은 호출자에 기댄다. 닫힘의 spawn 마디(`'s'`)에서, 그런 호출에 닿은 스레드 t 는 t 의 코드에 있는 그 함수의 호출자가 만들 수 있는 스레드로만 간다(바깥으로 따라감; 그 호출에 풀린 스레드가 있으면 그 안에서; 풀린 entry 를 넘기지 않는 호출자는 아무것도 더하지 않음). HB-lock 간선도 쓰기의 스레드가 그 호출로 만들 수 있는 자식에게만 낸다. 그런 HB-spawn 간선에 `caller-dependent:` 줄(따라가는 스레드). 머리의 `closure:` 줄에 한 문장.
  2. **order rules revision 2 — child without success 의 위치 근거**(`semantics-2`). 증거 반환이 소스 위치만으로 서면(값이 RETURNS `redefined-later` 정의에서 옴, 조건이 지역을 읽음, 그것이 옮기는 안쪽 호출의 표지가 그러함) `possible` 대신 `unknown (position-based): every witness rests on the source position alone (…)`. 머리의 `summary spawn calls` 줄에 `(position-based N)`.
  3. **relations rules revision 9**(`rules-01`). 생성자 callee 의 상수 줄 가운데 `redefined-later` 정의를 거쳐 오는 것은 끝에 `; not shown to reach the return: <정의>`. 생성자 callee 마다 `callee-check (set): <callee>: complete under these facts (…)` 또는 `… not established (N returns not followed, M constants not shown to reach the return): …; no success constant is taken from them`.
  4. **설명 스크립트 `env/obs1/explain.py` 개정 3**(`rules-02` 의 스크립트 쪽): 생성자 callee 의 행에 `callee-check (returns not followed): <callee>` 또는 `callee-check (set): <callee>: not established` 줄이 있으면 성공 상수가 없고(out 을 읽지 않음), `not shown to reach the return` 이 붙은 줄의 상수는 받지 않는다. 그 까닭의 하위 사유 `record_insufficient(no success constant of the creator: the callee's returns are not all followed)`(판정 규칙 8.4 의 2026-11-16 덧붙임). 이 주기의 기록은 다시 내지 않는다(부팅 없음; 그 기록의 relations 입력 `plangen_relcheck` 에는 생성자 callee 줄이 없어 개정 3 으로도 같은 사유다 [I]).
- **1차 근거**: 닫힘의 spawn 마디가 스레드와 무관하게 `spawn_children(S)` 로 갔다(`kairos/lib/llvm/order.cpp` 빌드 전 판 `:957-961`); 스레드의 시작 마디가 그 코드의 모든 spawn 호출로 간다(`:963-967`); HB-lock 이 `kids` 의 모든 자식에게 간선을 냈다(`:674`). C1 에서 BP 의 `CFE_ES_CreateChildTask` 호출은 `BP_CLA_*` 만 만들고(SPAWN_WRAP fixed-entry `bp_cla_bundle_io.c:155:17`, `:207:17`), `CFE_ES_BackgroundTask` 는 ES 의 `CFE_ES_BackgroundInit`(`cfe_es_backgroundtask.c:184`)이 만든다. 위치 근거: `os-impl-tasks.c:544-548` 은 `pthread_create` 실패 갈래이고 성공 뒤 반환은 `:562` 뿐(묶음 1df262a); `osapi-task.c:202` 의 RETURNS 행 alt 0 은 `OS_ObjectIdAllocateNew` 의 정의가 `redefined-later`(c1 facts 보기); `cfe_es_api.c:1332` alt 0–6 도 `redefined-later`. 생성자 상수: `osapi-countsem.c:94-109`(성공이면 `:103`, `:106` 에서 다시 정의, `:109` 에 실패 값만), c1 facts 의 RETURNS 행 `:109:5 alt 0/2 … {from-definition,redefined-later}`.
- **코드**(`kairos/`): `lib/llvm/order.cpp`(개정 문자열, `chain_call`, `caller_dependent`, `callers_of`, `children_on`, 닫힘의 spawn 마디, HB-lock 의 자식, HB-spawn 의 `caller-dependent:` 줄, `closure:` 머리 줄, `positional_only`, `strength_of`, `child_without_success`, summary 줄), `lib/llvm/relations.cpp`(개정 문자열 8 → 9, `callee_returns` 의 표시, `callee-check (set)` 줄); fixture `tests/static/fixtures/order.c`(사례 둘: 두 스레드가 한 spawn 감싸개로 제 자식을 만듦 `peers`, 지역이 기본 연산의 결과를 담는 실패 반환 `local`; 머리 주석 7 줄이 늘어 뒤 위치가 7 줄 밀림), `tests/static/fixtures/relcallback.c`(생성 API 의 성공 값이 뒤에 다시 정의되는 꼴과 매개변수를 돌려주는 마무리 함수; 줄 번호는 그대로), `tests/static/test_static_cli.cpp`(개정 문자열 셋, 확인 10 더: relcallback 셋, order 일곱; 953 → 963 [I]); `docs/static_facts.md` §5r(새), §5p·§5q 의 날짜 붙은 한 줄. 저장소의 연구 스크립트: `env/obs1/explain.py`(개정 3), `env/obs1/tests/test_explain.py`(경우 둘 더: 20 → 22 [I]).
  - 하지 않는 것: `include/`, `cmake/`, 최상위 `CMakeLists.txt`, `tests/static/CMakeLists.txt`, 추출(`ast_*`, `argflow`, `finalize`, `serialize`, `lockset`, `target_rules`, `rel_facts`), 계획 생성기, `runtime/`, `instrument/`, `tests/lint/`, 어휘 자료(새 바탕 사유 없음), 2a 드라이버와 `one_build.sh`.
- **도구 해시와 정체성**: kz-static 의 파일 sha256(빌드 전 `c6aa4811612f13bd79b826e53e1f104e3e759d4d97123029c232558a98c73507`)과 `--identity` 의 `tool_sha256`(빌드 전 `464d62b3…`)·`tool_source`(빌드 전 `3a8a7b8d…`)가 바뀐다. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 는 그대로 [I]. 다시 링크되는 것 13(`relink_expected.sha256`, 빌드 전 sha256): `libkz-llvm.a`, kz-static, `libkz-llvm.a` 를 링크하는 시험 프로그램 열, 소스가 바뀐 `test_static_cli`. `static_golden` 은 소스가 바뀌지 않아 다시 링크되지 않는다. 다시 링크되지 않아야 하는 것 111(`not_relink.sha256`: MC 의 ELF 실행 파일과 정적 라이브러리 가운데 위 13 밖 모두; kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz_lint `5f62ec20…`, kz-instr-clang `28db4017…`, `static_golden` 포함). `CMakeCache.txt` `bc2f43b4…` 그대로(CMakeLists 를 바꾸지 않음) [I]. flags.make 목록 해시는 바뀐다(kz-static 의 정체성 define).
- **빌드**(MC 제자리, 목표별, 각각 공간 감시 아래): `make kz-static`, 그리고 `make all`. 감시가 도중에 멈추면 적고, kz-static 이 새 `--identity` 를 적을 때까지 출력을 내지 않는다.
- **도구 보존**: 빌드 전에 kz-static `c6aa4811` 을 새 디렉터리 `/home/user/work/preserve/tools-20261116-g/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 공간 감시 아래). 새 도구의 출력은 옛 도구의 것과 다르므로(개정 줄, 표지, 짝) "대체됨"이 아니라 사본으로 남긴다. 등록부: `c6aa4811` in-place → copy, 새 kz-static in-place. 장부: 사본 디렉터리 행 `TOOLS_20261116g`.
- **golden**(빌드 전 scratchpad 의 점검 프로그램으로 미리 만들어 봄 [I, 근거 아님]): `tests/static/data/` 의 `relations`, `relations_np`, `relpaths_np`, `relsound_np`, `relincomplete_np`, `relincomplete_cut_np`(각각 `rules … revision 8` → `9` 한 줄, 그리고 생성자 행마다 `callee-check (set): … complete under these facts (…)` 한 줄: 2/2/0/0/2/3 줄), `relcallback_np`(fixture 가 바뀌어 facts 보기의 함수·호출·인자 흐름 줄과 위치, relations 부분의 개정 줄·id holder·writer 위치, 생성자 상수 셋의 표지, `returns not followed` 줄, `callee-check (set): sem_create: not established (1 returns not followed, 3 constants not shown to reach the return)…`), `order_np`(fixture 가 바뀌어 위치가 7 줄 밀리고 새 행 B3·새 간선, 개정 줄, `closure:` 줄, 요약, `caller-dependent:` 줄, `start_local` 의 `unknown (position-based)`; 앞 행 A1–A3, B1, B2, C1–C5 의 값은 그대로), `plangen_np.plan.golden`(두 줄: manifest 머리와 sidecar `rules` 의 relations 개정). **첫 ctest 에서 실패는 아홉이어야 한다**: `static_golden_{relations,relations_np,relpaths_np,relsound_np,relincomplete_np,relincomplete_cut_np,relcallback_np,order_np}` 여덟과 `test_static_plan`. `test_static_cli`(963 [I]), `test_static_reasons`(660 [I]; 갱신한 golden 으로도 660 [I]), lint 7/7(빌드 전 MC `kz_lint` 으로 이 트리에서 7/7 [M])는 처음부터 통과한다. 갱신한 것은 scratchpad 의 것과 `cmp` 같아야 한다. 그 밖의 golden(`tests/golden/data`, `tests/static/data` 의 나머지, `tests/cfs-free/golden`, `tests/phi/data`)은 바뀌지 않는다.
- **시험**: 전체 ctest 254(새 시험 없음), golden 갱신 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). 확인 수는 `ctest -V` 의 출력으로 `ctest_counts.log` 에 남긴다(`builds-8`): `test_static_cli` 963, `test_static_reasons` 660, `test_static_plan` 182, `test_core_schema` 34,017, `lint_selftest` 109. 단위 점검 `env/tools/tests/run_unit.sh` 15/15(점검 15 의 설명 fixture 22/22 [M: 빌드 전]), `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤와 다섯 빌드 실행 뒤 각각 `env/tools/ledger_check.sh` 전체(빌드 전 76 행, 실패 0 [M])와 `env/tools/tools_check.sh --deep`(빌드 전 39 행, 실패 0 [M]), 출력을 따로 남김(`builds-6`).
- **다섯 빌드**(새 디렉터리, 비교는 `compare.sh`, 스크립트는 이 선언과 함께 커밋):
  - relations, T8CF facts(`rerun.sh` → `revfix_relcheck`), `extfix_relcheck` 와: `compare_relations.py` — 3 번째 줄(개정 8 → 9), 생성자 상수 줄 끝의 표지(옛 줄은 새 줄에서 표지를 뗀 것), 새 `callee-check (set)` 줄만. `keys.txt` 는 3 번째 줄만. `relations.log` 빈 파일. [I]: 표지 붙은 줄 94/94/14/18/18, `not established` 5/5/1/2/2, `complete` 0/0/0/13/13.
  - order, T8CF facts(`order.sh` → `revfix_order`), 두 번 같은 바이트, `extfix_order` 와 `compare_order.py`: 머리의 다섯 줄(order 개정, relations 개정, `closure:`, `summary edges listed`, `summary spawn calls`)만; 두 쪽에 있는 간선은 HB-spawn 의 `child without success:` 줄과 새 `caller-dependent:` 줄, HB-lock 의 조건 줄의 `child without success: ` 꼬리만 다를 수 있음; 한쪽에만 있는 간선은 HB-seq hard 걸음뿐(나열한 짝이 쓰는 걸음); 대기는 같음; 행은 같은 id·key·차례; 행 안의 짝 줄과 `pairs:` 줄은 바뀔 수 있고 다른 줄은 HB-lock 조건의 그 꼬리만; 두 쪽에 나열된 짝의 값이 바뀌면 옛 값이 hard_before 나 fixed_early_proved 이고 옛 경로에 새 간선 줄이 허락하지 않는 스레드의 caller-dependent HB-spawn 걸음이 있어야 함; 새 글의 나열된 hard_before·fixed_early_proved 짝은 그런 걸음이 없어야 함; 행 값은 그 짝 바뀜에서만 바뀔 수 있음. `edges.tsv`: HB-spawn·L0-script·observed-only 행 같음, HB-lock 행은 조건 칸의 그 꼬리만, HB-seq 행은 한쪽에만 있을 수 있음. `order.log` 빈 파일. [I]: 행 값 바뀜 0/0/0/0/0; 짝 줄이 바뀐 행 103/103/63/18/36; 옛 글의 나열된 hard·fixed 짝 740/740/485/90/162 가운데 허락되지 않은 걸음이 있는 것 572/572/154/0/0, 새 글 0; 모든 짝의 hard_before 5,023 → 1,525(c1 두 빌드), 1,660 → 1,117(r2), fixed_early_proved 2,549 → 1,276, 1,348 → 825, c5 그대로; child without success possible 12/12/9/5/5 → 0, unknown (position-based) 0 → 12/12/9/5/5, not found 1/1/2/1/1 그대로; 검토가 든 짝: c1 B55 의 BP 쓰기 → EVS 사용 unordered, r2 의 `CFE_TIME_TaskMain` 쓰기 → `CFE_ES_BackgroundTask` 사용 둘(옛 `order.txt:3813`, `:4956`) unordered, r2 B37 의 fixed_early_proved 짝에 "BackgroundTask 가 TIME_TaskMain 을 시작" 걸음 없음.
  - 다시 뽑기(`extract.sh` → `revfix_extract`, `env/redesign/one_build.sh`), `extfix_extract` 와: basis.kz·facts.kz 레코드 단위(`check_head.py --whole`: `tool_sha256`, `artifact_sha256`, `basis_sha256`, `parent[0]` 만), facts 보기 바이트 동일, `extract.txt` 1 줄만, `basis.txt`·`sha256.txt` 해시 줄만, `relations.txt` 는 `revfix_relcheck` 와 2 번째 줄 밖 같음, `args.txt`·`relations.log` 같음. 추출 코드를 바꾸지 않으므로 [I].
  - 다시 뽑은 facts 위(`order.sh revfix_extract revfix_on_extract`): order 두 번 같음, `order.txt` 는 `revfix_order` 와 2 번째 줄 밖 같음, `edges.tsv` 같음, `relations.txt` 는 다시 뽑기의 것과 같음, `keys.txt` 는 `revfix_relcheck` 의 것과 2 번째 줄 밖 같음.
  - 계획(`plans.sh` → `revfix_plans`; `plans.sh revfix_extract revfix_plans_on_extract`): `compare_plans.py t8cf extfix_plans/<b>` 와 `extract extfix_plans_on_extract/<b>`: 생성기 정체성 줄, relations 개정이 든 두 줄, sidecar sha256 줄, module 표·실행 파일 목록 첫 줄, args.txt 의 출력 디렉터리(extract 쪽은 facts·basis 입력·artifact 줄과 그 경로)만. site 와 필요 줄은 같다 [I].
  - 생성자 성공 상수의 재측정(`rules-01`): `creator_success.py` 가 설명 스크립트 개정 3 의 함수로 `revfix_relcheck/c1_pre_tx/relations.txt` 의 A20·A21 을 읽는다. [I]: 두 행 모두 `OS_CountSemCreate` 의 표지 붙은 줄 14, `not established`, 성공 상수 없음.
- **공간**: 각 빌드·사본·실행은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 빌드 필요량 = 앞 kz-static 빌드 dip 의 최대 123,981,824 + 새 출력 약 110 MB(다시 뽑기 약 47 MB, order 셋과 둘째 실행 약 40 MB, relations 약 16 MB, 계획 약 2 MB, 여유) + 한 시간 흐름 13 MB = 246,981,824 B. 사본은 필요량 100,000,000.
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh`(앞 단계의 것을 옮김), `compare.sh`, `compare_relations.py`, `compare_order.py`, `compare_plans.py`(옮김), `creator_success.py`, `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`; 빌드 전 점검 `tools_check_pre.txt`, `ledger_check_pre.txt`.

#### 6.0 (나-1) 다시 뽑기 비교 뒤의 덧붙임 (그 비교를 다시 돌리기 전에 적고 커밋함) [D][M]

- **선언 밖의 차이** [M]. `compare.sh extract` 의 첫 실행(`first/compare_extract_first.txt`)에서 다섯 빌드 모두 BAD 셋: (1) facts.kz 의 전체 dump 에서 STRINGS 섹션 줄의 crc32 와 BASIS `extract.units_from` 의 basis 해시(`first/compare/facts_head_<b>.txt`: 다른 줄 둘); (2) facts 보기에서 `extract.units_from = basis …` 한 줄; (3) 다시 뽑기의 `relations.txt` 가 `revfix_relcheck` 와 둘째 줄 밖에서 다름. `compare.sh onextract` 의 첫 실행(`first/compare_onextract_first.txt`)에서 c1 두 빌드와 r2_core 의 `order.txt` 가 `revfix_order` 와 둘째 줄 밖에서 다름. 나머지(basis.kz, extract.txt, basis.txt, sha256.txt, args.txt, relations.log, 두 번 실행, edges.tsv, 키, 계획 열)는 선언 안이었다. 그래서 여기서 멈추고 이 덧붙임을 먼저 커밋한다.
- **까닭**: (1)(2) basis 의 artifact 해시는 도구 해시를 담고, facts 의 BASIS `extract.units_from` 이 그 해시를 인용하며(그래서 STRINGS 의 crc32 도 바뀜), facts 보기가 그 항목을 보인다. 앞 단계들의 선언은 이것을 받았다(§4.0 의 다시 뽑기 허용 "같은 count 의 STRINGS 섹션 줄 crc32 … facts BASIS `extract.units_from` 의 basis 해시", §5.0 (다)의 `extract.units_from`). §6.0 (나)는 "추출 코드를 바꾸지 않으므로"라며 이 둘을 빠뜨렸다. (3)(4) 비교의 기준을 잘못 적었다: T8CF facts 와 다시 뽑은 facts 는 추출 고침 단계 뒤로 다르다(§5.5: 이름 없는 레코드 형의 이름과 그 행). 그래서 다시 뽑은 facts 의 relations·order 는 T8CF 쪽의 것과 같을 수 없고, 같은 facts 의 앞 단계 출력(`extfix_extract` 의 relations.txt, `extfix_on_extract` 의 order.txt·edges.tsv)과 선언한 규칙 차이로 비교해야 한다.
- **고침**(`compare.sh` 의 extract·onextract 단계만): facts.kz 는 새 `check_whole.py`(앞 단계 `check_head.py` 의 허용에 STRINGS 줄의 crc32 와 `extract.units_from` 의 basis 해시만 더함, 다른 칸은 같아야 함); facts 보기는 `extract.units_from` 줄 하나만 다를 수 있음; 다시 뽑기의 `relations.txt` 는 `extfix_extract` 의 것과 `compare_relations.py --facts-line`(둘째 줄의 facts 해시를 더 받음; 나머지 갈래는 §6.0 (나) 그대로); `revfix_on_extract` 의 order 는 `extfix_on_extract` 와 `compare_order.py --facts-line`(머리 둘째 줄을 더 받음; 나머지는 §6.0 (나) 그대로). 두 번 실행의 비교는 첫 실행에서 OK 였고 그때 `again/` 을 지웠으므로 다시 돌릴 때는 첫 실행 기록을 가리킨다. 첫 실행의 기록은 `first/` 에 둔다. 이 덧붙임의 커밋 뒤 두 단계를 다시 돌린다. 선언: 다섯 빌드 모두 OK.

#### 6.0 (나-2) 둘째 비교 뒤의 고침 (다시 돌리기 전에 적고 커밋함) [D][M]

- §6.0 (나-1) 뒤의 둘째 실행(`first/compare_extract_second.txt`)에서 c1_post_tx 의 facts.kz 하나만 BAD 였다: STRINGS 줄의 crc32 가 `0x7c22f22`(일곱 자리)라 `check_whole.py` 의 무늬 `crc32=0x[0-9a-f]{8}` 에 맞지 않았다(`first/compare/facts_whole_c1_post_tx_second.txt`). kz-dump 는 crc32 를 앞의 0 없이 쓴다. 차이 자체는 §6.0 (나-1)이 받은 갈래(STRINGS 줄의 crc32 만)이고 스크립트의 무늬가 좁았다. 고침: 무늬를 `crc32=0x[0-9a-f]{1,8}` 로. 나머지 44 점검과 onextract 25 점검은 OK 였다. 이 고침을 커밋한 뒤 `compare.sh extract` 를 다시 돌린다.

### 6.1 바뀐 것 [F]

- **kairos**(`d75aa53`): `lib/llvm/order.cpp`(order rules revision 2: `chain_call`, `caller_dependent`, `callers_of`, `children_on`, 닫힘의 spawn 마디, HB-lock 의 자식, HB-spawn 의 `caller-dependent:` 줄, `closure:` 머리 줄, `positional_only`, `strength_of`, `child_without_success`, summary 줄), `lib/llvm/relations.cpp`(relations rules revision 9: `callee_returns` 의 `not shown to reach the return` 표지, `callee-check (set)` 줄), fixture `tests/static/fixtures/order.c`(`peers`, `local`), `tests/static/fixtures/relcallback.c`, `tests/static/test_static_cli.cpp`(확인 10 더), `docs/static_facts.md`(§5r 새, §5p·§5q 의 날짜 붙은 줄). golden 아홉(`6dbc9a0`): `relations`, `relations_np`, `relpaths_np`, `relsound_np`, `relincomplete_np`, `relincomplete_cut_np`, `relcallback_np`, `order_np`, `plangen_np.plan.golden`.
- **연구 스크립트**: `env/obs1/explain.py` 개정 3(생성자 callee 의 반환이 다 따라가지지 않았거나 표지가 붙으면 성공 상수 없음), `env/obs1/tests/test_explain.py`(경우 둘 더: 20 → 22).
- **판정 규칙**: `plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` §8.12(새, (ㄱ)–(ㅈ))와 8.2–8.11 끝의 가리키는 줄. 8.0–8.11 의 글은 지우지 않았다.
- **기록의 고침**: `records/order-rules-rev1-20261116/commands.txt` 의 36·40 줄(`builds-7`). 이 결과 문서 §1–§5 의 날짜 붙은 정정 줄(“2026-11-16 검토 뒤 정정: §6.7 <id>”).
- **비교 스크립트**: `records/review-fixes-20261116/extract_compare/`(`compare_classes.py` 개정 2, 허용 파일 셋, `compare_extract.sh`), `keys_check/`, `compare.sh`, `compare_relations.py`, `compare_order.py`, `check_whole.py`, `compare_plans.py`, `creator_success.py`, `measure.py`.
- 바꾸지 않은 것: `include/`, `cmake/`, CMakeLists, 추출 코드, 계획 생성기, 런타임, 계측, lint, 어휘 자료, 2a 드라이버와 `one_build.sh`.

### 6.2 빌드 [M]

MC 에서 제자리, 목표별, 각각 공간 감시 아래(필요량 246,981,824), 모두 첫 시도에 rc 0. 빌드 시작 15:40:04Z(`build_start.txt`), 빌드 전 HEAD `d75aa53`(`pre_mc.txt`, kairos 아래 커밋 안 된 줄 0).

| 목표 | dip (B) | 링크 줄 |
|---|---|---|
| `kz-static` | 97,423,360 | 2 (`libkz-llvm.a`, kz-static) |
| `all` | 6,135,808 | 11 (`libkz-llvm.a` 를 링크하는 시험 프로그램 열, `test_static_cli`) |

- 링크된 이름 13 = `relink_expected.sha256`. 그 가운데 바이트가 바뀐 것 5(`libkz-llvm.a`, kz-static, `test_static_reasons`, `test_static_plan`, `test_static_cli`), 다시 링크되었으나 같은 바이트 8. `sha256sum -c not_relink.sha256` 111 OK(`static_golden` 포함).
- 도구(`post_mc.txt`): kz-static `c6aa4811…` → `6adc1bdd3d7e78ba29d2ea44f316a36e0944d4fdead0a4159fd61f0480e99ec0`; `--identity` 의 `tool_sha256` `c657cb5c…`, `tool_source` `313d8a90…`; `core_source` `568e5061…`, schema fingerprint `6fbff6c6…` 그대로; 나머지 여섯 도구 그대로. flags.make 목록 해시 `95916686…` → `bb6d74c5…`. `CMakeCache.txt` `bc2f43b4…` 그대로. 빌드 시작 뒤 바뀐 소스 파일 0.
- 도구 보존: 빌드 전 kz-static `c6aa4811` 을 `/home/user/work/preserve/tools-20261116-g/kz-static.c6aa4811.xz`(43,266,164 B)로 사본, 왕복 확인(`tool_copy/ROUNDTRIP.txt`: `xz -t` OK, 스트림 sha256 같음, `cmp` 같음). 등록부: `c6aa4811` in-place → copy, `6adc1bdd` in-place.

### 6.3 시험 [M]

- 첫 ctest(`ctest_first.log`): 254 개 가운데 실패 9, 선언한 아홉 그대로(`static_golden` 의 relations 여섯·`relcallback_np`·`order_np`, `test_static_plan`).
- `KZ_GOLDEN_UPDATE=1` 로 아홉을 다시 썼다(`golden_update.log`, 9 통과). 갱신한 golden 의 차이를 §6.0 (나)의 golden 갈래와 맞대어 읽었다: relations 여섯은 개정 줄 하나와 `callee-check (set)` 줄 2/2/0/0/2/3(선언대로); `relcallback_np` 는 fixture 의 바뀜과 `not established (1 returns not followed, 3 constants not shown to reach the return)` 줄; `order_np` 는 행 A1–A3, B1, B2, C1–C5 의 값이 그대로이고 새 행 B3(unordered), `caller-dependent:` 줄 6, `unknown (position-based)` 하나(`summary spawn calls … possible 3, not found 19, unknown 1 (position-based 1)`); `plangen_np.plan.golden` 은 relations 개정이 든 두 줄. 선언의 “scratchpad 의 것과 `cmp`”는 하지 못했다(§6.8).
- 갱신 뒤 전체 ctest(`ctest_full.log`): 254 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음).
- 확인 수는 `ctest -V` 출력으로 남겼다(`ctest_counts.log`): `test_static_cli` 963, `test_static_reasons` 660, `test_static_plan` 182, `test_core_schema` 34,017, `lint_selftest` 109 경우 실패 0, lint 7/7.
- 단위 점검: `env/tools/tests/run_unit.sh` 15/15(점검 15 의 설명 fixture 22/22, `unit_tools.txt`), `env/redesign/step2a_tests_unit.sh` 7/7(`unit_step2a.txt`).

### 6.4 바이트 동일성 [M]

| 비교 | 결과 |
|---|---|
| relations 개정 7 의 다시 뽑기의 키(§6.0 (가) 1, 도구 `038cd9e3`) | `keys_check/keys_check.txt` 26 OK: 다섯 빌드 `keys.txt`·`relations.txt` 가 `relrev7_check` 와 둘째 줄 밖 같음, `relations.txt` = `relrev7_extract` 의 것, 둘째 줄이 다시 뽑은 facts, `relations.log` 빈 파일 |
| 추출 고침 단계의 다시 뽑기 비교 다시 돌림(§6.0 (가) 2, (가-1)) | `extract_compare/compare_extract.txt` 46 OK(도구 확인과 45). 첫 실행은 `extract_compare/first/` |
| relations, T8CF facts (`compare_relations.txt`) | 15 OK: 3 번째 줄, 표지 붙은 줄 94/94/14/18/18, `not established` 5/5/1/2/2, `complete` 0/0/0/13/13, 그 밖 0; `keys.txt` 3 번째 줄만; `relations.log` 빈 파일 |
| order, T8CF facts (`compare_order.txt`) | 20 OK: 두 번 같은 바이트, 선언 밖 0(`compare/order_<b>.txt`), `order.log` 빈 파일 |
| 다시 뽑기 (`compare_extract.txt`) | 45 OK(§6.0 (나-1), (나-2) 뒤): basis.kz 레코드 단위 허용 2, facts.kz 전체 dump 에서 STRINGS crc32 와 `extract.units_from` 만, facts 보기 `extract.units_from` 한 줄, `extract.txt` 1 줄, `relations.txt` 는 `extfix_extract` 의 것과 선언한 갈래만, `basis.txt`·`sha256.txt` 해시 줄만, `args.txt`·`relations.log` 같음. 첫 실행 둘은 `first/` |
| 다시 뽑은 facts 위 (`compare_onextract.txt`) | 25 OK: order 는 `extfix_on_extract` 와 선언한 갈래만, `edges.tsv` = `revfix_order` 의 것, `relations.txt` = 다시 뽑기의 것, `keys.txt` = `revfix_relcheck` 의 것 2 번째 줄 밖 |
| 계획 (`compare_plans.txt`) | 열 모두 “only declared differences”(site 와 필요 줄 같음; 계획된 행도 `extfix_plans` 와 같음) |

- **누적 목록과 등록부**(빌드마다 따로 남김, `builds-6`): 빌드 전 `ledger_check_pre.txt` 76 행 실패 0, `tools_check_pre.txt` 39 행 실패 0; 빌드 뒤 `ledger_check_after_build.txt` 76 행 실패 0, `tools_check_after_build.txt` 40 행 실패 0(등록부를 고친 뒤; §6.8); 다섯 빌드 실행과 장부 행을 더한 뒤 `ledger_check_end.txt` 84 행 실패 0(새 행 여덟: `OBS_revfix_relrev7_extract_keys`, `OBS_revfix_relcheck`, `OBS_revfix_order`, `OBS_revfix_extract`, `OBS_revfix_onextract`, `OBS_revfix_plans`, `OBS_revfix_plans_onextract`, `TOOLS_20261116g`), `tools_check_end.txt` 40 행 실패 0(deep).
- 바깥 이름 점검(`--tree`)은 일곱 출력 디렉터리, 사본 디렉터리, 기록 디렉터리에서 깨끗하다(`name_check_tree.txt`).

### 6.5 측정 [M]

**order rules revision 2**(`revfix_order/<b>/order.txt`, `extfix_order` 와; `measure.txt`, `summary.txt`):

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| 행 값이 바뀐 행 | 0 | 0 | 0 | 0 | 0 |
| 짝 줄이 바뀐 행 | 103 | 103 | 63 | 18 | 36 |
| 모든 짝의 hard_before | 5,023 → 1,525 | 5,023 → 1,525 | 1,660 → 1,117 | 123 그대로 | 195 그대로 |
| 모든 짝의 fixed_early_proved | 2,549 → 1,276 | 2,549 → 1,276 | 1,348 → 825 | 1 그대로 | 1 그대로 |
| 모든 짝의 unordered | 16,868 → 21,639 | 같음 | 5,399 → 6,465 | 4,011 그대로 | 3,931 그대로 |
| 나열한 hard·fixed 짝: 옛 (그 가운데 허락되지 않은 걸음) → 새 | 740 (572) → 242 | 같음 | 485 (154) → 449 | 90 (0) → 90 | 162 (0) → 162 |
| 나열한 짝의 값 바뀜 | hard→unordered 369, hard→fixed 22, fixed→unordered 8 | 같음 | hard→unordered 18, fixed→unordered 7 | 0 | 0 |
| 나열한 간선 (HB-seq) | 479 → 308 (402 → 231) | 같음 | 517 → 498 (477 → 458) | 91 그대로 | 121 그대로 |
| 닫힘이 따라간 HB-seq 걸음 | 30,956 → 30,824 | 같음 | 15,067 → 15,030 | 8,763 그대로 | 8,760 그대로 |
| `caller-dependent:` 줄이 붙은 HB-spawn 간선 | 53 | 53 | 30 | 6 | 6 |
| child without success possible / not found / unknown (position-based) | 12/1/0 → 0/1/12 (12) | 같음 | 9/2/0 → 0/2/9 (9) | 5/1/0 → 0/1/5 (5) | 같음 |

- 값이 바뀐 짝은 모두 옛 경로에 새 간선 줄이 허락하지 않는 caller-dependent HB-spawn 걸음이 있었고, unordered·unknown 이던 짝이 순서를 얻은 일은 없다(`compare_order.py` 의 점검 (b)). 새 글의 나열된 hard_before·fixed_early_proved 짝에는 그런 걸음이 없다(점검 (a)).
- 검토가 든 짝: c1_pre_tx 의 BP 쓰기(`cfe_es_syslog.c:219:9`, BP_AppMain) → EVS 사용(`cfe_evs_log.c:207:17`) hard_before → unordered; r2_core 의 옛 `order.txt:3813`(TIME 쓰기 → BackgroundTask 사용), `:4956`(`os-impl-tasks.c:614:9` 의 TIME 쓰기 → BackgroundTask) hard_before → unordered; r2_core B37 의 fixed_early_proved 짝 가운데 “BackgroundTask 가 `CFE_TIME_TaskMain` 을 시작”(E73f4411c15f2c30c) 걸음을 쓰는 것 1 → 0. B37 의 남은 fixed_early_proved 경로는 ES 의 `CFE_ES_TaskMain` 이 제 BackgroundTask 를 spawn 하기 전의 사용이다(ES 가 만드는 스레드, 허락된 걸음).
- 행 값은 하나도 바뀌지 않았다: 나열한 짝의 값이 바뀐 행(c1_pre_tx 80, r2_core 15)은 모두 이미 unordered 였다. r2 의 hard_before 34·fixed_early_proved 4 행은 그대로다. 측정이고 판정이 아니다.

**relations rules revision 9**: 표지 붙은 줄 94/94/14/18/18, `callee-check (set)` `not established` 5/5/1/2/2, `complete` 0/0/0/13/13. 생성자 성공 상수의 재측정(`creator_success.txt`, 설명 스크립트 개정 3 의 함수로): `revfix_relcheck/c1_pre_tx` 의 A20·A21 은 `OS_CountSemCreate` 의 표지 붙은 줄 14, 받은 상수 둘(모두 실패 등급), `returns not followed`, 생성자 out 은 읽지 않음(성공 상수 없음). 같은 스크립트로 개정 7 의 relations(`relrev7_check`)를 읽어도 `returns not followed` 줄 때문에 성공 상수 없음이다: 앞 단계의 `records/relrules-rev7-20261116/creator_success.txt` 는 개정 2 의 규칙을 손으로 적용해 `(0, OS_SUCCESS)` 를 냈다.

**추출 고침 단계의 비교 다시 돌림**(`extract_compare/compare/facts_<b>.txt`; 옛 표 §5.5 와 같은 출력, 갈래만 다시 셈):

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| renamed | 249 | 249 | 176 | 39 | 39 |
| successor(짝을 얻은 뒤를 잇는 줄 가운데 선언한 칸이 다른 것) | 17 | 17 | 10 | 0 | 0 |
| added-named | 151 | 151 | 12 | 2 | 2 |
| removed-anon-paired | 54 | 54 | 42 | 24 | 24 |
| 짝 목록 줄 | 183 | 183 | 157 | 60 | 60 |
| mask: site / row / count-head / and-more | 41,919 / 1,634 / 10 / 1 | 41,932 / 1,634 / 10 / 1 | 20,983 / 264 / 8 / 0 | 0 / 114 / 3 / 0 | 0 / 114 / 3 / 0 |

옛 표의 renamed + added-named(417/417/198/41/41)와 새 표의 renamed + successor + added-named 는 같다: 줄은 같고, 짝을 지어 이름만 바뀐 줄과 칸이 바뀐 줄로 다시 나눈 것이다. 가림 합(43,564/43,577/21,255/117/117)도 옛 표와 같다.

### 6.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 14:58:08 | 2,805,444,608 | 키 점검 시작(dip 187,404,288; 푼 바이너리는 끝에 지움) |
| 15:02:13 | 2,851,311,616 | 다시 뽑기 비교 다시 돌림 시작(dip 52,768,768) |
| 15:37:12 | 2,306,027,520 | 선언 커밋 전(scratchpad 점검 프로그램이 있을 때) |
| 15:39:49 | 2,805,026,816 | 사본 끝(dip 43,290,624; 그 전에 scratchpad 점검 프로그램을 지움) |
| 15:40:42 | 2,807,885,824 | `make kz-static` 끝(가장 낮은 2,707,582,976, dip 97,423,360) |
| 15:42:55 | 2,803,281,920 | `make all` 끝(dip 6,135,808) |
| 15:47:36–15:51:08 | 2,794,598,400 → 2,716,418,048 | relations(dip 7,675,904), order(14,209,024), 다시 뽑기(47,157,248), 그 order(21,860,352), 계획 둘(987,136 씩) |
| 이 절을 쓸 때 | 2,722,320,384 | 비교와 장부 뒤 |

- 하한(314,572,800)을 넘지 않았다. 빌드 dip 은 필요량 안이다. 키 점검의 dip 은 선언한 필요량 60,000,000 보다 컸다(§6.8).
- 남긴 출력: `revfix_relrev7_extract_keys` 7,564,932, `revfix_relcheck` 7,630,029, `revfix_order` 7,058,482, `revfix_extract` 46,967,397, `revfix_on_extract` 14,689,595, `revfix_plans` 886,411, `revfix_plans_on_extract` 886,913 B(장부 행), 사본 43,266,328 B. 지운 것: 비교 뒤의 둘째 실행(`again/`), facts 보기(scratchpad, 비교 뒤 바로), 키 점검이 푼 바이너리, scratchpad 의 점검 프로그램과 그 출력뿐이다.

### 6.7 지적마다: 무엇이 틀렸고, 무엇을 바꿨고, 무엇을 다시 돌렸는가

| id | 무엇이 틀렸나 | 무엇을 바꿨나 | 다시 돌린 것과 증거 |
|---|---|---|---|
| `rules-01` | relations 개정 7 의 A20·A21 에서 `OS_CountSemCreate` 의 `0 [OS_SUCCESS]` 줄 넷은 모두 `OS_ObjectIdAllocateNew` 사슬이고, 그 결과는 성공이면 `osapi-countsem.c:103`, `:106` 에서 다시 정의되어 `:109` 에 닿지 않는다. 그 줄로 “생성자 성공 상수를 주었다”(§2.5 :176–177, §2.8 :222), “돌려줄 수 있는 상수”(§2.7 :214)라 적었다 | relations rules revision 9(`relations.cpp`): `redefined-later` 정의를 거친 상수에 `; not shown to reach the return: …`, 생성자 callee 마다 `callee-check (set)` 줄. 설명 스크립트 개정 3. 결과 §2.5, §2.7, §2.8 의 정정 줄 | 빌드 뒤 relations 다섯(`compare_relations.txt` 15 OK, 표지 94/94/14/18/18); `creator_success.txt`: A20·A21 성공 상수 없음(개정 9 와 개정 7 의 relations 모두) |
| `rules-02` | 기초 4 가 callee 의 상수 집합의 완전성을 요구하지 않아, 개정 7 의 불완전한 집합으로도 섰다 | 판정 규칙 8.12 (ㄱ)(8.2, 8.4, G10): 따라가지 못한 반환·`not established`·표지가 없을 때만 기초 4, 아니면 `record_insufficient(no success constant of the creator: the callee's returns are not all followed)`. `explain.py` 개정 3 의 `publisher_out` | `test_explain.py` 22/22(새 경우 `derived_met_open`, `derived_met_mark`), `run_unit.sh` 15/15; `creator_success.txt` |
| `rules-03` | 8.6 (a)의 같은 행위자 대조가 8.2 의 범위로는 A20 에서 생길 수 없다(생성자가 남지 않음); §2.5 :177 은 그 줄이 생겼다고 적었다 | 8.12 (ㄴ): 존재 탐침 행의 대조에서 c 는 `probe-create` 로 뺀 생성과 `key constants differ` 로 뺀 생성, 결과는 대조 기록에만; 8.6 (a)의 [I] 는 이 범위를 조건으로. §2.5 의 정정 줄 | 이름 결합을 구현한 설명 스크립트 판이 아직 없어 돌린 것 없음(덧붙임은 어느 판보다 먼저 커밋: `d75aa53`) |
| `rules-04` | 개정 7 뒤의 계획에 C1 관계 밖의 경로 A 행이 없어, 8.0 (2)·8.6 의 안전장치(사전 시험)가 서지 않는데 적지 않았다 | 8.12 (ㄷ): C1 관계 밖의 경로 A 행과 A20 을 기록된 예산 아래 넣은 계획의 부팅만 사전 시험. §1.4 의 정정 줄 | `revfix_plans` 의 계획된 행을 셈: 경로 A 는 c1_pre_tx 의 A21 하나(`extfix_plans` 와 같음) |
| `rules-05` | 한 기록에 사용 사건이 여럿일 때 `link.status` 의 집계가 없고, 8.5 의 `not_observed_in_runs` 논증이 u 하나에 기댄다 | 8.12 (ㄹ): 기록 단위 집계, 섞인 바탕의 하위 사유 `link_unlinked(name binding: the uses of the record differ in basis)`, 서식 검사 12 | 돌린 것 없음(규칙 글; 구현은 §6.9) |
| `rules-06` | relations 의 정적 키 상수 배제가 8.3 의 “키 글 차이만으로 refuted 를 내지 않음”과 어긋나는데 8.7·8.11 에 없었다 | 8.12 (ㅁ): 후보 거르기로만 남고 refuted 로 읽지 않으며 생성자 site 를 줄이지 않음; 8.7 의 입력, 8.11 의 남은 문제 14 | 돌린 것 없음; A20 의 계획 행 `order=NOPAIR … state=cut` 확인 |
| `rules-07` | (라)의 증거가 “정적 또는 실행”인지 “정적과 실행”인지 모호; §2.8 은 표 선택을 준 것으로 셌다 | 8.12 (ㅂ): 둘 다 필요; 개정 7 은 정적 반쪽만. §2.8 의 정정 줄 | 돌린 것 없음 |
| `rules-08` | 기초 6, 모순, 여럿 규칙에 1차 근거가 없고, 기초 6 의 경계가 경로 A 순서 규칙과 다른 까닭이 없었다 | 8.12 (ㅅ): G15–G17(`osapi-idmap.c:975`, `:1158`, `:828-863`, `:1170`, `:1177-1180`)과 경계의 까닭. 조회의 잠금 줄은 `:975` 다(지적의 `:974` 는 그 앞 줄) | 소스 대조(묶음 1df262a) |
| `rules-09` | refuted 쌍의 (마) 구간이 c 가 u 앞일 때 정의되지 않았다 | 8.12 (ㅇ): 앞선 사건의 at-call 부터 나중 사건의 after-call 까지(8.2, 8.3) | 돌린 것 없음 |
| `rules-10` | 8.7 셋째 줄이 종류별 writer 나눔을 relations 개정 7 에 맡겼는데, 그 단계가 주지 않았고 날짜 붙은 줄이 없었다 | 8.12 (ㅈ): 뒤 추출 단계(OBJECT_ACCESS 를 채우는 단계)가 맡음; 그때까지 (다)는 `link_unlinked(…)`. §2.8 의 정정 줄 | 개정 9 의 A20·A21 `writers:` 줄이 여전히 나누지 않은 한 줄임(relations 비교의 그 밖 0) |
| `builds-1` | 추출 고침 단계의 빌드 2 뒤 첫 ctest 에서 `test_static_reasons` 가 실패했는데 덧붙임 §5.0a 가 첫 실패를 적지 않았고, §5.7 의 선언 밖 목록에도 없었다 | §5.3, §5.7 의 정정 줄. 이 단계의 선언 §6.0 (나)는 첫 ctest 의 실패 아홉을 이름으로 적었고 그대로였다 | `ctest_first.log`: 실패 9, 선언 그대로 |
| `builds-2` | 그 단계의 facts 보기 비교가 짝 없는 옛 `<anonymous>` 줄과 새 이름 줄을 짝 없이 받았다 | `extract_compare/compare_classes.py` 개정 2(뒤를 잇는 줄과 짝짓기, 칸 허용은 `pair` 행), `facts_allow.tsv`, `extract_txt_allow.tsv`(§6.0 (가-1)). §5.4, §5.5 의 정정 줄 | `extract_compare/compare_extract.txt` 46 OK; 첫 실행(`first/`)은 `extract.txt` 셋 BAD → 덧붙임 → 다시 OK |
| `builds-4` | relations 개정 7 의 선언이 요구한 다시 뽑기의 키 비교를 덧붙임 없이 뺐다 | `keys_check/keys_check.sh`(보존 사본 `038cd9e3` 을 풀어 돌림). §2.0, §2.4, §2.7 의 정정 줄 | `keys_check/keys_check.txt` 26 OK; 출력 `revfix_relrev7_extract_keys`(장부 행) |
| `builds-5` | schema 개정 12 의 다시 링크 셈: 75 는 도구 넷·시험 70·`libkz-core.a` 인데 “시험 71”이라 적었고, 83 = 75 + 여덟 + 하나는 84 다 | §4.0, §4.2 의 정정 줄: 83 = 75 + 다른 정적 라이브러리 일곱 + `test_trace_neutral_rev12` | `records/schema-rev12-20261116/relink_expected.sha256` 의 줄을 셈(1 줄 `libkz-core.a`, 끝 넷 도구) |
| `builds-6` | 빌드가 둘인 두 단계에서 누적 목록·등록부를 빌드 1 과 빌드 2 사이에 돌리지 않았다 | §3.4, §5.4 의 정정 줄. 이 단계는 빌드 전·빌드 뒤·끝에 따로 남김 | `ledger_check_pre/after_build/end.txt`, `tools_check_pre/after_build/end.txt` 모두 실패 0 |
| `builds-7` | order 단계의 명령 기록 36·40 줄이 덧붙임과 빌드 2 전 HEAD 를 `c023b11` 로 적었다(실제 `6ae9a41`) | `records/order-rules-rev1-20261116/commands.txt` 36·40 줄을 `6ae9a41` 로, 날짜 붙은 괄호 | `pre_mc_build2.txt:30` `HEAD=6ae9a41`, `git log` 의 두 커밋 시각 |
| `builds-8` | 몇 단계의 확인 수가 도구 출력 없이 `commands.txt` 의 글로만 있었다 | §2.3, §3.3, §5.3 의 정정 줄(콘솔에서 읽었고 남기지 않음). 추출 고침 단계의 660 은 빌드 전 같은 바이너리로 다시 재 남김; 이 단계는 `ctest_counts.log` | `counts_before_build.log`: `test_static_reasons` 660; `ctest_counts.log` |
| `builds-9` | 그 단계의 relations 허용 `head-C` 가 `(spawn calls ` 뒤를 `.*` 로 받았다 | `extract_compare/relations_allow.tsv`: 머리의 글 그대로, 수만 `\d+`. §5.4 의 정정 줄 | `extract_compare/compare_extract.txt` 의 relations 다섯 OK |
| `builds-10` | `compare_classes.py` 개정 1 이 가린 줄에 두 줄을 다르게 남기는 첫 가림의 이름을 붙여, 갈래 표가 `mask:site` 로 몰렸다 | 개정 2: 그 줄에 작용한 가림의 이름으로 셈. §5.5 의 정정 줄 | 새 갈래 표(§6.5): 가림 합은 옛 표와 같고 site/row/count-head/and-more 로 나뉨 |
| `semantics-1` | order 닫힘이 공유 spawn 호출에서, 닿은 스레드와 무관하게 그 호출의 모든 자식으로 가서 소스로 틀린 hard 경로와 거짓 hard_before·fixed_early_proved 짝을 냈다(c1 BP→EVS, r2 TIME→BackgroundTask, r2 B37) | order rules revision 2: 호출자에 기대는 spawn 호출(`children_on`), HB-lock 의 자식, `caller-dependent:` 줄; fixture `peers`. §3.5, §3.7, §3.8 의 정정 줄 | 빌드 뒤 order 다섯(`compare_order.txt` 20 OK, 새 글의 허락되지 않은 걸음 0), 다시 뽑은 facts 위(`compare_onextract.txt`), `measure.txt`(검토가 든 짝 모두 unordered, B37 의 그 걸음 1 → 0) |
| `semantics-2` | child without success `possible` 12/12/9/5/5 가 거짓 증거(`pthread_create` 실패 갈래, 다시 정의된 `AllocateNew` 의 값, spawn 을 건너뛴 갈래의 상수)에 섰고 출력에 단서가 없었다 | order rules revision 2: 증거가 위치만으로 서면 `unknown (position-based)`, summary 에 그 수; fixture `local`. §3.5, §3.7, §3.8 의 정정 줄 | 빌드 뒤 order 다섯: possible 0, unknown (position-based) 12/12/9/5/5, not found 1/1/2/1/1 |
| `semantics-3` | C1 의 `CFE_SB_MemParams_t::Partition.Data`, `CFE_TBL_BufParams_t::Partition.Data` 의 `writer-set-complete writers=1` 은 배열이 포인터로 바뀌어 풀 코드로 넘어가는 탈출을 세지 않아 소스가 뒷받침하지 않는다 | 추출 규칙은 고치지 않음(다섯 빌드의 다시 뽑기를 바꾸는 뒤 추출 단계의 일, §6.9). `static_facts.md` §5r 의 알려진 한계, §5.5·§5.8 의 정정 줄 | 소스 대조(`cfe_sb_init.c:104`, `cfe_tbl_internal.c:105`, `cfe_es_mempool.c:313`, `:84-85`); 이 단계의 다시 뽑기도 그 행이 같음(facts 레코드 같음, §6.4) |
| `semantics-4` | 묶음 루트 밖 파일의 형 이름이 절대 경로를 담아 c1_pre_tx 와 c1_post_tx 에서 다르다; §5q 의 “같은 이름”은 한 추출 안의 말인데 적지 않았다 | `static_facts.md` §5q 의 날짜 붙은 줄과 §5r 의 알려진 한계; §5.5 의 정정 줄. 위치를 상대 경로로 쓰는 고침은 하지 않음(§6.9) | `extract_compare/compare/facts_c1_{pre,post}_tx.txt` 의 새 이름 목록(`item@/home/user/work/procrace/c1_build/{pre,post}/…`) |

### 6.8 계획과 다르게 한 것

- **덧붙임 셋으로 멈췄다**(§6.0 (가-1), (나-1), (나-2)). 각각 비교의 첫 실행에서 선언 밖의 차이가 나왔고, 까닭을 적은 덧붙임을 커밋한 뒤 같은 비교를 다시 돌렸다(`be9a94b`, `6dbc9a0`, `4e5a267`). 첫 실행의 기록은 `extract_compare/first/`, `first/` 에 남겼다. 출력은 어느 것도 다시 만들지 않았다(비교 스크립트와 허용만 바뀜).
- **키 점검의 dip**(187,404,288 B)은 선언한 필요량(60,000,000)보다 컸다: 선언이 사본에서 푼 바이너리(약 185 MB)를 필요량에 넣지 않았다. 감시 문턱(374,572,800) 위였고 하한은 넘지 않았다.
- **갱신한 golden 을 scratchpad 의 것과 `cmp` 하지 못했다**: 빌드 전에 scratchpad 의 점검 출력(golden 사본 포함)을 공간 때문에 지웠다. 대신 갱신한 golden 의 `git diff` 를 선언의 golden 갈래와 하나씩 맞대어 읽었다(§6.3).
- **빌드 뒤 첫 `tools_check.sh --deep`** 은 39 행 가운데 1 실패였다(덮인 `c6aa4811` 이 아직 in-place 로 등록됨). 등록부를 고친 뒤 다시 돌린 출력이 같은 파일 이름(`tools_check_after_build.txt`)을 덮어 첫 출력은 남지 않았다; `commands.txt` §3 에 적었다.
- **닫힘 규칙을 선언 전에 한 번 넓혔다**: scratchpad 점검에서 r2_core 의 `OS_TaskCreate → OS_TaskCreate_Impl` 이 spawn 호출이 아니어서 `:4956` 이 남았고, “spawn 연산의 사슬에 든 호출”로 넓힌 뒤 선언을 적었다(`commands.txt` §1). 선언 뒤에는 코드를 바꾸지 않았다.
- **§6.0 (나)의 다시 뽑기 비교의 기준을 잘못 적었다**(T8CF 쪽 출력): (나-1)에서 고쳤다.
- **이름 결합을 구현하지 않았다**: `rules-03`, `rules-05`, `rules-06`, `rules-09` 는 규칙 글의 덧붙임이고, 그것을 읽는 설명 스크립트 판은 없다(개정 3 은 기초 4 의 생성자 쪽만).

### 6.9 열린 것

- **`semantics-3` 의 추출 고침**: 배열에서 포인터로의 변환으로 호출 인자에 넘어가거나 정수로 바뀐 대상 필드를 주소 탈출(또는 알 수 없는 효과)로 세는 것. 다섯 빌드의 facts 를 바꾸므로 뒤 추출 단계에서 선언·빌드·다시 뽑기와 함께 한다. 그때까지 그 두 행의 complete 는 소스가 뒷받침하지 않는다.
- **`semantics-4` 의 이름 안정**: 묶음 루트 밖 파일의 위치를 앱 트리의 소스 루트 기준으로 쓰는 것. 이름 결합의 실행 사이 대응과 빌드 사이 비교 단계가 이 이름을 쓰기 전에 정한다.
- **이름 결합의 구현**(8.2–8.5, 8.12 (ㄴ)(ㄹ)(ㅁ)(ㅇ))과 설명 스크립트의 여러 사용 사건 집계(지금 `use_events[0]` 만 읽음). 구현 판은 8.12 를 따르고 서식 검사 12 를 더한다.
- **키 상수 배제로 빠진 생성자 site 의 계획**(8.12 (ㅁ), 8.11 의 14): 계획 생성기가 `not a pair: key constants differ` 줄의 생성 호출을 site 로 넣어야 8.3 (2)가 선다.
- **사전 시험의 계획**(8.12 (ㄷ)): C1 관계 밖의 경로 A 행과 A20 을 기록된 예산 아래 넣은 계획. 지금 계획들의 경로 A 계획된 행은 A21 하나다.
- **생성자 성공 상수**: 개정 9 는 그 상수를 서지 않게 했을 뿐 주지 않는다. `OS_CountSemCreate` 의 성공 반환(`OS_ObjectIdFinalizeNew` 의 매개변수 반환)을 따라가려면 지역의 정의를 호출 결과에 잇는 사실이 필요하다(order 의 위치 근거와 같은 빈칸, §3.8).
- 호출자에 기대는 spawn 호출의 규칙은 덜 잡는 쪽이다(풀린 entry 를 넘기지 않는 호출자는 아무것도 더하지 않음). 그래서 새 unordered 가운데 실제로는 순서가 있는 짝이 있을 수 있다 [미확인].

### 6.10 lead 의 판단 (2026-11-16, 검토 뒤 고침을 읽고)

이 묶음(§1–§6)의 끝에 lead 가 정한 것이다. 모두 기술 판단이고, 목표·범위·공개·고유 증거·새 외부 자원에 닿는 것은 없다.

1. **이 묶음의 끝 판정.** §1–§5 의 단계는 끝났고 확인된 지적 23 개는 §6.7 대로 고쳤다. 순서 간선의 짝 수는 §3.5 대신 §6.5(순서 규칙 개정 2)를 쓴다. C1 관계 1 의 머리글은 그대로다: `violation = undetermined`, `odf = failure_sign`, `link = unlinked`(이름 결합은 규칙 글만 있고 구현이 없으며, 기초 4 의 생성자 성공 상수도 아직 주어지지 않는다).

2. **사전 시험의 계획 (`rules-04`, 8.12 (ㄷ)).** 대조 행을 이름(행 id)으로 고르지 않고 규칙으로 고른다. 관측 계획 생성기의 다음 판(manifest 개정 4)에 경로 A 대조 선택 규칙을 둔다: (가) `handling: probe-create` 인 경로 A 행 모두(같은 행위자 대조), (나) 생성자 site 가 하나라도 있는 경로 A 행 모두(8.12 (ㅁ)에 따라 `not a pair: key constants differ` 로 뺀 생성 호출도 site 로 셈). 이 행들은 층 차례와 따로 계획에 들고, 그 예산은 계획을 만들기 전에 sidecar 에 적는다. 선택 규칙의 글은 어느 계획보다 먼저 커밋하고, 행마다의 기대([I])는 부팅 전에 커밋한다. 생성기는 행 id·키·사례 값을 받지 않는다. 예산이 그 행들을 담지 못하면 그 부팅은 사전 시험이 아니며, 그렇게 적고 연결 결론은 `unlinked` 로 둔다. C1 의 A20 은 (가)로 들어가고, C5 주기의 계획에도 같은 규칙이 c5_641 의 경로 A 행에 적용된다.

3. **기초 사유와 하위 사유.** 8.10 의 선택지 가운데 지금은 하위 사유(`link_unlinked(name binding: …)`)를 유지한다. 3.5 의 규칙(표에 없는 원인은 가장 가까운 사유의 하위 사유로 쓴다)과 맞고, 어휘 파일과 그 시험을 바꾸는 빌드가 필요 없다. 사유별로 세어야 하는 설명 판이 생기면 그때 8.10 의 차례(문서 → 같은 선언 빌드에서 어휘 파일·시험 수·코드)를 따른다.

4. **열린 것의 자리.** §6.9 와 §3.8 의 열린 것을 뒤 단계에 이렇게 둔다.
   - 배열에서 포인터로의 탈출(`semantics-3`)과 묶음 루트 밖 형 이름의 안정(`semantics-4`): 형과 객체 접근 단계. 이름은 앱 트리의 소스 루트 기준 위치로 쓰고, 그 단계의 선언에 c1_pre_tx·c1_post_tx 사이의 같은 정의가 같은 이름을 갖는지를 점검으로 넣는다.
   - 지역의 정의를 호출 결과에 잇는 사실(생성자 성공 상수, child without success 의 위치 근사), 호출의 guard(HB-await 의 호출자 쪽 간선, 예산 탈출 뒤 사용): 결과 흐름 단계. 지역 플래그 술어(loop 177, CF 재시도 대기의 `ret`): 루프 술어·잠금 정체성 단계.
   - 호출자에 기대는 spawn 규칙의 덜 잡음([미확인]): 루프 술어·잠금 정체성 단계의 마지막 회차에서, 풀린 entry 를 넘기지 않는 호출자의 수와 그 때문에 unordered 로 남은 짝의 수를 잰다.
   - 키 상수 배제로 빠진 생성자 site 의 계획: manifest 개정 4 의 생성기.
   - 이름 결합의 구현과 한 기록의 여러 사용 사건 집계: 설명 스크립트 개정 4. 이름 공간 항목(공통 의미 단계)과 이름 수명(기록 분석 단계)이 생긴 뒤, C1 주기의 설명 전에 한다.

5. **실천의 좁힘.** 이 묶음에서 드러난 어긋남(`builds-1`, `builds-6`, `builds-8`, §4.7 의 감시 없는 점검 빌드, §6.8 의 키 점검 dip)에 따라 다음 단계부터: scratchpad 의 점검 빌드도 감시 아래 둔다; 누적 목록과 등록부 점검은 빌드마다 돌린다; 첫 ctest 에서 실패할 시험은 이름으로 선언한다; 보고하는 시험 수는 남긴 도구 출력에서만 가져온다; 선언하는 공간 필요량에는 사본에서 푸는 바이너리를 넣는다; 점검 출력을 덮어쓰지 않는다(새 이름).

## 7. 형과 객체 접근: 추출 옵션 `--object-access` (kz-static 빌드 하나)

추출에 옵션 하나를 더한다: schema 개정 12 가 정의만 해 둔 OBJECT_ACCESS·FIELD_TYPES·ARG_TYPES 를 쓰고, 그 옵션 아래에서 배열이 포인터로 바뀌는 대상 필드를 writer 집합에 세며(§6.9 첫 항목, §6.10 의 4), 묶음 루트 밖 파일의 형 이름을 소스 트리 기준 위치로 쓴다(§6.9 둘째 항목, §6.10 의 4). 계획 생성기는 문자 배열 키 인자의 크기를 키 글의 상한에 쓴다(plan rules revision 2). 기록: `../records/object-access-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `objacc_*`. 도구 사본: 새 디렉터리 `/home/user/work/preserve/tools-20261116-h/`. T8CF 와 앞선 출력 디렉터리에는 쓰지 않는다. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 7.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다.

- **무엇을 하는가** (`kairos/docs/static_facts.md` §5s, `kairos/docs/plan_manifest.md` 의 revision 2):
  1. **옵션** `kz-static extract --object-access`(확장 사실과 함께만; 없으면 종료 코드 2). 옵션이 없으면 추출의 코드 경로와 출력은 앞판 그대로다(새 계수·수집·섹션은 모두 옵션 아래).
  2. **배열이 포인터로 바뀌는 대상 필드**(옵션 아래, 모든 추출 차례): 호출 인자로 가면 `&x.f` 와 같은 out 포인터 호출(callee 요약: 저장만 하면 writer 행, 탈출·본문 없음·간접 호출이면 그 필드의 주소 탈출 A-TYPE 행), 정수 변환·포인터 저장·초기화·반환·그 밖의 쓰임은 주소 탈출 A-TYPE 행, 원소에 대한 저장은 그 필드의 writer 행(partial), 시험과 원소 읽기는 셈 밖. 데이터 함수의 목적지·원천은 그 요약 그대로.
  3. **묶음 루트 밖 파일의 형 이름**(옵션 아래): 위치를 그 파일의 소스 트리(위로 가장 가까운 `.git` 항목이 있는 디렉터리) 기준 `<트리>/<경로>` 로. 어느 트리에도 없는 파일은 절대 경로 그대로. 함수·객체 키는 바꾸지 않는다.
  4. **섹션 셋의 writer**(옵션 아래): OBJECT_ACCESS(READS 행마다, 이어 WRITERS 행마다: 바탕 종류·바탕 객체·경로·첨자·상태), FIELD_TYPES(읽기가 이름 대는 레코드 형의 필드마다: 원소 종류·크기, 배열 크기와 그 글, 비트 폭, 한정자), ARG_TYPES(호출 인자마다: 형의 종류·폭; 문자 배열이면 크기·선언·그 배열을 읽는 READS 행이 호출을 지배하는지, 잠금 집합 골격의 블록 지배). BASIS 항목 여섯(`extract.object_access`, `count.object_access`, `count.field_types`, `count.arg_types`, `count.array_decay`, `count.type_names_outside_root`). `kz-static show` 의 절 `objaccess`, `ftypes`, `argtypes`.
  5. **plan rules revision 2**: use-call·creator-call 의 키 인자가 크기 있는 문자 배열이고 호출을 지배하는 읽기가 있으면(ARG_TYPES) 그 at-call 의 키 값을 `string=validated bound=min(크기, 64)` 로, sidecar 에 `bound string-extent source=ARG_TYPES` 한 줄. ARG_TYPES 없는 facts 의 계획은 개정 글 두 줄 밖에서 앞판 그대로.
- **레이아웃 점검.** 쓰는 코드가 개정 12 의 세 레코드(`kz_fact_objacc` 72 B, `kz_fact_ftype` 60 B, `kz_fact_argtype` 44 B)와 그 열거형에 그대로 맞는다(필드를 더하거나 바꾸지 않음). 그래서 `include/` 바뀜은 더하지 않는다(§4.7 의 [I] 를 이 셋에 대해 잰 것).
- **1차 근거.**
  - 배열의 포인터 변환: C11 6.3.2.1 p3(배열 형의 lvalue 는 sizeof·`&`·문자열 초기화 밖에서 첫 원소의 포인터로 바뀐다). 앞판의 빈칸: `lib/llvm/ast_fn.cpp` `classify_lvalue_use` 는 암묵 변환을 만나면 Read 로 멈추고(그 함수의 부모 따라가기, `ImplicitCastExpr` 를 다루지 않음), 호출 인자의 out 포인터 행은 인자 종류 ADDR_FIELD(`&x.f`)에서만 만든다(`visit_call` 의 "calls with the address of a target field"). C1 의 두 행: `cfe_sb_init.c:104`(`CFE_ES_PoolCreateEx` 에 `CFE_SB_Global.Mem.Partition.Data`), `cfe_tbl_internal.c:105`(`CFE_ES_PoolCreate` 에 `CFE_TBL_Global.Buf.Partition.Data`), 풀 코드가 정수 주소로 그 안에 씀(`cfe_es_mempool.c:313`, `:84-85`)(§5.5 의 정정 줄, 묶음 1df262a).
  - 형 이름: 앞판 `TuReader::name_pos` 는 `pos_of` 의 기록된 경로를 그대로 썼고 루트 밖은 절대 경로다(`lib/llvm/ast_tu.cpp` `path_of`). c1_pre_tx·c1_post_tx 의 CF 소스는 묶음 루트 밖의 두 checkout `…/c1_build/pre/src/cf`, `…/c1_build/post/src/cf` 이고 둘 다 `.git` 이 있다(`fetch_refs.record` 의 `pre/src/cf 281a9418…`, 같은 줄의 post). 두 트리에서 바뀐 파일은 `cf_platform_cfg.h`, `cf_cfdp.c`, `unit-test/cf_cfdp_tests.c` 셋이다(`diff -rq`, 이 단계 전 [M]).
  - 키 글의 상한: 런타임은 NUL 이나 상한까지 복사하고 잘림을 표시한다(`kairos/runtime/src/kz_rt.c:468-482`, `KZ_RT_STR_MAX` 64, `kz_rt.h:82`). CF 의 키는 `char sem_name[OS_MAX_API_NAME]`(`cf_tbldefs.h:74`)이고 호출 앞에서 `sem_name[0]` 을 시험한다(`cf_cfdp.c:991`, 호출 `:993`). 상한을 배열 크기 안에 두면 복사가 프로그램 스스로 읽는 배열 밖으로 나가지 않는다.
- **코드**(`kairos/`): `lib/llvm/{extract.hpp,spec.hpp,model.hpp,ast_reader.hpp,ast_tu.cpp,ast_fn.cpp,finalize.cpp,serialize.cpp,facts_text.cpp,plan_gen.cpp}`, `tools/kz-static/{cmd_extract.cpp,main.cpp}`; 시험 `tests/static/{test_static_cli.cpp,test_static_plan.cpp,static_golden.cpp,CMakeLists.txt}`, 새 fixture `tests/static/fixtures/objaccess.c`, `plankey.c`, 새 자료 `tests/static/data/plankey_framework.txt`; 문서 `docs/static_facts.md` §5s(와 §5r 의 날짜 붙은 줄), `docs/plan_manifest.md`(revision 2), `docs/schema.md` §20 의 날짜 붙은 줄.
  - 하지 않는 것: `include/`, `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `instrument/`, `tests/lint/`, relations·order 의 규칙(개정 9 와 2 그대로), 판정 규칙과 어휘 자료, 설명 스크립트, 2a 드라이버와 `one_build.sh`(옵션은 그 `ONE_BUILD_EXTRACT_EXTRA` 로 준다).
- **도구 해시와 정체성**: kz-static 의 파일 sha256(빌드 전 `6adc1bdd3d7e78ba29d2ea44f316a36e0944d4fdead0a4159fd61f0480e99ec0`), `--identity` 의 `tool_sha256`(빌드 전 `c657cb5c…`)·`tool_source`(빌드 전 `313d8a90…`)가 바뀐다. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 는 그대로 [I](`include/`·`cmake/`·최상위 CMakeLists 를 바꾸지 않음, `cmake/KzSourceIdentity.cmake` 의 `kz_tool_identity`). 나머지 여섯 도구의 정체성은 그대로.
  - 다시 링크되는 것 14(`relink_expected.sha256`, 빌드 전 sha256; 바뀔 수 있음): `libkz-llvm.a`, kz-static, `libkz-llvm.a` 를 링크하는 시험 프로그램 열(`test_static_{reasons,plan,unroll,flow,elf32,wrappers,gitrev,link,recipe,expr}`), 소스가 바뀐 `test_static_cli`·`static_golden`.
  - 다시 링크되지 않아야 하는 것 110(`not_relink.sha256`, 빌드 전 sha256: MC 의 ELF 실행 파일과 정적 라이브러리 가운데 위 14 밖 모두; kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz_lint `5f62ec20…`, kz-instr-clang `28db4017…` 포함). 빌드 뒤 sha256 이 같아야 한다.
  - `CMakeCache.txt` `bc2f43b4…` 그대로 [I](`tests/static/CMakeLists.txt` 가 바뀌어 다시 구성된다). kz-static 의 flags.make(도구 정체성 define)는 바뀐다.
- **빌드**(MC 제자리, 목표별, 각각 공간 감시 아래): `make kz-static`, 그리고 `make all`. 감시가 도중에 멈추면 적고, kz-static 이 새 `--identity` 를 적을 때까지 출력을 내지 않는다.
- **도구 보존**: 빌드 전에 kz-static `6adc1bdd` 를 새 디렉터리 `/home/user/work/preserve/tools-20261116-h/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 공간 감시 아래). 사본 직후 등록부의 `6adc1bdd` 를 in-place+copy 로, 빌드 뒤 copy 로 고치고 새 kz-static 을 in-place 로 더한 다음에 빌드 뒤 등록부 점검을 돌린다(§6.8 의 첫 점검 실패를 되풀이하지 않음). 새 도구의 출력은 옛 도구의 것과 다르므로(계획의 개정 줄) "대체됨"이 아니라 사본으로 남긴다. 장부: 사본 디렉터리 행 `TOOLS_20261116h`.
- **golden**(빌드 전 scratchpad 점검 빌드로 미리 만듦 [I, 근거 아님]; sha256 은 `golden_scratch.sha256`): `tests/static/data/plangen_np.plan.golden` 두 줄(manifest 머리와 sidecar `rules` 줄의 `plan rules revision 1` → `2`), 새 `objaccess.golden`(옵션 아래의 writer·A-TYPE·FACT_LOCS·OUTCALLS·FIELD_STATUS 와 새 절 셋), 새 `objaccess_off.golden`(같은 fixture, 옵션 없음). **첫 ctest 에서 실패는 셋이어야 한다**: `static_golden_objaccess`, `static_golden_objaccess_off`(golden 파일이 아직 없음), `test_static_plan`(plangen golden 의 개정 줄 둘). `KZ_GOLDEN_UPDATE=1` 로 셋을 다시 쓴 뒤 그 셋은 scratchpad 의 것과 `cmp` 같아야 하고, 그 밖의 golden 은 바뀌지 않는다(`git diff --stat tests` 로 확인).
- **시험**: 전체 ctest 254 → 258(새: `static_fixture_compile_objaccess`, `static_fixture_compile_plankey`, `static_golden_objaccess`, `static_golden_objaccess_off`), golden 갱신 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). 확인 수는 `ctest -V` 출력으로 `ctest_counts.log` 에 남긴다: `test_static_cli` 963 → 1014 [I], `test_static_plan` 182 → 195 [I], `test_static_reasons` 660, `test_core_schema` 34,017, `lint_selftest` 109, lint 7/7. 단위 점검 `env/tools/tests/run_unit.sh` 15/15, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤와 다섯 빌드 실행 뒤 각각 `env/tools/ledger_check.sh` 전체(빌드 전 84 행, 실패 0 [M]: `ledger_check_pre.txt`)와 `env/tools/tools_check.sh --deep`(빌드 전 40 행, 실패 0 [M]: `tools_check_pre.txt`), 출력은 단계마다 새 이름(`*_after_build.txt`, `*_end.txt`). 같은 파일 이름에 다시 쓰지 않는다.
- **옵션 없는 경로(기본 경로)는 바이트 동일**(새 디렉터리, `compare.sh`):
  - relations, T8CF facts(`rerun.sh` → `objacc_relcheck`): `relations.txt`·`keys.txt` 가 `revfix_relcheck` 의 것과 바이트 동일, `relations.log` 빈 파일.
  - order, T8CF facts(`order.sh` → `objacc_order`): 두 번 같은 바이트, `order.txt`·`edges.tsv` 가 `revfix_order` 의 것과 바이트 동일, `order.log` 빈 파일.
  - 다시 뽑기 옵션 없음(`extract.sh off` → `objacc_off_extract`), `revfix_extract` 와: `basis.kz` 레코드 단위(`check_head.py --whole`: `tool_sha256`, `artifact_sha256` 만), `facts.kz` 레코드 단위(`check_whole.py`: 머리의 해시 넷, STRINGS 줄의 crc32, BASIS `extract.units_from` 의 basis 해시만), facts 보기는 `extract.units_from` 한 줄만, `extract.txt` 1 줄(facts 해시)만, `relations.txt` 2 줄(facts 해시)만, `basis.txt`·`sha256.txt` 해시 줄만, `args.txt`·`relations.log` 같음.
  - 그 facts 위의 order·relations·키(`order.sh objacc_off_extract objacc_off_order`): 두 번 같음, `order.txt` 는 `revfix_on_extract` 와 2 번째 줄만, `edges.tsv` 바이트 동일, `relations.txt` 는 다시 뽑기의 것과 같음, `keys.txt` 는 `revfix_on_extract` 의 것과 2 번째 줄만, 로그 빈 파일.
  - 계획(`plans.sh` → `objacc_plans`; `plans.sh objacc_off_extract objacc_plans_off`): `compare_plans.py t8cf revfix_plans/<b>` 와 `extract revfix_plans_on_extract/<b>`: 생성기 정체성 줄, plan rules revision 이 든 두 줄(1 → 2), sidecar sha256 줄, module 표·실행 파일 목록 첫 줄, args.txt 의 출력 디렉터리(extract 쪽은 그에 더해 facts·basis 입력·artifact 줄, manifest 의 `source build rev=basis-…`, args.txt 의 facts·basis 경로)만. site 와 필요 줄은 같다.
- **옵션 있는 경로(다섯 빌드 측정)**:
  - 다시 뽑기 옵션 있음(`extract.sh on` → `objacc_on_extract`), 같은 도구의 옵션 없는 다시 뽑기와(`compare.sh extract_on`): `basis.kz`·`basis.txt`·`relations.log` 바이트 동일, `args.txt` 는 extract-only 줄에 `--object-access` 만, `sha256.txt` 는 facts.kz 줄만. facts 보기는 `compare_on.py` 의 갈래만(그 머리 주석에 뜻): `new-section`(objaccess·ftypes·argtypes), `writer-added`, `atype-added`, `outcall-added`, `file-added`, `locs-changed`, `fstatus-changed`, `loop-note`(L1 (i) undecided 의 노트가 DISABLED 로 이름 대는 필드만), `basis-added`(옵션의 여섯 항목), `basis-changed`(`count.sites` 등 일곱 항목의 값); 가림: site·row 번호, 절 머리의 수, "(and N more)", `[files]` 줄의 번호; 형 이름의 절대 위치는 옵션의 쓰는 법으로 바꾼 뒤 비교. 그 밖은 모두 같아야 한다. `extract.txt`·`relations.txt` 의 차이는 줄 수로 센다(측정).
  - **같은 정의는 같은 이름**(`name_stability.py`, 옵션 있는 c1_pre_tx·c1_post_tx 의 facts 보기): 위치가 붙은 형 이름 가운데 파일 내용(FILE_HASHES 의 sha256)이 두 빌드에서 같은 것은 두 보기에 모두 있다. 한쪽에만 있는 이름은 두 빌드에서 내용이 다른 파일에 있어야 한다. 옵션 없는 두 보기의 같은 셈을 비교로 남긴다(`names_c1_off.txt`). [I]: 둘 다 위치가 붙은 이름 17, 모두 공통, 루트 밖은 `item@<cf>/fsw/src/cf_cmd_c:945::type` 하나(옵션 없이는 `…/pre/…`, `…/post/…` 둘로 갈림).
  - 회차의 섹션별 sha256(`section_sha_off.txt`, `section_sha_on.txt`): 옵션 없는 회차와 옵션 있는 회차의 facts 마다 섹션의 dump 글 sha256. 이 회차들은 중간 회차다(계획 4.3 의 관행): 압축 보존하지 않고, 마지막 옵션 회차가 섹션 단위로 다시 만드는지 견준다.
  - 그 facts 위의 order·relations·키(`order.sh objacc_on_extract objacc_on_order`): order 두 번 같음, `relations.txt` 는 다시 뽑기의 것과 같음, 로그 빈 파일. 계획(`plans.sh objacc_on_extract objacc_plans_on`). 이 셋의 옵션 없는 쪽과의 차이는 허용 목록 없이 잰다(`measure_on.py`: 사용 지점 키로 맞춘 행 수, 한쪽에만 있는 키와 그 키 글, 두 쪽 행의 줄 갈래별 다름 수, order 행 값의 바뀜 표, 계획의 site 줄 차이와 키 값의 모드). 측정이고 판정이 아니다.
  - [I](scratchpad 점검 빌드, LLVM Release 구성, 감시 아래; 근거 아님, 지움): c1 두 빌드 `CFE_SB_MemParams_t::Partition.Data`·`CFE_TBL_BufParams_t::Partition.Data` 가 DISABLED(escape 2 / 1); `count.array_decay` c1 out-pointer 147·escape 74·element writer 34; OBJECT_ACCESS 6,711/6,711/3,729/4,128/4,130 행, FIELD_TYPES 1,816/1,816/1,222/911/910, ARG_TYPES 17,714/17,715/12,778/7,469/7,459(지배하는 읽기가 있는 문자 배열 c1 8); relations 행 키는 c1·r2 모두 같고 c5 두 빌드에서 경로 B 의 키 7 이 빠지고 1 이 더함; order 행 값 바뀜 c1 1(unknown → unordered), r2 1(hard_before → unknown), c5 0; c1_pre_tx 계획의 A21 사용 호출 키가 `string=validated bound=20`(`CF_ChannelConfig_t.sem_name`, 읽기 `cf_cfdp.c:991:46`), 계획 site 259 → 267.
- **공간**: 각 빌드·사본·실행은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 빌드 필요량 = 앞 kz-static 빌드 dip 의 최대 123,981,824 + 새 출력 약 200 MB(다시 뽑기 둘 약 105 MB, order 넷과 둘째 실행 약 60 MB, relations 약 8 MB, 계획 셋 약 3 MB, 여유) + 한 시간 흐름 13 MB = 336,981,824 B. 사본은 필요량 100,000,000(도구 파일을 읽어 xz 를 씀; 사본에서 푸는 바이너리는 없음). 실행 단계는 필요량 100,000,000.
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh`(앞 단계의 것을 옮김), `compare.sh`, `compare_on.py`, `name_stability.py`, `measure_on.py`, `section_sha.py`, `check_whole.py`·`check_head.py`(옮김), `compare_plans.py`(옮겨 개정 줄 1|2), `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`, `golden_scratch.sha256`; 빌드 전 점검 `tools_check_pre.txt`, `ledger_check_pre.txt`; scratchpad 점검 빌드의 공간 감시 기록 `df_scratch_*.log`.

### 7.1 바뀐 것 [F]

- 커밋 `85dbdb5`(선언, 코드, fixture, 시험, 문서, 비교 스크립트, 빌드 전 목록·점검, 빌드 전). 규모: `lib/llvm/ast_fn.cpp` +약 400 줄(`object_access_of`, `path_text`, `note_array_decay`, `add_element_writer`, `decayed_target_arg`, `arg_type_of`, `note_field_types`, 호출·읽기·writer 의 수집), `ast_tu.cpp`(`name_file`, `object_key_for_name`, 이름 셈), `finalize.cpp`(`dominating_reads`, out 포인터 writer 의 바탕, 배열 탈출의 글), `serialize.cpp`(섹션 셋과 BASIS 여섯), `facts_text.cpp`(절 셋), `model.hpp`(`ObjAccess`, `FieldTypeFact`, `ArgTypeFact`, 셈), `plan_gen.cpp`(revision 2), `extract.hpp`·`spec.hpp`·`ast_reader.hpp`, `tools/kz-static/{cmd_extract.cpp,main.cpp}`; `test_static_cli.cpp` 확인 51 더, `test_static_plan.cpp` 확인 13 더, `static_golden.cpp` 사례 둘, fixture 둘, 자료 하나; `docs/static_facts.md` §5s, `docs/plan_manifest.md`, `docs/schema.md`.
- 빌드 뒤: golden 셋(`objaccess.golden`, `objaccess_off.golden` 새로, `plangen_np.plan.golden` 두 줄), 이 절과 기록, 장부·등록부 행.

### 7.2 빌드 [M]

MC 에서 제자리, 목표별, 각각 공간 감시 아래(필요량 336,981,824), 모두 첫 시도에 rc 0. 빌드 시작 17:04:54Z(`df_build_kz-static.log`), 빌드 전 HEAD `85dbdb5`(`pre_mc.txt`, kairos 아래 커밋 안 된 줄 0).

| 목표 | 때 (UTC) | dip (B) | 링크 줄 |
|---|---|---|---|
| `kz-static` | 17:04:54–17:08:43 | 256,942,080 | 2 (`libkz-llvm.a`, kz-static) |
| `all` | 17:08:43–17:12:17 | 38,322,176 | 12 (`libkz-llvm.a` 를 링크하는 시험 프로그램 열, `test_static_cli`, `static_golden`) |

- 링크된 이름 14 = `relink_expected.sha256`, 14 모두 바이트가 바뀜(`relink_result.txt`). `sha256sum -c not_relink.sha256` 110 OK(kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz_lint `5f62ec20…`, kz-instr-clang `28db4017…` 포함).
- 도구(`post_mc.txt`, `identity_kz-static.txt`): kz-static `6adc1bdd…` → `88f529b1acd7091571fe4a391022f12ebd663a277762c14e07bd0af9084e5477`; `--identity` 의 `tool_sha256` `7dba57267cfbc6bb255431c0ac97cb518a2ad39399bfb78374df084fc614f187`, `tool_source` `d7ae60a805f03df3a609f8b18563b04f30442d85895b2cd5a5cab21b705617a9`; `core_source` `568e5061…`, `schema_revision=12`, schema fingerprint `6fbff6c6…` 그대로; 나머지 여섯 도구 그대로. `CMakeCache.txt` `bc2f43b4…` 그대로. 빌드 시작 뒤 바뀐 소스 파일 0(`find -newer`, kairos 의 lib·tools·include·cmake·tests·runtime).
- 출력은 kz-static 이 새 `--identity` 를 적은 뒤에만 냈다.
- 도구 보존: 빌드 전 kz-static `6adc1bdd` 를 `/home/user/work/preserve/tools-20261116-h/kz-static.6adc1bdd.xz`(43,317,232 B)로 사본, 왕복 확인(`tool_copy/ROUNDTRIP.txt`: `xz -t` OK, 스트림 sha256 같음, `cmp` 같음). 등록부: 사본 직후 `6adc1bdd` in-place+copy, 빌드 뒤 copy, `88f529b1` in-place(빌드 뒤 첫 점검 전에 고침).
- **2026-11-18 검토 뒤 정정: §12.7 `builds-10`.** “`not_relink.sha256` 110 OK” 의 남긴 출력은 빌드 직후의 것이 아니라 `records/object-access-20261116/resume_check.txt` 의 `not_relink.sha256: 110 OK`(2026-11-16T17:38:00Z, 모든 실행 뒤)다. `relink_result.txt` 에는 바뀐 14 줄과 sha256sum 의 경고뿐이다. 빌드 직후의 `sha256sum -c` 출력은 남기지 않았다.

### 7.3 시험 [M]

- 첫 ctest(`ctest_first.log`): 258 개 가운데 실패 3, 선언한 셋 그대로(`static_golden_objaccess`, `static_golden_objaccess_off`, `test_static_plan`). 건너뜀 1.
- `KZ_GOLDEN_UPDATE=1` 로 셋을 다시 썼다(`golden_update.log`, 3 통과). 셋 모두 scratchpad 점검 빌드의 것과 `cmp` 같다(`golden_scratch.sha256`). `git status` 로 바뀐 golden 은 `plangen_np.plan.golden`(두 줄: manifest 머리와 sidecar `rules` 줄의 revision 1 → 2)과 새 파일 둘뿐이다.
- 갱신 뒤 전체 ctest(`ctest_full.log`): 258 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음).
- 확인 수(`ctest_counts.log`, `ctest -V`): `test_static_cli` 1014(963 + 51), `test_static_plan` 195(182 + 13), `test_static_reasons` 660, `test_core_schema` 34,017, `lint_selftest` 109 경우 실패 0, lint 7/7.
- 단위 점검: `env/tools/tests/run_unit.sh` 15/15(점검 15 의 설명 fixture 22/22, `unit_tools.txt`), `env/redesign/step2a_tests_unit.sh` 7/7(`unit_step2a.txt`).

### 7.4 바이트 동일성 [M]

| 비교 | 결과 |
|---|---|
| relations, T8CF facts (`compare_relations.txt`) | 15 OK: 다섯 빌드 `relations.txt`·`keys.txt` 가 `revfix_relcheck` 와 바이트 동일, `relations.log` 빈 파일 |
| order, T8CF facts (`compare_order.txt`) | 25 OK: 두 번 같은 바이트, `order.txt`·`edges.tsv` 가 `revfix_order` 와 바이트 동일, `order.log` 빈 파일 |
| 다시 뽑기 옵션 없음 vs `revfix_extract` (`compare_extract.txt`, `compare/`) | 45 OK: `basis.kz` 레코드 단위 허용 2(도구 해시), `facts.kz` 전체 dump 에서 허용 6(머리 해시 넷, STRINGS crc32, `extract.units_from`; c1_pre_tx 162,142 줄), facts 보기는 `extract.units_from` 한 줄만(c1_pre_tx 149,111 줄), `extract.txt` 1 줄, `relations.txt` 2 줄, `basis.txt`·`sha256.txt` 해시 줄만, `args.txt`·`relations.log` 같음 |
| 그 facts 위 (`compare_onextract.txt`) | 40 OK: order 두 번 같음, `order.txt` 는 `revfix_on_extract` 와 2 번째 줄만, `edges.tsv` 같음, `relations.txt` = 다시 뽑기의 것, `keys.txt` 2 번째 줄만, 로그 빈 파일 |
| 계획 (`compare_plans.txt`) | 열 모두 “only declared differences”(T8CF 위 다섯, 옵션 없는 다시 뽑기 위 다섯; site 와 필요 줄 같음) |
| 다시 뽑기 옵션 있음 vs 옵션 없음 (`compare_extract_on.txt`) | 42 OK: `basis.kz`·`basis.txt`·`relations.log` 같은 바이트, `args.txt` 는 extract-only 줄만, `sha256.txt` 는 facts.kz 줄만, facts 보기 다섯 모두 선언한 갈래만(`compare/on_<b>.txt`), c1 이름 점검 선언대로(`compare/names_c1.txt`) |
| 그 facts 위 (`compare_onextract_on.txt`) | 30 OK: order 두 번 같음, `relations.txt` = 옵션 있는 다시 뽑기의 것, 로그 빈 파일, 측정 다섯(`compare/measure_<b>.txt`) |

- **옵션 없는 경로는 앞판과 같다**: 위 첫 다섯 줄이 그 측정이다. 다른 것은 도구 정체성에서 오는 값(머리의 tool hash 와 그것을 덮는 해시, 그 basis 해시를 인용하는 `extract.units_from`)과 계획의 개정 글뿐이다.
- **누적 목록과 등록부**(빌드마다 따로 남김): 빌드 전 `ledger_check_pre.txt` 84 행 실패 0, `tools_check_pre.txt` 40 행 실패 0; 빌드 뒤 `ledger_check_after_build.txt` 84 행 실패 0, `tools_check_after_build.txt` 41 행 실패 0(deep); 다섯 빌드 실행과 장부 행을 더한 뒤 `ledger_check_end.txt` 94 행 실패 0(새 행 열: `OBS_objacc_relcheck`, `OBS_objacc_order`, `OBS_objacc_off_extract`, `OBS_objacc_off_order`, `OBS_objacc_on_extract`, `OBS_objacc_on_order`, `OBS_objacc_plans`, `OBS_objacc_plans_off`, `OBS_objacc_plans_on`, `TOOLS_20261116h`), `tools_check_end.txt` 41 행 실패 0(deep).
- 바깥 이름 점검(`--tree`)은 아홉 출력 디렉터리, 사본 디렉터리, 기록 디렉터리에서 깨끗하다(`name_check_tree.txt`).
- 세션이 다시 시작된 뒤(17:38Z) 디스크의 MC 를 기록과 다시 맞댔다(`resume_check.txt`): `post_mc.txt` 8 줄 같음, `--identity` 가 `identity_kz-static.txt` 와 같음, `not_relink.sha256` 110 OK, 다시 뽑기의 `status.txt` 10 개 모두 `result PASS`, 나머지 일곱 출력 디렉터리에 다섯 빌드 모두. 반쯤 끝난 빌드·실행은 없었고 다시 빌드하거나 다시 돌린 것은 없다. 끝의 점검을 새 이름으로 한 번 더 남겼다: `ledger_check_final.txt` 94 행 실패 0, `tools_check_final.txt` 41 행 실패 0(deep).

### 7.5 측정 [M]

옵션 없는 다시 뽑기(`objacc_off_extract`)와 옵션 있는 다시 뽑기(`objacc_on_extract`), 같은 도구(kz-static `88f529b1`). 수의 출처: BASIS 항목(`measure_basis.txt`), 위치의 상태(`status_change.txt`, `status_change.py`), relations·order·계획(`compare/measure_<b>.txt`, `order_changed.txt`, `order_changed_rows.txt`), 섹션별 sha256(`section_compare.txt`).

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| OBJECT_ACCESS 행(resolved / partial / not resolved) | 6,711 (2,376 / 927 / 3,408) | 6,711 (2,376 / 927 / 3,408) | 3,729 (1,543 / 638 / 1,548) | 4,128 (2,754 / 465 / 909) | 4,130 (2,756 / 465 / 909) |
| FIELD_TYPES 행 | 1,816 | 1,816 | 1,222 | 911 | 910 |
| ARG_TYPES 행(지배하는 읽기가 있는 문자 배열) | 17,714 (8) | 17,715 (8) | 12,778 (7) | 7,469 (5) | 7,459 (5) |
| `count.array_decay`: out-pointer 호출 / 주소 탈출 / 원소 writer | 147 / 74 / 34 | 147 / 74 / 34 | 85 / 43 / 18 | 165 / 53 / 43 | 166 / 53 / 43 |
| `count.type_names_outside_root`: 소스 트리 / 절대 | 2 / 0 | 2 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| `count.sites` 옵션 없음 → 있음 | 37,325 → 46,005 | 37,327 → 46,007 | 20,640 → 25,457 | 17,985 → 21,439 | 18,123 → 21,578 |
| 대상 위치의 writer-set-complete 옵션 없음 → 있음 | 142 → 81 | 142 → 81 | 111 → 67 | 74 → 32 | 74 → 32 |
| complete → DISABLED (새 탈출이 있음 / 새 탈출 없이 형 단위로 드러남) | 61 (42 / 19) | 61 (42 / 19) | 44 (40 / 4) | 42 (15 / 27) | 42 (15 / 27) |
| A-TYPE 주소 탈출 줄 옵션 없음 → 있음(더해진 줄 가운데 호출 인자 / 정수 변환) | 993 → 1,526 (131 / 1) | 993 → 1,526 (131 / 1) | 626 → 1,069 (70 / 1) | 1,357 → 2,122 (129 / 0) | 1,359 → 2,125 (130 / 0) |
| relations 행(같은 키 / 옵션 없음에만 / 있음에만) | 367 (367 / 0 / 0) | 367 (367 / 0 / 0) | 295 (295 / 0 / 0) | 178 → 172 (171 / 7 / 1) | 175 → 169 (168 / 7 / 1) |
| 두 쪽에 있는 행 가운데 글이 다른 행(그 가운데 `writers` 줄) | 239 (239) | 239 (239) | 178 (178) | 108 (108) | 108 (108) |
| order 행 값이 바뀐 행 | 1 (A20 unknown → unordered) | 1 (A20 unknown → unordered) | 1 (B111 hard_before → unknown) | 0 | 0 |
| 계획 site 옵션 없음 → 있음 | 259 → 267 | 245 → 253 | 42 → 42 | 147 → 147 | 139 → 139 |

- **선언의 [I] 와**: OBJECT_ACCESS·FIELD_TYPES·ARG_TYPES 의 수, `count.array_decay`, relations 행 키(c1·r2 같음, c5 경로 B 키 7 빠짐·1 더함), order 행 값의 바뀜(c1 1, r2 1, c5 0), A21 의 `bound=20` 과 c1_pre_tx 의 계획 site 259 → 267 이 모두 scratchpad 점검 빌드의 [I] 와 같다. `compare/on_<b>.txt` 의 `new-section` 수가 BASIS 의 수보다 하나씩 많은 것은 그 절의 머리 줄을 센 것이다.
- **§6.9 첫 항목(배열의 포인터 변환)**: c1 두 빌드와 r2_core 에서 `CFE_SB_MemParams_t::Partition.Data` 는 `writer-set-complete writers=1` 에서 `DISABLED`(escape 2: `cfe_sb_init.c:104:12` 에서 `CFE_ES_PoolCreateEx` 로 넘어가 `cfe_es_mempool.c:313:41` 의 형 변환에 쓰임, `cfe_sb_init.c:111:30` 에서 정수로 바뀜), `CFE_TBL_BufParams_t::Partition.Data` 는 `DISABLED`(escape 1: `cfe_tbl_internal.c:105:14` 의 `CFE_ES_PoolCreate` 를 거쳐 `CFE_ES_PoolCreateEx`)로 바뀌었다(`measure_basis.txt`). 그 두 행에 writer-set-complete 를 더는 말하지 않는다.
- **위치 상태의 큰 움직임**: complete 에서 DISABLED 로 간 위치가 61/61/44/42/42 다. 두 갈래다. (가) 새 주소 탈출이 생긴 위치(42/42/40/15/15): 더해진 탈출 줄의 호출 상대는 c1 에서 35 종이고 `CFE_EVS_SendEventWithAppID` 25, `CFE_EVS_SendEvent` 20, `CFE_ES_WriteToSysLog` 15, `strncmp` 10, `CFE_SB_MessageStringSet` 8, `strcmp` 6, `OS_strnlen` 5, `snprintf` 5 가 많다(`status_change.txt`). 이들은 본문이 닫힌 프로그램에 없거나(가변 인자의 이벤트·로그 호출, C 라이브러리) 본문이 그런 함수로 인자를 넘기는 호출이라, 선언 2 의 규칙(“탈출·본문 없음·간접 호출이면 주소 탈출”)대로 탈출이다. 호출 인자 밖의 더해진 줄(c1 401)은 레코드 배열 필드가 포인터에 대입·초기화·반환되는 것(`RegRecPtr`, `AccessDescPtr` 등)이고, 앞판의 `&x.f` 탈출과 같이 그 형의 대상 필드 모두에 붙는다. (나) 새 탈출 없이 DISABLED 가 된 위치(19/19/4/27/27): 같은 형의 다른 필드가 탈출해 형 단위 상태가 DISABLED 가 되고, 그러면 프로그램의 형 없는 쓰기(c1 133, r2 99)가 그 위치에도 세어진다(`untyped 0->133`). 두 갈래 모두 앞판의 형 단위 규칙을 바꾸지 않은 결과다.
- **C1 의 A20**(사전 시험의 대조 행, §6.10 의 2): 옵션 없이 A20 의 키는 `known`(정적 이미지 `BP_Throttles[0].Name = "BP0_tsem"`, `[1] = "BP1_tsem"`, 전제는 `BP_Throttle_t.Name` 의 writer 0 과 writer-set-complete)이었고, SCH_LAB 의 `OS_CountSemCreate`(`sch_lab_app.c:168:16`, 키 `"SCH_LAB"`)는 `not a pair: key constants differ` 로 빠졌다. 옵션 있이 `BP_Throttle_t.Name` 은 escape 6(`bp_flow.c:126:13` 의 `OS_CountSemGetIdByName` → `OS_ObjectIdFindByName`…, `:132:18` 의 `OS_CountSemCreate` → `memchr` 본문 없음, `:129`·`:135`·`:140`·`:165` 의 `CFE_EVS_SendEvent` 본문 없음)으로 DISABLED 이고, 그래서 이미지를 실행 값으로 받지 않아 키가 `unresolved`, SCH_LAB 생성이 다른 행위자의 생성자로 남아 order 가 `unordered`(짝 1)가 되었다(`order_changed_rows.txt`). 넘겨진 호출들은 그 글을 읽기만 하는 것으로 보이나, 이 도구는 본문 없는 함수의 읽기 전용 효과를 사실로 갖지 않는다 [미확인]. 이 행의 해석은 §7.8.
- **R2 의 B111**: 옵션 있이 `CFE_TBL_TblRegPacket_Payload_t.OwnerAppName` 에 사용 호출 자신(`cfe_tbl_task_cmds.c:326:9` 의 `CFE_ES_GetAppName`, `cfe_es_api.c:829` strncpy·`:830`·`:835` 원소 저장)이 out-pointer writer 셋으로 더해졌고, order 는 그 writer 와 같은 호출의 사용을 한 스레드의 짝 `unknown` 으로 세어 행 값이 hard_before → unknown 이 되었다(`order_changed_rows.txt`). 같은 호출 안의 쓰기와 그 호출의 사용을 짝으로 볼지는 order 규칙의 일이다(§7.8).
- **c5 의 relations 행**: 빠진 경로 B 키 7 은 `CFE_TBL_LoadBuff_t.DataSource`(5), `CFE_TBL_RegistryRec_t.LastFileLoaded`, `CFE_TBL_RegistryRec_t.Name` 이고, 더한 1 은 `EVS_AppData_t.BinFilters` → `EVS_FindEventID#1` 이다(`compare/measure_c5_641.txt`). c1·r2 의 행 키는 바뀌지 않았다.
- **계획**: c1 두 빌드에서 site 가 8 늘었다(옵션 있음에만 12 줄, 없음에만 4 줄; `CFE_TBL_GetTblRegData` 의 `CFE_ES_GetAppName` 이 use-call+writer 로, `CFE_TBL_EarlyInit` 의 memset 두 site 의 참조가 늘고, 새 site 여덟은 `CFE_TBL_*` 의 저장 writer 다: `cfe_tbl_internal.c:1267`, `:941`, `:984`, `cfe_tbl_api.c:864`, `:1512`, `:312`, `:403`, `:637`). r2·c5 는 site 줄이 같다.
- **키 글의 상한**(plan rules revision 2): c1_pre_tx 의 계획에서 A21 의 사용 호출(`cf_cfdp.c:993`, `OS_CountSemGetIdByName` 인자 1)의 키 값이 `string=pointer-only` 에서 `string=validated bound=20` 이 되었다. 참조 글: `CF_ChannelConfig_t.sem_name`, 크기 20(`cf_tbldefs.h:74`), 읽기 `cf_cfdp.c:991:46` 이 호출을 지배; sidecar 에 `bound string-extent source=ARG_TYPES` 한 줄(`plan_key_c1.txt`). 옵션 없는 facts 의 계획은 개정 줄 밖에서 앞판과 같다(`compare_plans.txt`). c1_post_tx 의 계획에는 옵션 없음·있음 모두 키 값이 있는 사용·생성 호출이 없다(`compare/measure_c1_post_tx.txt`).
- **같은 정의는 같은 이름**(`compare/names_c1.txt`, `names_c1_off.txt`): 옵션 있이 위치가 붙은 형 이름 pre 17, post 17, 공통 17, 한쪽에만 0(소스 트리 1, 루트 기준 16). 옵션 없이는 공통 16, 한쪽에만 2(`item@/home/user/work/procrace/c1_build/{pre,post}/src/cf/fsw/src/cf_cmd_c:945::type`). 옵션 있는 쪽의 이름은 `item@<cf>/fsw/src/cf_cmd_c:945::type` 이고 두 빌드의 facts 에 각각 66 줄로 있다(`names_in_dump_c1.txt`). BASIS 의 `count.type_names_outside_root` 는 `by source tree 2` 인데 facts 의 dump 에 든 서로 다른 이름은 1 이다(§7.7).
- **회차의 섹션별 sha256**(`section_compare.txt`): 옵션 없는 회차 섹션 39, 옵션 있는 회차 42(새 셋), 같은 섹션은 c1·c5 3, r2 1. 나머지가 다른 것은 site 번호(`count.sites` 가 늘어남)와 문자열 표의 위치가 섹션의 레코드와 crc32 에 들기 때문이다; 뜻의 비교는 facts 보기의 갈래 비교(`compare/on_<b>.txt`)가 했다. 이 둘은 중간 회차이고, 마지막 옵션 회차가 섹션 단위로 견준다.
- **2026-11-18 검토 뒤 정정: §12.7 `semantics-3`, `semantics-4`.** (1) 이 절이 “본문이 닫힌 프로그램에 없다”고 적은 호출 인자 탈출 가운데 c1 의 60 줄(`CFE_EVS_SendEventWithAppID` 25, `CFE_EVS_SendEvent` 20, `CFE_ES_WriteToSysLog` 15; 마지막 회차 facts 의 atype 절)은 본문이 **있는** 가변 인자 함수의 이름 없는 인자다(같은 facts 의 functions 절: 셋 다 `{defined,variadic}`). 탈출이라는 상태는 그대로이고 사유 글이 틀렸다(`finalize.cpp` 가 요약이 없는 경우와 인자가 이름 있는 매개변수 밖인 경우를 한 글로 적음). 이 묶음은 그 callee 자신에 대해서는 C 라이브러리 글에 기대지 않는다. (2) “배열 decay 의 열린 것을 닫았다”는 평이한 꼴(인자·저장·정수 변환·반환)에만 선다: 조건 연산자나 쉼표 연산자 아래의 decay, 배열의 배열 필드의 한 줄은 탈출 행을 얻지 못했다(검토의 scratch 실행; 실제 빌드에서 틀린 행은 드러나지 않았고 fixture 에 그 꼴이 없었다). 둘 다 코드로 고쳤고(`static_facts.md` §5w) MC 도구는 아직 앞판이다(§12.6).

### 7.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 17:02:22 | 2,713,444,352 | 사본 시작(dip 43,347,968) |
| 17:04:54 | 2,670,075,904 | `make kz-static` 시작(가장 낮은 2,413,133,824, dip 256,942,080) |
| 17:08:43 | 2,664,148,992 | `make all` 시작(dip 38,322,176) |
| 17:15:50–17:21:55 | 2,646,081,536 → 2,476,085,248 | relations(dip 7,675,904), order(14,209,024), 다시 뽑기 옵션 없음(47,161,344), 그 order(21,856,256), 계획 둘(987,136 씩), 다시 뽑기 옵션 있음(54,165,504), 그 order(21,946,368), 계획(991,232) |
| 17:22:11–17:22:58 | 2,483,159,040 → 2,490,040,320 | 비교(facts 보기는 scratchpad, 끝에 지움; 옵션 비교의 dip 117,391,360) |
| 17:36 | 2,496,573,440 | 측정용 facts 보기(scratchpad, 약 200 MB)를 지운 뒤 |
| 이 절을 쓸 때 | 2,496,471,040 | 끝의 장부·등록부 점검 뒤 |

- 하한(314,572,800)을 넘지 않았다. `make kz-static` 의 dip 256,942,080 은 필요량(336,981,824) 안이지만 필요량 계산에 넣은 앞 kz-static 빌드 dip 의 최대(123,981,824)보다 컸다(§7.7).
- 남긴 출력(장부 행): `objacc_relcheck`, `objacc_order`, `objacc_off_extract`, `objacc_off_order`, `objacc_on_extract`, `objacc_on_order`, `objacc_plans`, `objacc_plans_off`, `objacc_plans_on` 합 147,744,408 B, 사본 `kz-static.6adc1bdd.xz` 43,317,232 B. 지운 것: scratchpad 의 점검 빌드와 그 출력(선언 전), 비교와 측정의 facts 보기(scratchpad)뿐이다.

### 7.7 계획과 다르게 한 것

- **세션이 끊겨 이어서 했다.** 빌드·시험·golden·다섯 빌드 실행·비교는 lead 의 스냅숏 커밋 `61f1122` 에 들었고, 이어 받은 뒤 디스크의 도구와 출력이 기록과 같음을 확인했다(`resume_check.txt`). 다시 빌드하거나 다시 돌린 것은 없다. 이어 받은 뒤에 더한 것: `status_change.py`·`status_change.txt`, `order_changed.txt`, `order_changed_rows.txt`, `a20_keys.txt`, `names_in_dump_c1.txt`, `resume_check.txt`, `ledger_check_final.txt`, `tools_check_final.txt`, 이 §7.4 끝 줄과 §7.5–§7.8. 측정 스크립트 `status_change.py` 는 선언에 없던 것이며, 판정이 아니라 선언의 측정(“허용 목록 없이 잰다”)을 위치 상태로 넓힌 것이다.
- **빌드 dip 이 필요량 계산의 바탕보다 컸다**: `make kz-static` 256,942,080 B 대 앞 최대 123,981,824 B. 그 까닭은 재지 않았다 [미확인]. 선언한 필요량(336,981,824) 과 감시 문턱(651,554,624) 안이었다. 다음 kz-static 빌드의 필요량은 이 값을 바탕으로 한다.
- **BASIS `count.type_names_outside_root` 의 수**가 facts 에 든 이름의 수와 다르다(2 대 1, `names_in_dump_c1.txt`). 그 셈은 이름을 만든 차례마다 집합에 넣으므로(`lib/llvm/ast_tu.cpp:251-253`) facts 에 쓰이지 않는 이름도 든다 [미확인: 다른 하나가 무엇인지 남긴 출력으로 보이지 않음]. 선언은 이 항목의 값을 정하지 않았으므로 선언 밖의 차이는 아니고, 항목의 뜻은 `docs/static_facts.md` §5s 의 “둘을 센다”보다 넓다.
- 선언 밖의 차이는 없었다. 덧붙임은 없다.
- **2026-11-18 검토 뒤 정정: §12.7 `builds-04`.** scratch 의 다섯 빌드 추출(`one_build.sh` 를 scratch kz-static 으로)과 `status_change.py` 용 facts 보기(약 200 MB, §7.6 의 17:36 줄)는 공간 감시 기록 없이 돌렸다. 남긴 `df_scratch_*` 는 cmake·make 뿐이다.

### 7.8 열린 것

- **본문 없는 함수의 읽기 전용 효과**: 더해진 주소 탈출의 다수는 글을 읽기만 하는 것으로 보이는 호출(`strcmp`, `strncmp`, `strlen`, `OS_strnlen`, `printf`, 가변 인자의 이벤트·로그 호출)이다. 이 효과를 사실로 가지면(C 라이브러리 함수의 효과는 P0 표의 비교 갈래와 함께) writer-set-complete 의 일부와 C1 A20 의 `known` 키가 돌아올 수 있다. 효과의 대조는 링크한 C 라이브러리 판의 글이 필요하고 그 글은 이 작업 환경에 없다(새 외부 자원, `unverified`). 자리: 의미와 확인 상태 단계(P0 항목의 의미).
- **A20 의 대조**(§6.10 의 2, (가)): 옵션 있는 facts 의 A20 은 키 `unresolved` 이고 SCH_LAB 생성과 `unordered` 짝을 갖는다. 사전 시험의 대조 선택 규칙(manifest 개정 4 의 생성기)은 이 행을 (가)로 넣지만, 기대([I])를 커밋할 때 이 facts 를 쓰면 키 글의 기대가 옵션 없는 facts 와 다르다. 어느 facts 로 기대를 적는지는 마지막 옵션 회차 뒤에 정한다.
- **같은 호출 안의 out-pointer 쓰기와 그 호출의 사용**(R2 B111): order 가 이를 한 스레드의 `unknown` 짝으로 센다. 같은 호출의 쓰기를 사용의 일부로 볼지 order 규칙에 정할 일이다.
- **함수·객체 키의 절대 위치**: 형 이름만 소스 트리 기준이 되었고, 루트 밖 함수의 키(`initialize_throttling@/home/user/work/procrace/c1_build/pre/…`)와 위치는 절대 경로다. 그래서 같은 관계의 키가 c1_pre_tx 와 c1_post_tx 에서 다르다(A20 `35f07539e7606e2e` 대 `bdcc6978b9a41c8c`; 키 글 `A|OS_common_record_t.name_entry|initialize_throttling@/home/user/work/procrace/c1_build/{pre,post}/src/bp/fsw/src/bp_flow.c|OS_CountSemGetIdByName#1|1`, `a20_keys.txt`). 빌드 사이 비교 단계가 이 키를 쓰기 전에 정한다. 계획의 ARG_TYPES 참조 글에도 절대 위치가 든다(`plan_key_c1.txt`).
- **`count.type_names_outside_root` 의 뜻**(§7.7): facts 에 쓰인 이름만 세도록 할지, 문서의 글을 고칠지.
- 이 회차의 출력은 중간 회차다. 마지막 옵션 회차(루프 술어·잠금 정체성 단계)가 섹션 단위로 견주고, 같으면 이 회차를 “뒤 회차로 다시 만들 수 있음”으로 적는다.
- **2026-11-18 검토 뒤 정정: §12.7 `semantics-3`, `rules-06`.** “본문 없는 함수” 묶음에 넣은 `:129`·`:135`·`:140`·`:165` 의 `CFE_EVS_SendEvent` 는 본문이 있다(가변 인자의 이름 없는 인자; 위 §7.5 의 정정). 그리고 이 단계가 맡은(판정 규칙 8.12 (ㅈ)) 종류별 키 필드 writer 나눔은 주지 않았다: 마지막 회차의 A21 행은 여전히 `writers: OS_common_record_t.name_entry: 18 (assign 17, memset 1)` 한 줄이다(`ll_final_extract/c1_pre_tx/c1_pre_tx/NP/relations.txt:650`). §12.9 의 표.

## 8. 검증 상태가 붙은 의미: 범위 규칙, 기계적 훑기, 생성된 자료 파일, 추출 옵션 `--semantics` (kz-static·kz_lint 빌드 하나)

범위 규칙을 어떤 자료 파일보다 먼저 적어 커밋하고, 판이 고정된 공개 머리말·소스의 기계적 훑기로 자료 파일을 만들고, 추출 옵션 하나가 그 자료를 빌드에 이어 섹션 SEMANTICS 로 쓴다. relations 는 SEMANTICS 가 있는 facts 에서만 검증되지 않은 의미 항목을 행에 적는다. lint 규칙 하나가 자료의 API 이름을 엔진 코드 밖에 둔다. 기록: `../records/semantics-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `sem_*`. 도구 사본: 새 디렉터리 `/home/user/work/preserve/tools-20261116-i/`. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 8.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다.

- **범위 규칙(이미 커밋함)**: `kairos/docs/semantics_coverage.md` 개정 1, 구성 요소와 닻 목록 `../records/semantics-20261116/semantics_trees.tsv`, 훑기 `env/tools/semantics_scan.py` 는 커밋 `40aa9f5`(2026-11-16T18:03:39Z)에 들어갔고, 그때 `kairos/semantics/` 에는 앞 단계의 `framework_api_roles.txt` 밖의 자료 파일이 없었다. 자료 파일은 그 뒤에 훑기가 만들었다(`scan_generate_1.txt` 18:03:55Z; 첫 판의 P0 선언 파일이 로컬 사본의 절대 경로를 담아 `case-ids-data` 에 걸려(`lint_data_1.txt`) 파일 이름만 쓰도록 훑기를 고쳐 다시 만듦: `scan_generate_2.txt`, `lint_data_2.txt` clean). 규칙의 글은 바꾸지 않았다.
  - 항목(규칙 §3): 공개 머리말에 선언된 함수 가운데 (E1) 반환·매개변수의 형이 상태·event-type·run-status typedef 이거나, (E2) 선언 바로 위 주석이 반환 줄에서 상태 상수를, 매개변수 문단에서 event-type·run-status 상수를 이름 대거나, (E3) 소스의 정의가 상태 상수를 돌려준다. E2·E3 은 여러 판이 그 값을 정수형으로 선언하고 문서에만(또는 문서 없이) 정의역을 두기 때문이다(예: 6cccb3f `CFE_EVS_SendEvent(uint16 EventID, uint16 EventType, …)` `cfe_evs.h:155`, 문서 `\arg #CFE_EVS_EventType_*` `:129-133`; OSAL 4.1.1 머리말에는 함수별 문서가 없음 `osapi-os-core.h:128-188`).
  - 이름 공간 항목(규칙 §4, 판정 규칙 8.2 조건 (가)의 입력): 객체 종류마다 종류 상수와 값, 조회·생성·삭제 함수, 저장된 이름의 필드와 그 크기 선언, 비교 함수와 그 비교, 이름 필드의 다른 저장, 조회 사슬의 상태 상수(규칙 N: object-id 세대; 규칙 T: 표 세대).
  - P0 선언(규칙 §5): P0 표 두 개의 이름마다 빌드가 읽는 C 라이브러리 머리말의 첫 원형과 그 속성.
- **자료 파일**(`kairos/semantics/`, 모두 훑기의 출력, `scan_check_3.txt` PASS: 13 파일, 실패 0): `framework_semantics_<판>.txt` 일곱(osal-f34759d 128 항목/선언 153, cfe-6cccb3f 129/182, osal-7.0.1 150/178, cfe-7.0.1 133/196, osal-4.1.1 55/118, cfe-6.4.1 73/131, cfe-6.4.2 73/131; 모두 741 항목), `framework_namespaces_<판>.txt` 다섯(osal-f34759d 11 종류, osal-7.0.1 12, osal-4.1.1 10, cfe-6.4.1 2, cfe-6.4.2 2), `p0_declarations_glibc-2.39-headers.txt`(85 이름 모두 원형 찾음). 시험 fixture 의 자료 `kairos/tests/static/data/semantics_fixture/framework_semantics_fixture-sem.txt` 도 같은 훑기의 출력이다(`fixture_trees.tsv`, `fixture_check_2.txt` PASS).
- **무엇을 하는가** (`kairos/docs/static_facts.md` §5t):
  1. **옵션** `kz-static extract --semantics FILE`(여러 번; 확장 사실과 함께만, 없으면 종료 코드 2; 자료 형식이 맞지 않으면 아무것도 읽기 전에 종료 코드 2). 옵션이 없으면 추출의 코드 경로와 출력은 앞판 그대로다.
  2. **결합**: facts 를 한 번 쓰고 다시 읽어 의미 행을 계산하고 SEMANTICS 와 BASIS 항목(`extract.semantics`, `extract.semantics[i]`, `count.semantics`, `count.semantics_entries[i]`)을 더해 한 번 더 쓴다. 추출 출력에 `semantics …` 한 줄.
  3. **층**: P0(빌드가 부르는 P0 표 항목의 역할·out 인자·조건부와 출처 상태, P0 선언 속성 pure·const·noreturn·nonnull), framework(문서의 반환, 인자 역할·정의역, 본문이 돌려주는 상수, 문서와 본문의 일치, 도출한 역할), status-space(성공 상수, severity mask). 항목은 고정된 파일의 sha256 이 빌드에 있을 때만 적용한다.
  4. **relations**: SEMANTICS 가 있을 때만 머리 한 줄 `semantics of these facts …` 와 layers 칸 끝의 ` sem-unverified(api:column, …)`. 규칙 개정 줄(relations rules revision 9)과 그 밖의 글은 그대로. SEMANTICS 없는 facts 의 relations·order·plan 은 앞판과 같은 바이트.
  5. **lint 규칙 `semantics-names`**: `semantics/` 의 framework 층 자료가 이름 대는 함수(api 줄, 이름 공간 파일의 함수, `function=`)가 엔진 코드(case-ids 의 범위)에 낱말로 있으면 위반. P0 선언 파일(layer p0)은 읽지 않는다. 선택 시험 다섯 더(109 → 114).
  6. `kz-static show` 의 절 `semantics`.
- **레이아웃 점검.** 쓰는 코드가 개정 12 의 `kz_fact_sem`(72 B)과 열거형 `KZ_SEM_LAYER`·`KZ_SEM_COLUMN`·`KZ_SEM_VERIFY`·`KZ_SEM_ORIGIN`·`KZ_SEM_FLAG` 에 그대로 맞는다. `include/` 는 바꾸지 않는다. 설계 연구가 따로 두자고 한 P0 의미 섹션 대신 SEMANTICS 의 P0 층을 쓴다(개정 12 가 그렇게 정의함).
- **1차 근거.**
  - 판 고정: 항목마다 선언 머리말과 정의 소스의 file:line 과 그 파일의 sha256(자료의 `file` 줄). 같은 이름이 판마다 다르다(6.4.1 `CFE_ES_GetAppID(uint32 *)` 와 6cccb3f `CFE_Status_t CFE_ES_GetAppID(CFE_ES_AppId_t *)` `cfe_es.h:534`)는 것이 이름으로 옮기지 않는 까닭이다.
  - P0 대조: 빌드가 링크하는 C 라이브러리는 GNU C Library 2.39 의 배포 패키지(앞 기록 `observation_path_results_20261116.md` §1.7 `c_library.txt`)이고, 빌드가 읽는 머리말 `/usr/include/{string,pthread,…}.h` 는 같은 판의 개발 패키지에 든다(`c_library_headers.txt`: libc6, libc6-dev, libc6-i386, libc6-dev-i386 같은 판, 머리말 열한 개의 sha256 과 패키지). 행위 문장을 대조할 로컬 소스 사본은 `pthread_create` 의 upstream 2.39 사본 하나(`p0.cpp` 의 source, sha256 `36fd1e6d…`)이고 배포 패치는 대조하지 않는다(배포 changelog 에 `pthread_create` 를 이름 대는 줄 1, `c_library_headers.txt`). 그 밖은 `unverified` 다(새 외부 자원 없음).
  - 선언 속성의 뜻: glibc 머리말의 `__attribute_pure__` 는 `__attribute__ ((__pure__))` 로 펼쳐진다(`/usr/include/x86_64-linux-gnu/sys/cdefs.h:325`; 그 속성을 모르는 컴파일러에서는 `:327` 에서 빈 것); GCC 의 pure 속성은 반환값 밖의 효과가 없음을 뜻한다(GCC 문서 "Common Function Attributes", 이 환경에 원문 없음 [미확인]).
- **코드**(`kairos/`): 새 `lib/llvm/semantics.{hpp,cpp}`; `lib/llvm/{serialize.hpp,serialize.cpp,facts_text.cpp,rel_facts.hpp,rel_facts.cpp,relations.cpp,CMakeLists.txt}`, `tools/kz-static/{cmd_extract.cpp,main.cpp}`; `tests/lint/{kz_lint.cpp,CMakeLists.txt}`; 시험 `tests/static/{test_static_cli.cpp,static_golden.cpp,CMakeLists.txt}`, 새 fixture `tests/static/fixtures/semantics.c`, 새 자료 `tests/static/data/semantics_fixture/`; 문서 `docs/semantics_coverage.md`(커밋됨), `docs/static_facts.md` §5t, `docs/schema.md` 의 날짜 붙은 줄; 자료 `semantics/` 의 열셋.
  - 하지 않는 것: `include/`, `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `instrument/`, relations·order·plan 의 규칙(개정 9, 2, 2 그대로), `semantics/framework_api_roles.txt`, 판정 규칙과 어휘 자료, 설명 스크립트, `one_build.sh`(옵션은 `ONE_BUILD_EXTRACT_EXTRA` 로).
- **도구 해시와 정체성**: kz-static 의 파일 sha256(빌드 전 `88f529b1acd7091571fe4a391022f12ebd663a277762c14e07bd0af9084e5477`), `--identity` 의 `tool_sha256`(빌드 전 `7dba5726…`)·`tool_source`(빌드 전 `d7ae60a8…`)가 바뀐다. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 는 그대로 [I]. kz_lint 의 파일 sha256(빌드 전 `5f62ec20d2d26e74501cf7469649761df662d673659835ec9bb0902ed6acd018`)이 바뀐다(정체성 출력 없음). 나머지 도구의 정체성은 그대로.
  - 다시 링크되는 것 15(`relink_expected.sha256`, 빌드 전 sha256; 바뀔 수 있음): `libkz-llvm.a`, kz-static, `libkz-llvm.a` 를 링크하는 시험 프로그램 열(`test_static_{reasons,plan,unroll,flow,elf32,wrappers,gitrev,link,recipe,expr}`), 소스가 바뀐 `test_static_cli`·`static_golden`, kz_lint.
  - 다시 링크되지 않아야 하는 것 109(`not_relink.sha256`, 빌드 전 sha256: 앞 단계의 110 에서 kz_lint 를 뺀 것, 빌드 전에 모두 그대로임을 확인; kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz-instr-clang `28db4017…` 포함). 빌드 뒤 sha256 이 같아야 한다.
  - `CMakeCache.txt` `bc2f43b4…` 그대로 [I](`lib/llvm`·`tests/static`·`tests/lint` 의 CMakeLists 가 바뀌어 다시 구성된다).
- **빌드**(MC 제자리, 목표별, 각각 공간 감시 아래): `make kz-static`, `make kz_lint`, `make all`. 감시가 도중에 멈추면 적고, kz-static 이 새 `--identity` 를 적을 때까지 출력을 내지 않는다.
- **도구 보존**: 빌드 전에 kz-static `88f529b1` 과 kz_lint `5f62ec20` 을 새 디렉터리 `/home/user/work/preserve/tools-20261116-i/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 공간 감시 아래). 사본 직후 등록부의 둘을 in-place+copy 로, 빌드 뒤 copy 로 고치고 새 둘을 in-place 로 더한 다음에 빌드 뒤 등록부 점검을 돌린다. 새 kz-static 의 옵션 있는 출력은 옛 도구의 것과 다르므로 "대체됨"이 아니라 사본으로 남긴다. 장부: 사본 디렉터리 행 `TOOLS_20261116i`.
- **golden**(빌드 전 scratchpad 점검 빌드로 미리 만듦 [I, 근거 아님]; `golden_scratch.sha256` `bcdfb9a7…`): 새 `semantics_np.golden` 하나. **첫 ctest 에서 실패는 하나여야 한다**: `static_golden_semantics_np`(golden 파일이 아직 없음). `KZ_GOLDEN_UPDATE=1` 로 그것만 쓴 뒤 scratchpad 의 것과 `cmp` 같아야 하고, 그 밖의 golden 은 바뀌지 않는다(`git status`).
- **시험**: 전체 ctest 258 → 261(새: `static_fixture_compile_semantics`, `static_golden_semantics_np`, `lint_semantics-names`), golden 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`). 확인 수는 `ctest -V` 출력으로 `ctest_counts.log` 에: `test_static_cli` 1014 → 1058 [I], `test_static_plan` 195, `test_static_reasons` 660, `test_core_schema` 34,017, `lint_selftest` 109 → 114, lint 규칙 7 → 8. 단위 점검 `env/tools/tests/run_unit.sh` 15/15, `env/redesign/step2a_tests_unit.sh` 7/7. 훑기의 점검 `semantics_scan.py check` 를 빌드 뒤에 다시(새 이름).
- **누적 바이트 동일성**: 빌드 뒤와 다섯 빌드 실행 뒤 각각 `env/tools/ledger_check.sh`(빌드 전 94 행 실패 0: `ledger_check_pre.txt`)와 `env/tools/tools_check.sh --deep`(빌드 전 41 행 실패 0: `tools_check_pre.txt`), 출력은 단계마다 새 이름.
- **옵션 없는 경로(기본 경로)는 바이트 동일**(새 디렉터리, `compare.sh`):
  - relations, T8CF facts(`rerun.sh` → `sem_relcheck`): `relations.txt`·`keys.txt` 가 `objacc_relcheck` 의 것과 바이트 동일, `relations.log` 빈 파일.
  - order, T8CF facts(`order.sh` → `sem_order`): 두 번 같은 바이트, `order.txt`·`edges.tsv` 가 `objacc_order` 의 것과 바이트 동일, 로그 빈 파일.
  - 다시 뽑기 옵션 없음(`extract.sh off` → `sem_off_extract`), `objacc_off_extract` 와: `basis.kz` 레코드 단위(`check_head.py --whole`: 도구 해시만), `facts.kz` 전체 dump(`check_whole.py`: 머리 해시 넷, STRINGS crc32, `extract.units_from` 만), facts 보기는 `extract.units_from` 한 줄만, `extract.txt` 1 줄, `relations.txt` 2 줄, `basis.txt`·`sha256.txt` 해시 줄만, `args.txt`·`relations.log` 같음.
  - 그 facts 위의 order·relations·키(`order.sh sem_off_extract sem_off_order`): 두 번 같음, `order.txt` 는 `objacc_off_order` 와 2 번째 줄만, `edges.tsv` 같음, `relations.txt` = 다시 뽑기의 것, `keys.txt` 2 번째 줄만, 로그 빈 파일.
  - 계획(`plans.sh` → `sem_plans`; `plans.sh sem_off_extract sem_plans_off`): `compare_plans.py t8cf objacc_plans/<b>`, `extract objacc_plans_off/<b>`: 생성기 정체성 줄, sidecar sha256 줄, module 표·실행 파일 목록 첫 줄, args.txt 의 출력 디렉터리(extract 쪽은 facts·basis 입력·artifact 줄과 그 경로)만.
- **옵션 있는 경로(다섯 빌드)**: 다시 뽑기(`extract.sh on` → `sem_on_extract`: `--object-access` 와 framework 자료 일곱·P0 선언 하나의 `--semantics`), 앞 단계의 옵션 있는 회차 `objacc_on_extract` 와(`compare.sh extract_on`):
  - `basis.kz` 레코드 단위(도구 해시만), `facts.kz` 머리(도구·artifact·basis 해시와 크기만), **섹션 단위**(`section_sha.py` → `section_sha_on.txt`, `compare_sections.py` 가 앞 회차의 `../records/object-access-20261116/section_sha_on.txt` 와): 앞 회차의 모든 섹션이 같은 수·crc32·sha256, 다만 STRINGS 와 BASIS 는 다를 수 있고, 새 섹션은 SEMANTICS 하나.
  - facts 보기(`compare_sem.py`): `[semantics]` 절이 새로, `[basis]` 에 새 항목 넷 무리(`extract.semantics`, `extract.semantics[i]` 여덟, `count.semantics`, `count.semantics_entries[i]` 여덟)와 `extract.units_from` 의 basis 해시, 그 밖은 같음.
  - `extract.txt`: 1 줄(facts 해시)과 새 줄 `semantics …` 하나만. `relations.txt`(`compare_rel_sem.py`): 2 줄, 새 머리 한 줄, layers 줄 끝의 ` sem-unverified(…)` 만. `args.txt`: extract-only 줄만(`--semantics …` 더함). `basis.txt`·`sha256.txt` 해시 줄만. `relations.log` 빈 파일.
  - 그 facts 위의 order·relations·키(`order.sh sem_on_extract sem_on_order`): 두 번 같음, `order.txt` 는 `objacc_on_order` 와 2 번째 줄만, `edges.tsv` 같음, `relations.txt` = 다시 뽑기의 것, `keys.txt` 2 번째 줄만, 로그 빈 파일. 계획(`plans.sh sem_on_extract sem_plans_on`): `compare_plans.py extract objacc_plans_on/<b>` 의 허용만(계획 생성기는 layers 를 읽지 않음).
  - 측정(`compare.sh measure`, `measure_sem.py`): 층·검증 상태별 행 수, framework 칸별 수, 일치의 갈래, 자료마다 적용 수, relations 의 표지 붙은 행 수, 첫 경로(C1 관계 1, 사용 지점 키 `17975444d83f6589`)가 이름 대는 함수의 항목, 계획 site 가 이름 대는 함수의 항목. 측정이고 판정이 아니다.
  - 이 회차는 중간 회차다(계획 4.3 의 관행): 압축 보존하지 않고 섹션별 sha256(`section_sha_on.txt`)을 남긴다.
  - [I](scratchpad 점검 빌드, LLVM Release 구성, 감시 아래; 근거 아님, 지움): SEMANTICS 행 1,773/1,773/1,476/795/786(c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642); 검증된 행/행: P0 30/77, 30/77, 29/75, 28/68, 28/68, framework 1,578/1,690, 1,578/1,690, 1,303/1,395, 639/716, 632/707, status-space 6/6, 6/6, 6/6, 11/11, 11/11. 적용된 framework 항목: c1 두 빌드 cfe-6cccb3f 76·osal-f34759d 82, r2 cfe-7.0.1 71·osal-7.0.1 66, c5_641 cfe-6.4.1 41·cfe-6.4.2 41(바이트가 같은 고정 파일)·osal-4.1.1 33, c5_642 41·41·31; P0 선언 35/35/35/30/30. c1_pre_tx 의 일치: `derived = documented` 21, 문서가 본문이 돌려줄 수 없는 값을 적음 2, 본문이 문서에 없는 값을 돌려줌 23, 비교하지 않음 112. `OS_CountSemGetIdByName`(f34759d): 문서 {0, -2, -13, -17}, 본문은 그에 더해 -35, -16, -33(`OS_ObjectIdConvertToken` 을 거친 것) — "the body returns a value the documentation omits". relations 의 `sem-unverified` 행 262/262/183/86/81. 첫 경로: c1_pre_tx 의 A21 행이 이름 대는 함수 가운데 SEMANTICS 항목이 있는 것 framework 15, P0 6; c1_pre_tx 계획의 site 가 이름 대는 19 함수 가운데 framework 11, P0 3.
- **공간**: 각 빌드·사본·실행은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 빌드 필요량 = 앞 kz-static 빌드 dip 의 최대 256,942,080 + 새 출력 약 200 MB(다시 뽑기 둘 약 100 MB, order 넷과 둘째 실행 약 60 MB, relations 약 8 MB, 계획 셋 약 3 MB, 여유) + 한 시간 흐름 13 MB = 469,942,080 B. 사본은 필요량 100,000,000(도구 파일 둘을 읽어 xz 를 씀; 사본에서 푸는 바이너리는 없음). 실행 단계는 필요량 100,000,000. 점검 빌드(scratchpad)는 감시 아래에서 했고 지웠다(`df_scratch_*.log`).
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py`, `section_sha.py`(앞 단계의 것을 옮김), `compare.sh`, `compare_sem.py`, `compare_rel_sem.py`, `compare_sections.py`, `measure_sem.py`, `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`, `golden_scratch.sha256`; 빌드 전 점검 `tools_check_pre.txt`, `ledger_check_pre.txt`; 훑기의 기록 `semantics_trees.tsv`, `fixture_trees.tsv`, `scan_generate_*.txt`, `scan_check_*.txt`, `fixture_*_*.txt`, `lint_data_*.txt`, `c_library_headers.txt`.

#### 8.0a 빌드 뒤 확인 수의 덧붙임 (다섯 빌드를 돌리기 전에 적고 커밋함) [D][M]

선언 §8.0 은 `test_static_reasons` 의 확인 수를 660 으로 적었고(scratchpad 점검 빌드에서 잰 값), MC 의 갱신 뒤 ctest 는 666 이다(`ctest_counts.log`). 시험은 통과했다. 차이는 선언 밖이므로 여기서 멈추고 까닭을 적는다: 그 시험은 `tests/static/data/` 의 relations 가 든 golden 마다 행의 `incomplete:` 줄을 읽어 확인을 센다(`tests/static/test_static_reasons.cpp:105-138`: 비지 않은 줄 1, 사유 낱낱 1, golden 마다 행 머리·짝·summary 셋). 새 `semantics_np.golden` 은 relations 행 하나에 사유 둘(`no_run`, `writer_set_incomplete`)이라 1 + 2 + 3 = 6 이 더해진다. scratchpad 점검 빌드에서는 새 golden 을 저장소가 아닌 scratchpad 의 자료 디렉터리에 썼으므로 그 시험이 읽지 않았다. 선언의 값을 666(= 660 + 6)으로 고친다. 다른 시험·수는 선언대로다(`test_static_cli` 1058, `test_static_plan` 195, `test_core_schema` 34,017, `lint_selftest` 114, lint 8 규칙 clean, ctest 261 개 실패 0 건너뜀 1).

#### 8.0b 옵션 켠 회차 비교의 덧붙임 (그 비교를 다시 돌리기 전에 적고 커밋함) [D][M]

`compare.sh extract_on`(`records/semantics-20261116/compare_extract_on.txt`, rc 1)에서 두 가지가 선언 §8.0 의 허용 밖으로 나왔다. 나머지 40 줄은 OK 다(facts 보기는 `[semantics]` 와 BASIS 새 항목 18 개뿐, relations·extract·args·basis·sha256 는 선언대로). 선언 밖의 차이이므로 여기서 멈추고 까닭을 적는다.

1. **facts.kz 머리의 `sections=` 줄**: 다섯 빌드 모두 그 줄 하나가 하나 늘었다(c1 `sections=42 | sections=43`; `compare/on_facts_head_<b>.txt`, 다른 줄 1). 선언은 머리에서 도구·artifact·basis 해시와 크기만 허용했고, 새 섹션 SEMANTICS 하나를 선언하면서 그 섹션 수 줄을 허용 목록에 넣지 않았다. 선언을 고친다: 머리는 앞의 허용에 더해 `sections=N` → `sections=N+1` 하나만 허용한다.
2. **섹션 단위 sha256**: `compare/sections_on.txt` 에서 앞 회차의 섹션 가운데 같은 것이 c1·c5 4 개, r2 2 개뿐이고 나머지는 다 달랐다. 까닭: `section_sha.py` 는 kz-dump 의 `section` 줄(바이트의 crc32 를 담음)까지 해시하고, 레코드 바이트의 문자열 참조는 정규 STRINGS 풀(정렬, 참조된 문자열마다 한 번; `lib/core/container_writer.cpp`, `lib/core/container_reader.cpp:275`) 안의 위치다. SEMANTICS 의 새 문자열이 풀에 끼면 그 뒤 문자열의 위치가 밀려, 문자열을 참조하는 모든 섹션의 바이트와 crc32 가 바뀐다. 레코드의 내용은 같을 수 있다: kz-dump 의 레코드 줄은 문자열 참조를 글자로 적는다(예: c5_642 FUNCS 의 레코드 줄은 같고 `section` 줄의 crc32 만 다름). 선언이 문자열을 더하는 회차에서 바이트 수준의 섹션 동일을 기대한 것은 틀렸다.
   선언을 고친다: 섹션은 레코드 글자로 비교한다. `section_text_sha.py` 가 두 회차의 facts.kz 마다 섹션별 레코드 수와 레코드 줄(`section` 줄 빼고)의 sha256 을 적고(`section_text_old.txt`, `section_text_new.txt`), `compare_sections_text.py` 가 빌드마다 앞 회차의 모든 섹션이 같은 수·같은 레코드 글자임을, 다만 STRINGS 와 BASIS 는 다를 수 있고 새 섹션은 SEMANTICS 하나임을 본다(`compare/sections_text_on.txt`). 앞의 `section_sha_on.txt` 는 이 회차의 섹션 바이트 기록으로 그대로 둔다.

두 가지를 `compare.sh extract_on_text` 한 단계로 다시 돌린다(새 스크립트 `check_head_sem.py` 는 `check_head.py` 의 사본에 `--allow-new-section` 만 더함; `check_head.py`, `section_sha.py`, `compare_sections.py` 와 앞 출력은 바꾸지 않음). 출력 `compare_extract_on_text.txt`. 여기서도 OK 가 아닌 줄이 나오면 다시 멈춘다.

#### 8.0c 레코드 글자 비교 스크립트의 고침 (그 비교를 다시 돌리기 전에 적고 커밋함) [D][M]

`compare.sh extract_on_text`(`compare_extract_on_text.txt`, rc 1): 머리는 다섯 빌드 모두 OK(허용 5, 다른 줄 0)다. 섹션은 빌드마다 앞 회차의 42 개 가운데 39 개가 같은 레코드 글자이고, STRINGS·BASIS 밖에 ARG_TYPES 가 달랐다(`compare/sections_text_on.txt`). 까닭은 스크립트에 있다: kz-dump 는 마지막 섹션 뒤에 머리 아닌 끝줄 `artifact_sha256=…` 을 적고, `section_text_sha.py` 는 `section` 줄 뒤의 비지 않은 줄을 다 그 섹션의 것으로 받아, 앞 회차에서는 마지막 섹션 ARG_TYPES 에, 이 회차에서는 SEMANTICS 에 그 줄을 넣었다(c5_642 의 ARG_TYPES 는 레코드 줄 7,459 개가 같고 앞 회차 쪽에 끝줄 하나만 더 있음). 8.0b 가 선언한 "레코드 줄만"과 다르게 짠 것이다.

고침: `section_text_sha_2.py`(`section_text_sha.py` 의 사본, 두 칸 들여 쓴 레코드 줄만 받음)로 같은 비교를 다시 돌린다. `compare.sh` 의 `extract_on_text` 는 `TEXT_ROUND=_2` 일 때 그 사본을 쓰고 출력 이름에 `_2` 를 붙인다(`section_text_old_2.txt`, `section_text_new_2.txt`, `compare/sections_text_on_2.txt`, `compare/on_facts_head_sem_2_<b>.txt`, `compare_extract_on_text_2.txt`). 첫 실행의 스크립트와 출력은 그대로 둔다. 허용은 8.0b 그대로(STRINGS, BASIS, 새 섹션 SEMANTICS 하나)다.

### 8.1 바뀐 것 [F]

- 커밋 차례: `40aa9f5`(범위 규칙 `kairos/docs/semantics_coverage.md` 개정 1, 구성 요소·닻 목록 `semantics_trees.tsv`, 훑기 `env/tools/semantics_scan.py`; 어떤 자료 파일보다 먼저), `0649a43`(자료 파일, 코드, fixture, 시험, 그때까지의 기록; 선언 전의 스냅숏, §8.7), `b02ff21`(선언 §8.0, 비교 스크립트, 빌드 전 목록; 빌드 전), `1db871f`(빌드 기록, 새 golden, 등록부 행, 덧붙임 8.0a; 다섯 빌드 전), `a1d86d9`(8.0a 의 자리 고침), `93800dc`(덧붙임 8.0b), `43b3d98`(덧붙임 8.0c), 그리고 이 절의 커밋.
- 규모(`371ddb0` 에서 `43b3d98` 까지, `kairos/` 와 `env/tools/`): 36 파일, 7,344 줄 더함, 11 줄 지움. 새 `lib/llvm/semantics.cpp` 529 줄·`semantics.hpp` 83 줄(자료 읽기 `load_semantics_file`, 결합 `semantics_rows`, `semantics_revision()` = `semantics join revision 1 (2026-11-16)`), `relations.cpp` +41(머리 한 줄, layers 칸의 `sem-unverified(…)`; relations rules revision 9 그대로), `serialize.{hpp,cpp}` +30(SEMANTICS 는 다른 모든 문자열 뒤에 intern), `cmd_extract.cpp`·`main.cpp`(옵션 `--semantics`, 두 번 쓰기), `facts_text.cpp`(절 `semantics`), `rel_facts.{hpp,cpp}`(SEMANTICS 를 있으면 읽음), `tests/lint/kz_lint.cpp` +91(규칙 `semantics-names`, 선택 시험 다섯), 시험 `test_static_cli.cpp` +96, `static_golden.cpp`, fixture `semantics.c` 140 줄, 자료 `semantics_fixture/` 하나, golden `semantics_np.golden` 136 줄; 훑기 1,142 줄; 자료 `kairos/semantics/` 열셋(framework 의미 일곱 4,312 줄, 이름 공간 다섯 367 줄, P0 선언 105 줄); 문서 `semantics_coverage.md` 123 줄, `static_facts.md` §5t, `schema.md` 날짜 줄.
- 바꾸지 않은 것: `include/`, `cmake/`, 최상위 CMakeLists, `runtime/`, `instrument/`, relations·order·plan 규칙 개정(9, 2, 2), `semantics/framework_api_roles.txt`, 판정 규칙·어휘, 설명 스크립트, `one_build.sh`.

### 8.2 빌드 [M]

MC 에서 제자리, 목표별, 각각 공간 감시 아래(필요량 469,942,080), 모두 첫 시도에 rc 0. 사본 18:43:33Z, 빌드 시작 18:47:15Z(`build_start.txt`), 빌드 전 HEAD `b02ff21`, kairos 아래 커밋 안 된 줄 0(`post_mc.txt`).

| 목표 | 때 (UTC) | dip (B) |
|---|---|---|
| `kz-static` | 18:47:15–18:49:14 | 31,846,400 |
| `kz_lint` | 18:49:20–18:49:32 | 18,477,056 |
| `all` | 18:49:32–18:52:44 | 7,970,816 |

- 다시 링크된다고 적은 15 가운데 7 이 바뀌었고(`libkz-llvm.a`, kz-static, `test_static_reasons`, `test_static_plan`, `test_static_cli`, `static_golden`, kz_lint), 8 은 다시 링크되었으나 같은 바이트다(`test_static_{unroll,flow,elf32,wrappers,gitrev,link,recipe,expr}`; `relink_result.txt`). 선언은 15 모두 "바뀔 수 있음"으로 적었다. `not_relink.sha256` 109 OK.
- 도구(`post_mc.txt`, `identity_kz-static.txt`): kz-static `88f529b1…` → `12a3970f91ce8b9dde10aac22978629d9786623f07b2e2c1b0fa8afb69339b1b`; `--identity` 의 `tool_sha256` `7dba5726…` → `cccffd9782b9c0de71426632192818a4eab24333480fe8d07cfeafcc0e7ff69e`, `tool_source` `d7ae60a8…` → `e4ed3acf1e94cef7336935078bf659ee668c5e5d3293b529834170fcc7b88029`; `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 그대로(선언의 [I] 대로). kz_lint `5f62ec20…` → `3d4d5a8faf6995d367ba428412f3d373615d064f45e009e9fb3a907ec38e91e5`. kz, kz-dump, kz-phi, kz-rt-dump, kz-instr-clang 그대로. `CMakeCache.txt` `bc2f43b4…` 그대로.
- 도구 보존: 빌드 전 kz-static `88f529b1`·kz_lint `5f62ec20` 을 `/home/user/work/preserve/tools-20261116-i/` 에 `kz-static.88f529b1.xz`(43,696,080 B), `kz_lint.5f62ec20.xz`(956,516 B)로 사본, 왕복 확인(`tool_copy/ROUNDTRIP.txt`). 등록부: 사본 직후 in-place+copy, 빌드 뒤 copy, 새 `12a3970f`·`3d4d5a8f` in-place(`tools_check_after_copy.txt`, `tools_check_after_build.txt` 43 행 실패 0).

### 8.3 시험 [M]

- 첫 ctest(`ctest_first.log`): 261 개 가운데 실패 1, 선언한 그것(`static_golden_semantics_np`, golden 없음). 건너뜀 1.
- `KZ_GOLDEN_UPDATE=1` 로 그것만 썼다(`golden_update.log`). scratchpad 점검 빌드의 것과 같은 sha256(`golden_scratch.sha256` `bcdfb9a7…`). 다른 golden 은 바뀌지 않았다.
- 갱신 뒤 전체 ctest(`ctest_full.log`): 261 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`).
- 확인 수(`ctest_counts.log`): `test_static_cli` 1,058, `test_static_plan` 195, `test_static_reasons` 666(선언 660 은 8.0a 에서 고침), `test_core_schema` 34,017, `lint_selftest` 114 경우 실패 0, lint 시험 여덟(선택 시험과 규칙 일곱; 선언의 "규칙 7 → 8" 은 이 시험 수) 모두 통과, 규칙 일곱 모두 clean(`case-ids`, `case-ids-data`, `semantics-names`, `retired-term`, `banned-files`, `no-llvm`, `no-generated`).
- 단위 점검 `run_unit.sh` 15/15(`unit_tools.txt`), `step2a_tests_unit.sh` 7/7(`unit_step2a.txt`).
- 훑기의 출처 점검을 다섯 빌드 뒤에 다시: `scan_check_4.txt` PASS 13 파일 실패 0, `fixture_check_3.txt` PASS 1 파일.

### 8.4 바이트 동일성 [M]

| 비교 | 결과 |
|---|---|
| relations, T8CF facts (`compare_relations.txt`) | 15 OK: `relations.txt`·`keys.txt` 가 `objacc_relcheck` 와 바이트 동일, 로그 빈 파일 |
| order, T8CF facts (`compare_order.txt`) | 25 OK: 두 번 같음, `objacc_order` 와 바이트 동일 |
| 다시 뽑기 옵션 없음 vs `objacc_off_extract` (`compare_extract.txt`) | 45 OK: `basis.kz` 허용 2(도구 해시), `facts.kz` 전체 dump 허용 6(c1_pre_tx 162,142 줄), facts 보기 `extract.units_from` 한 줄만(149,111 줄), 나머지 선언대로 |
| 그 facts 위 (`compare_onextract.txt`) | 40 OK |
| 계획 (`compare_plans.txt`) | 열 모두 “only declared differences”(T8CF 위 다섯, 옵션 없는 다시 뽑기 위 다섯) |
| 다시 뽑기 옵션 있음 vs `objacc_on_extract` (`compare_extract_on.txt`) | 40 OK, BAD 6: 머리 다섯과 섹션 바이트(선언 밖, §8.0b) |
| 같은 것, 8.0b 의 허용으로 (`compare_extract_on_text.txt`) | 머리 5 OK; 섹션 BAD(스크립트 결함, §8.0c) |
| 같은 것, 8.0c 의 고친 스크립트로 (`compare_extract_on_text_2.txt`) | 6 OK: 머리 다섯 허용 5 다른 줄 0; 섹션은 빌드마다 앞 회차의 42 가운데 40 이 같은 레코드 글, 다른 것 STRINGS·BASIS, 새 것 SEMANTICS 하나(`compare/sections_text_on_2.txt`) |
| 그 facts 위 (`compare_onextract_on.txt`) | 40 OK |
| 계획 (`compare_plans_on.txt`) | 다섯 “only declared differences” |

- **옵션 없는 경로는 앞판과 같다**(위 첫 다섯 줄). 다른 것은 도구 정체성에서 오는 값과 계획의 생성기 정체성 줄뿐이다.
- **옵션 있는 경로**: facts 보기는 빌드마다 `[semantics]` 절과 BASIS 새 항목 18 개뿐이고 그 밖의 절은 같다(`compare/on_<b>.txt`). relations 는 2 번째 줄, 새 머리 한 줄, 표지 붙은 layers 줄뿐이다(268/268/186/100/95 줄, 그 가운데 행 262/262/183/86/81, `namespace layers:` 줄 6/6/3/14/14; `compare/on_relations_<b>.txt`). order 의 `order.txt`·`edges.tsv`·`keys.txt` 는 앞 회차와 같다(2 번째 줄 밖). 계획 생성기는 layers 를 읽지 않으므로 계획도 허용 밖 차이가 없다.
- **누적 목록과 등록부**: 빌드 전 `ledger_check_pre.txt` 94 행·`tools_check_pre.txt` 41 행 실패 0; 사본 뒤 `tools_check_after_copy.txt`; 빌드 뒤 `ledger_check_after_build.txt` 94 행, `tools_check_after_build.txt` 43 행 실패 0(deep); 장부 행 열을 더한 뒤 `ledger_check_end.txt` 104 행 실패 0(새 행 `OBS_sem_relcheck`, `OBS_sem_order`, `OBS_sem_off_extract`, `OBS_sem_off_order`, `OBS_sem_on_extract`, `OBS_sem_on_order`, `OBS_sem_plans`, `OBS_sem_plans_off`, `OBS_sem_plans_on`, `TOOLS_20261116i`), 행 설명 한 줄을 고친 뒤 `ledger_check_end_2.txt` 104 행 실패 0, `tools_check_end.txt` 43 행 실패 0(deep).
- 바깥 이름 점검(`--tree`)은 아홉 출력 디렉터리, 사본 디렉터리, 기록 디렉터리에서 깨끗하다(`name_check_tree.txt`, 11 모두 rc 0).

### 8.5 측정 [M]

옵션 있는 다시 뽑기(`sem_on_extract`), kz-static `12a3970f`. 수의 출처: `compare/measure_<b>.txt`(`measure_sem.py`), `compare/agreement_<b>.txt`, 자료 파일 수는 `scan_check_4.txt`. 측정이고 판정이 아니다. 모든 값이 선언 §8.0 의 scratchpad [I] 와 같다.

**범위(자료).** framework 의미 항목 741(osal-f34759d 128/공개 선언 153, cfe-6cccb3f 129/182, osal-7.0.1 150/178, cfe-7.0.1 133/196, osal-4.1.1 55/118, cfe-6.4.1 73/131, cfe-6.4.2 73/131), 이름 공간 종류 37(osal-f34759d 11, osal-7.0.1 12, osal-4.1.1 10, cfe-6.4.1 2, cfe-6.4.2 2), P0 선언 85/85.

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| SEMANTICS 행 | 1,773 | 1,773 | 1,476 | 795 | 786 |
| P0 층: 검증됨/행 (doc-collated, standard-collated, unverified) | 30/77 (28, 2, 47) | 30/77 (28, 2, 47) | 29/75 (27, 2, 46) | 28/68 (26, 2, 40) | 28/68 (26, 2, 40) |
| framework 층: 검증됨/행 (body-derived, doc-collated, unverified) | 1,578/1,690 (999, 579, 112) | 1,578/1,690 | 1,303/1,395 (820, 483, 92) | 639/716 (429, 210, 77) | 632/707 (422, 210, 75) |
| status-space 층 | 6/6 | 6/6 | 6/6 | 11/11 | 11/11 |
| 적용된 framework 항목(정의로/선언으로) | cfe-6cccb3f 76 (76/0), osal-f34759d 82 (82/0) | 같음 | cfe-7.0.1 71, osal-7.0.1 66 | cfe-6.4.1 41 (41/0), cfe-6.4.2 41 (24/17), osal-4.1.1 33 | cfe-6.4.1 41 (24/17), cfe-6.4.2 41 (41/0), osal-4.1.1 31 |
| 적용된 P0 선언(속성 행) | 35 (28) | 35 (28) | 35 (27) | 30 (26) | 30 (26) |
| SEMANTICS 행이 있는 함수 P0 / framework | 36 / 158 | 36 / 158 | 35 / 137 | 30 / 74 | 30 / 72 |
| 일치: 같음 / 문서가 본문이 돌려줄 수 없는 값을 적음 / 본문이 문서에 없는 값을 돌려줌 / 비교 못 함 | 21 / 2 / 23 / 112 | 21 / 2 / 23 / 112 | 19 / 2 / 24 / 92 | 28 / 8 / 2 / 77 | 28 / 8 / 2 / 75 |
| 비교 못 함의 까닭: 본문의 집합이 서지 않음 / 문서 반환 없음 | 111 / 1 | 111 / 1 | 91 / 1 | 40 / 37 | 40 / 35 |
| relations: framework 항목이 있는 함수를 이름 대는 행 / 그 가운데 검증 안 된 항목이 있는 행 | 274 / 262 | 274 / 262 | 193 / 183 | 147 / 86 | 142 / 81 |

- **검증됨/전체 항목**: 빌드에 적용된 framework 항목은 c1 158/741, r2 137/741, c5_641 115/741(두 6.4 판 41 씩이 겹침, §8.7), c5_642 113/741. 적용되지 않은 판의 항목은 그 빌드에서 쓰이지 않는다(각 판의 "called but neither pin held" 수가 `count.semantics_entries[i]` 에 있음). 행 단위의 검증됨/전체는 위 표의 층별 값이다. framework 층의 `unverified` 는 모두 일치 칸의 "비교 못 함"이고, documented-return·arg-domain·arg-role 은 모두 doc-collated, success·failure·role 은 모두 body-derived 다.
  - (2026-11-19 덧붙임, PI 결정 ①) 이 줄의 분자는 그 빌드에 적용된 항목의 수이고 '검증됨'의 수가 아니다. 741 은 일곱 판 파일의 끝줄 `entries=` 의 합이다(`declared=` 의 합 1,089, 서로 다른 `api` 이름 310). c1 의 158 은 그 빌드의 두 판(cfe-6cccb3f 129 항목, osal-f34759d 128 항목; 합 257) 가운데 적용된 것(76 + 82, 위 표의 '적용된 framework 항목' 줄)이다. 일치 칸의 158 줄 가운데 112 가 `unverified` 이고 46 이 `body-derived` 다(`../records/semantics-20261116/compare/agreement_c1_pre_tx.txt`). doc-collated 는 빌드의 판 고정 머리말(문서나 선언)에서 읽은 것이지 구현과 대조한 것이 아니다(`kairos/docs/semantics_coverage.md:111`). 수는 `../records/closure-notes-20261119/check.txt` I. [§26.1]
- **첫 관측 경로가 쓰는 항목**(C1 관계 1, 사용 지점 키 `17975444d83f6589`, c1_pre_tx 의 A21 행): 그 행이 이름 대는 함수 가운데 framework 항목이 있는 것 15(`CFE_ES_ExitApp`, `CFE_ES_ResetCFE`, `OS_BinSemCreate`, `OS_CondVarCreate`, `OS_CountSemCreate`, `OS_CountSemGetIdByName`, `OS_DirectoryOpen`, `OS_FileSysAddFixedMap`, `OS_ModuleLoad`, `OS_MutSemCreate`, `OS_OpenCreate`, `OS_QueueCreate`, `OS_SocketBind`, `OS_TaskCreate`, `OS_TimeBaseCreate`), P0 항목 6(`abort`, `dlsym`, `exit`, `memset`, `pthread_exit`, `strcmp`). 곧 적용된 158 framework 항목 가운데 15, 적용된 P0 선언 35 가운데 6. 그 15 가운데 13 은 unverified 칸이 하나씩 있다(일치를 비교 못 함). c1_post_tx 의 같은 키 행은 16 과 7 이다(`OS_TaskDelay`, `clock_nanosleep` 더함). c1_pre_tx 계획의 site 가 이름 대는 19 함수 가운데 framework 항목이 있는 것 11, P0 3(`memset`, `pthread_create`, `strncpy`). r2·c5 에는 그 키가 없다.
- **`OS_CountSemGetIdByName`**(osal-f34759d, 첫 경로의 사용 호출): 문서 반환 {0, -2, -13, -17}, 본문은 그에 더해 -35, -16, -33(`OS_ObjectIdConvertToken` 을 거친 것)을 돌려준다: "the body returns a value the documentation omits". 본문 8 항목 body-derived, 문서 4 항목 doc-collated.
- **비교 못 함의 큰 몫**은 본문이 돌려주는 값의 집합이 서지 않는 함수다(c1 111): 따라가지 못한 반환(간접 호출의 결과, 예: `OS_NotifyEvent` 가 `<indirect>` 의 결과를 돌려줌 — c1_pre_tx 에서 이 까닭을 적은 일치 행 16)이나, 상수가 뒤에 다시 정의되는 정의를 지나 반환에 닿는다고 보이지 않는 것(relations rules revision 9 와 같은 표시)이다. c5 의 문서 반환 없음 37/35 는 OSAL 4.1.1 에 함수별 문서가 없기 때문이다(규칙 §3).
- **P0**: 검증된 P0 행은 선언 속성(doc-collated: 머리말의 `__attribute_pure__`, `__nonnull` 등)과 로컬 사본이 있는 `pthread_create` 의 두 줄(역할과 arg 0 의 out 동작, standard-collated)이다. 나머지 P0 행위 문장(역할, out 인자, 막힘)은 링크하는 판의 글이 이 환경에 없어 `unverified` 다.
- **2026-11-18 검토 뒤 정정: §12.7 `rules-03`, `rules-04`, `rules-05`, `rules-07`.** (1) `osal-f34759d 11` 은 항목 수다: 서로 다른 종류는 10(STREAM 이 둘). 파일에 종류 macro(`LOCAL_OBJID_TYPE`)가 없는 종류(TIMECB, CONDVAR, CONSOLE)는 자료에 없고, 종류 상수를 직접 넘기는 TIMEBASE 는 `delete`·`stored-name` 만 있고 `lookup`·`create` 가 없다(`framework_namespaces_osal-f34759d.txt:100-102`; 7.0.1 도 같은 꼴). 층의 자체 지우기(`osapi-idmap.c:640`)와 실패한 생성의 되돌림(`:836-846`)은 어느 항목에도 없다(OSAL 자료에 `clear`·`retire` 줄 0). 범위 규칙의 글(“with the kind”)과 스크립트(“with the kind macro”)가 달랐다; `semantics_coverage.md` §7 에 적었다. 규칙의 개정과 자료의 다시 만들기는 뒤 단계다. (2) c5 두 빌드의 검증된 행 수에는 틀린 행이 들어 있다: `CFE_ES_RunLoop` 의 `framework success … value 0 body-derived [FALSE]` 와 `framework failure … value 1 body-derived [TRUE]`(적용된 판 파일마다 둘; 자료는 `return=none`, `cfe_es.h:536-537` 은 TRUE 가 계속)이고, `CFE_ES_ExitApp` 의 `documented-return … doc-collated` 는 void 선언 함수의 문서 반환이다(`cfe_es.h:515`). status space 의 severity 상수 넷은 `failure` 칸에 적혀 있었다(성공 severity 0 도). SEMANTICS 의 `failure` 칸은 “성공 상수가 아닌 body 상수”였고 RESULT_EFFECT 의 outcome 과 다른 정의였다(c1_pre_tx 에서 정보 코드 `CFE_TBL_INFO_UPDATED 1275068430` 등이 SEMANTICS 는 failure, RESULT_EFFECT 는 other). 모두 코드로 고쳤고(outcome class rule revision 1, `static_facts.md` §5w) MC 도구는 아직 앞판이라 이 절의 수는 다시 재지 않았다(§12.6).

### 8.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 18:43:33 | 2,535,858,176 | 사본 시작(dip 44,679,168) |
| 18:47:15 | 2,491,117,568 | `make kz-static`(가장 낮은 2,459,271,168, dip 31,846,400), `kz_lint`(18,477,056), `all`(7,970,816) |
| 18:58:06–19:03:49 | 2,470,817,792 → 2,299,109,376 | relations(dip 7,675,904), order(14,209,024), 다시 뽑기 옵션 없음(47,161,344), 그 order(21,856,256), 계획 둘(987,136 씩), 다시 뽑기 옵션 있음(55,816,192), 그 order(22,020,096), 계획(991,232) |
| 19:04:20–19:10:31 | 2,320,371,712 → 2,278,916,096 | 비교(facts 보기·dump 는 scratchpad, 끝에 지움; 가장 낮은 2,254,970,880, `extract_on` 의 dip 65,351,680) |
| 이 절을 쓸 때 | 2,317,590,528 | 끝의 장부·등록부·이름 점검 뒤 |

- 하한(314,572,800)을 넘지 않았다. 빌드 dip 은 필요량 안이었다.
- 남긴 출력(장부 행): `sem_relcheck`, `sem_order`, `sem_off_extract`, `sem_off_order`, `sem_on_extract`, `sem_on_order`, `sem_plans`, `sem_plans_off`, `sem_plans_on` 합 149,472,264 B; 사본 44,652,596 B. 지운 것: scratchpad 의 점검 빌드와 그 출력(선언 전), 비교·측정의 facts 보기와 dump(scratchpad), order 의 둘째 실행 디렉터리(첫 실행과 같은 바이트임을 본 뒤, 앞 단계와 같은 관행)뿐이다.

### 8.7 계획과 다르게 한 것

- **선언 전의 스냅숏 커밋**: `0649a43`(자료 파일, 코드, fixture, 시험, 그때까지의 기록)이 선언 커밋 `b02ff21` 보다 먼저 들어갔다. 범위 규칙과 훑기는 그보다 앞선 `40aa9f5` 에 있고, 빌드·다섯 빌드 실행·비교는 모두 `b02ff21` 뒤다.
- **P0 의미의 자리**: 설계 연구가 따로 두자고 한 P0 의미 섹션 대신 SEMANTICS 의 P0 층을 썼다(schema 개정 12 의 정의; 선언 §8.0 에 적음).
- **문서 읽기의 자리**: 주석의 `@retval`·`\arg` 읽기를 추출기(`ast_tu.cpp`)가 아니라 훑기 스크립트가 한다. 추출기는 주석을 읽지 않고, 자료 파일의 file:line 과 sha256 으로 빌드에 잇는다.
- **범위 규칙의 E2·E3**: 연구의 형 기준(E1)만으로는 여러 판의 상태·event-type 값이 정수형이어서 항목이 빠지므로 문서 기준(E2)과 본문 기준(E3)을 더했다(규칙 §3, 자료 전에 커밋).
- **닻은 판마다 손으로 적었다**: `semantics_trees.tsv` 의 줄 범위·이름은 손으로 적고, 훑기가 그것을 file:line 으로 풀고 점검한다. 닻을 기계적으로 찾지는 않는다.
- **P0 선언 파일을 한 번 다시 만들었다**(선언 전): 첫 판이 로컬 사본의 절대 경로를 담아 `case-ids-data` 에 걸렸다; 훑기가 파일 이름만 쓰도록 고쳤다.
- **확인 수 하나**(8.0a): `test_static_reasons` 660 → 666. 그 덧붙임을 처음에 앞 절(6.0)에 잘못 넣어 `a1d86d9` 에서 옮겼다(글은 같음).
- **다시 링크된 15 가운데 8 은 같은 바이트**였다. 선언은 바뀔 수 있다고만 적었다.
- **옵션 켠 회차의 비교 둘**(8.0b): 머리의 `sections=` 줄과 섹션 바이트 비교를 선언이 너무 좁게 적었다. 문자열 풀이 정렬된 정규 풀이라 문자열이 더해지면 모든 섹션의 바이트가 바뀐다. 레코드 글 비교로 바꾸었고, 그 첫 스크립트가 dump 의 끝줄을 마지막 섹션에 넣는 결함이 있어(8.0c) 사본으로 고쳐 다시 돌렸다. 앞 스크립트와 출력은 그대로 남겼다.
- **두 6.4 판의 항목이 c5 의 두 빌드 모두에 적용된다**: cfe-6.4.1 과 6.4.2 의 고정 파일 가운데 바이트가 같은 것이 있어, c5_641 에 6.4.2 항목 41(정의 파일로 24, 선언 머리말로 17)이, c5_642 에 6.4.1 항목 41(같은 갈래)이 함께 적용된다. 같은 API 에 판이 다른 두 줄이 생긴다(`component` 칸으로 구별). 선언의 [I] 에 적었지만 결합 규칙이 판 하나를 고르지 않는다는 것은 적지 않았다.
- **가벼운 비교 여섯**(relations, order, onextract, plans, onextract_on, plans_on)은 공간 감시 없이 돌렸다. 기록 디렉터리에 글 파일만 쓰고(그 밖에는 order 의 둘째 실행 디렉터리를 지울 뿐) 그때 여유는 2,299,109,376 B 이상이었다. 무거운 비교 넷과 측정은 감시 아래 돌렸다(`df_compare_*.log`).
- **2026-11-18 검토 뒤 정정: §12.7 `builds-04`.** scratch 의 다섯 빌드 추출과 facts 보기는 공간 감시 기록 없이 돌렸다(남긴 `df_scratch_*` 는 cmake·make 뿐).

### 8.8 열린 것

- **이 층이 방법이 받아들이는 framework adapter 층인가**: 판이 고정된 framework 소스에서 기계적으로 만든 의미 자료(`framework-layer data`)다. 그 층인지는 PI 가 정할 물음이고, 이 단계는 그렇다고 가정하지 않았다(규칙 머리).
- **P0 의 대조**: 행위 문장 47/77(c1) 등은 `unverified` 다. 링크하는 판의 글(로컬 사본 밖의 glibc 2.39 소스, POSIX 글)이 새 외부 자원이라 하지 않았다. 배포 패치도 대조하지 않았다(changelog 에 `pthread_create` 줄 1).
- **범위 밖**: 플랫폼 지원 묶음과 응용, framework 의 단위 시험 stub. cFE 6.7.99(6cccb3f)·7.0.1 의 이름 공간은 저장된 이름이 필드 경로가 아니라 접근 함수로 닿아 규칙 T 가 잡지 못하고, 그 두 판의 이름 공간 파일은 없다(규칙 §7).
- **이름 공간 자료는 kz-static 이 읽지 않는다**: 판정 규칙 8.2 조건 (가)의 입력으로 만들었고, 읽는 쪽(설명 스크립트의 다음 개정)은 다음 단계다 [I].
- **읽기 전용 효과**(§7.8 첫 항목): `strcmp`·`strlen` 등의 pure 선언이 이제 doc-collated 로 SEMANTICS 에 있으나, writer 집합의 계산은 아직 그것을 쓰지 않는다. 쓰는 것은 뒤 단계다.
  - (2026-11-19 덧붙임) `strlen` 은 P0 선언 자료에 항목이 없다(`kairos/semantics/p0_declarations_glibc-2.39-headers.txt` 에 `strlen` 이 든 줄 0). 그 자료에 pure 선언으로 있는 것은 `strcmp`(같은 파일 93 줄) 같은 비교 함수다. 수는 `../records/closure-notes-20261119/check.txt` J. [§26.1]
- **문서와 본문이 다른 함수**: 문서가 본문이 돌려줄 수 없는 값을 적은 것 2/2/2/8/8, 본문이 문서에 없는 값을 돌려주는 것 23/23/24/2/2. 판정 규칙이 어느 쪽을 쓰는지(예: `OS_CountSemGetIdByName` 의 -35·-16·-33) 정해야 한다.
- **두 6.4 판의 겹침**(§8.7): 결합이 빌드마다 판 하나를 고를지, 겹침을 그대로 둘지.
- **루트 밖의 절대 키**(§7.8): 그대로다.
- 이 회차는 중간 회차다: 섹션별 바이트 sha256(`section_sha_on.txt`)과 레코드 글 sha256(`section_text_new_2.txt`)을 남겼다. 앞 회차와의 비교는 레코드 글로 한다(8.0b).

## 9. 결과 흐름과 효과: 추출 옵션 `--result-flow`, relations rules revision 10, order rules revision 3 (kz-static 빌드 하나)

호출 결과가 어디로 흐르는지(지역의 도달 정의, 그 결과를 읽는 조건 아래의 호출)와 callee 가 돌려줄 수 있는 값마다 그 값이 호출자 몸에서 무엇으로 이어지는지(return, 저장, event 보내기, 응용 종료, terminal, 재시도, 아무것도 없음)를 facts 의 두 섹션 RESULT_FLOW·RESULT_EFFECT 로 쓴다. relations 는 그 두 섹션이 있을 때만 경로 A·B 행에 처리 표지를 사실에서 붙이고, 생성자의 성공 값이 그 함수의 return 에 닿는지를 적는다. order 는 그 두 섹션이 있을 때만 child without success 를 안쪽 생성 호출의 효과로 읽고, 대기 함수의 호출자 쪽 사용을 guard 행으로 찾는다. 기록: `../records/result-flow-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `rf_*`. 도구 사본: 새 디렉터리 `/home/user/work/preserve/tools-20261116-j/`. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 9.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다.

- **무엇을 하는가** (`kairos/docs/static_facts.md` §5u):
  1. **옵션** `kz-static extract --no-profile --result-flow`(프로필 없는 방식에서만; 프로필 방식에서는 종료 코드 2 와 `kz-static extract: --result-flow needs the profile-free mode (--no-profile)`). 옵션이 없으면 추출의 코드 경로와 출력은 앞판 그대로다(모든 새 읽기가 `spec.result_flow` 아래에 있음).
  2. **모든 회차**: 함수마다 바꾸지 않고 돌려주는 매개변수(그 매개변수에 대입·증감·주소 취함이 없는 return), 호출 인자마다 그 상수 또는 그 값이 결과인 호출. 첫 회차의 RETURNS 로 함수마다 돌려줄 수 있는 값을 만든다(새 `lib/llvm/result_flow.cpp`, `result flow rules revision 1 (2026-11-16)`): 상수, 호출 결과(그 호출의 값; 매개변수는 그 호출의 인자로 풂), 바꾸지 않고 돌려주는 매개변수(각 호출에서 인자로 풂), 그 밖은 따라가지 않음. 몸이 없는 함수는 따라가지 않음, P0 스레드 생성은 0 과 "그 밖"(POSIX.1-2017 `pthread_create` RETURN VALUE).
  3. **RESULT_FLOW**(둘째 회차, 섹션 142): 결과가 있는 호출마다 그 결과가 닿는 사용(분기 조건·return·호출 인자·저장·그 밖)을 CFG 의 도달 정의로(`unique`/`shared`/`undef-path`), 조건 안의 지역은 그 자리에 닿는 정의로 이름 붙여(`x@def:L:C`, `x@defs:L:C|…[|undef]`, `x@undef`) 적는다. 호출 결과를 읽는 조건 아래에서 도는 호출마다 **guard 행**(def none, use none)에 그 조건의 곱을 적는다.
  4. **RESULT_EFFECT**(둘째 회차, 섹션 143): 결과가 분기에서 시험되거나 return 되는 호출마다, callee 가 돌려줄 수 있는 값마다(따라가지 않은 return 이 있거나 P0 스레드 생성이면 열거 밖의 값 하나 더) CFG 를 그 호출 다음부터 걸어, 그 값에만 닿는 블록의 효과를 적는다: 호출(SEMANTICS 의 인자 역할 `event-type` 이면 event-send, `run-status` 이면 exit-app, 호출 표지 terminal-op 이면 terminal), 저장, 닿는 모든 return(return-result/return-const/none open), 재시도, fallthrough, 저장한 위치나 그 값을 인자로 받는 뒤의 종료 호출. 그 값이 정하지 않는 분기를 지난 효과는 open 이고 까닭을 적는다.
  5. **relations rules revision 10**(그 두 섹션이 있을 때만): 머리 두 줄(`rules with result flow: relations rules revision 10 (2026-11-16), …`, `handling labels (…): …`), 경로 A 행에 `absence:`·`handling_label:`, 경로 B 행에 `handling_label:`, 생성자 callee 에 `callee-check (result flow):`·`callee-check (result flow, success):` 줄. `rules` 줄은 `relations rules revision 9` 그대로이고 그 밖의 줄은 바뀌지 않는다. 표지(R, H, T?, J, J?, T, none)는 처리 사실이며 판정이 아니다.
  6. **order rules revision 3**(그 두 섹션이 있을 때만): 머리 한 줄 `rules with result flow: order rules revision 3 (2026-11-16), …`. child without success 를 안쪽 생성 호출의 성공 값 RESULT_EFFECT 행으로 읽는다(그 행이 없으면 revision 2 의 위치 규칙). 대기 함수의 호출이 결과를 시험·사용하면, 예산 출구가 return 이고 그 값이 상수일 때 그 결과를 이름 붙은 정의로 읽는 guard 행의 호출을 호출자 쪽 사용(`use caller-side:`)으로 적고, 예산 값으로 그 guard 를 평가해 skip/continue 이면 HB-await 간선을 그린다. 첫 줄 `kz-static order (order rules revision 2 (2026-11-16))` 는 그대로다.
  7. `kz-static show` 의 절 `resultflow`, `resulteffect`. 오프라인 탐침의 새 판 `plan/ovm_rev1_20261116/probe/s1_rev2.py`(앞판 `s1_rev1.py` 그대로): 행에 `handling_label:` 이 있으면 그 표지를 쓰고, 글에서 얻은 표지와 나란히 적는다(`[개정 2]`).
- **레이아웃 점검.** 쓰는 코드가 개정 12 의 `kz_fact_rflow`·`kz_fact_reffect` 와 열거형 `KZ_RFLOW_DEF/USE/REACH/FLAG`, `KZ_REFF_OUTCOME/REACH/EFFECT/STATUS/FLAG` 에 그대로 맞는다. `include/` 는 바꾸지 않는다(바꾸면 kz·kz-dump·kz-phi 의 정체성도 바뀜). guard 행은 따로 섹션을 두지 않고 RESULT_FLOW 의 def none·use none 행으로 쓴다(`docs/schema.md` 의 날짜 붙은 줄, `static_facts.md` §5u).
- **1차 근거.**
  - 첫 관측 경로(C1 관계 1, 사용 지점 키 `17975444d83f6589`, c1_pre_tx 의 A21 행)에서 앞판 relations 의 `callee-check (set): OS_CountSemCreate: not established (2 returns not followed, 14 constants not shown to reach the return)`(`/home/user/work/redesign/obs1/sem_on_order/c1_pre_tx/relations.txt` 의 A21 행): 성공 상수가 생성 함수의 return 에 닿는지 앞판은 보이지 못했다. c1_pre_tx 가 읽는 OSAL(`/home/user/work/procrace/c1_build/bundle_1df262a/osal`)의 `OS_CountSemCreate` 는 `:103` 의 `OS_CountSemCreate_Impl` 결과를 `:106` 의 `OS_ObjectIdFinalizeNew(return_code, &token, sem_id)` 에 넘겨 그 결과를 `:109` 에서 돌려주고(`src/os/shared/src/osapi-countsem.c`), `OS_ObjectIdFinalizeNew` 는 첫 매개변수를 바꾸지 않고 돌려준다(`src/os/shared/src/osapi-idmap.c:828` 의 정의, `:863` `return operation_status;`). 그래서 매개변수를 통한 성공 값의 흐름이 필요하다.
  - 그 행의 처리 효과: `CF_CFDP_InitEngine` 은 lookup 의 결과가 `OS_SUCCESS` 가 아니면 `CFE_EVS_SendEvent(CF_EID_ERR_INIT_SEM, CFE_EVS_EventType_ERROR, …)` 를 부르고 루프를 벗어나 lookup 의 결과를 돌려주며(`/home/user/work/procrace/c1_build/pre/src/cf/fsw/src/cf_cfdp.c:994-999`), `CF_AppMain` 은 `while (CFE_ES_RunLoop(&CF_AppData.run_status))` 뒤 `CFE_ES_ExitApp(CF_AppData.run_status)` 로 끝난다(같은 디렉터리의 `cf_app.c:338,366`). 앞판 relations 는 이를 `handling:` 줄의 글로만 적었다(같은 파일의 A21 행).
  - 표지의 뜻은 판정 규칙 3.4 의 처리 사실(R, H, J, T?)이고, T(시험하고 이어감)와 J?(버려짐·부재 값 미평가)는 탐침 개정 2 가 더한 것이다(`s1_rev2.py` 머리).
- **코드**(`kairos/`): 새 `lib/llvm/result_flow.{hpp,cpp}`; `lib/llvm/{model.hpp,spec.hpp,ast_fn.cpp,wrappers.cpp,serialize.cpp,facts_text.cpp,rel_facts.hpp,rel_facts.cpp,relations.hpp,relations.cpp,order.hpp,order.cpp,CMakeLists.txt}`, `tools/kz-static/{cmd_extract.cpp,main.cpp}`; 시험 `tests/static/{test_static_cli.cpp,static_golden.cpp,CMakeLists.txt}`, 새 fixture `tests/static/fixtures/resultflow.c`; 문서 `docs/static_facts.md` §5u, `docs/schema.md` 의 날짜 붙은 줄. 저장소의 그 밖: `plan/ovm_rev1_20261116/probe/s1_rev2.py`(새), `env/tools/tests/run_unit.sh` 의 점검 16(탐침 개정 2 의 stub 입력).
  - 하지 않는 것: `include/`, `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `instrument/`, `tests/lint/`(kz_lint 는 다시 링크되지 않음), `semantics/` 의 자료, 계획 생성기의 규칙(새 섹션을 읽지 않음; event 보내기와 그 형을 계획에 올리는 것은 뒤 단계), 판정 규칙과 어휘 자료, 설명 스크립트, `one_build.sh`(옵션은 `ONE_BUILD_EXTRACT_EXTRA` 로), `s1_rev1.py`.
- **도구 해시와 정체성**: kz-static 의 파일 sha256(빌드 전 `12a3970f91ce8b9dde10aac22978629d9786623f07b2e2c1b0fa8afb69339b1b`), `--identity` 의 `tool_sha256`(빌드 전 `cccffd97…`)·`tool_source`(빌드 전 `e4ed3acf…`)가 바뀐다. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 는 그대로 [I]. 나머지 도구의 정체성과 파일은 그대로(kz_lint `3d4d5a8f…` 포함).
  - 다시 링크되는 것 14(`relink_expected.sha256`, 빌드 전 sha256; 바뀔 수 있음): `libkz-llvm.a`, kz-static, `libkz-llvm.a` 를 링크하는 시험 프로그램 열(`test_static_{reasons,plan,unroll,flow,elf32,wrappers,gitrev,link,recipe,expr}`), 소스가 바뀐 `test_static_cli`·`static_golden`.
  - 다시 링크되지 않아야 하는 것 110(`not_relink.sha256`, 빌드 전 sha256: 앞 단계의 109 와 kz_lint; 앞 단계 목록의 109 줄과 같음을 확인함; kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz-instr-clang `28db4017…`, kz_lint `3d4d5a8f…` 포함). 빌드 뒤 sha256 이 같아야 한다.
  - `CMakeCache.txt` `bc2f43b4…` 그대로 [I](`lib/llvm`·`tests/static` 의 CMakeLists 가 바뀌어 다시 구성된다).
- **빌드**(MC `/home/user/work/merge-check/kz-llvm` 제자리, 목표별, 각각 공간 감시 아래): `make kz-static`, `make all`. 감시가 도중에 멈추면 적고, kz-static 이 새 `--identity` 를 적을 때까지 출력을 내지 않는다.
- **도구 보존**: 빌드 전에 kz-static `12a3970f` 를 새 디렉터리 `/home/user/work/preserve/tools-20261116-j/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 공간 감시 아래). 사본 직후 등록부의 kz-static 행을 in-place+copy 로, 빌드 뒤 copy 로 고치고 새 kz-static 을 in-place 로 더한 다음에 빌드 뒤 등록부 점검을 돌린다. 장부: 사본 디렉터리 행 `TOOLS_20261116j`.
- **golden**(빌드 전 scratchpad 점검 빌드로 미리 만듦 [I, 근거 아님]; `golden_scratch.sha256`: `resultflow_np.golden` `041b4ab2…`, `resultflow_order_np.golden` `a03cc621…`): 새 golden 둘. **첫 ctest 에서 실패는 둘이어야 한다**: `static_golden_resultflow_np`, `static_golden_resultflow_order_np`(golden 파일이 아직 없음). `KZ_GOLDEN_UPDATE=1` 로 그 둘만 쓴 뒤 scratchpad 의 것과 `cmp` 같아야 하고, 그 밖의 golden 은 바뀌지 않는다(`git status`).
- **시험**: 전체 ctest 261 → 264(새: `static_fixture_compile_resultflow`, `static_golden_resultflow_np`, `static_golden_resultflow_order_np`), golden 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`). 확인 수는 `ctest -V` 출력으로 `ctest_counts.log` 에: `test_static_cli` 1,058 → 1,099 [I]; `test_static_reasons` 첫 ctest 666, golden 뒤 692 [I](그 시험은 `tests/static/data/` 의 relations golden 마다 행의 `incomplete:` 줄을 센다, `tests/static/test_static_reasons.cpp:105-138`; 새 `resultflow_np.golden` 이 그 하나이고 `resultflow_order_np.golden` 은 order 글이라 세지 않음); `test_static_plan` 195, `test_core_schema` 34,017, `lint_selftest` 114, lint 규칙 여덟 clean. 단위 점검 `env/tools/tests/run_unit.sh` 15 → 16/16, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤와 다섯 빌드 실행 뒤 각각 `env/tools/ledger_check.sh`(빌드 전 104 행 실패 0: `ledger_check_pre.txt`)와 `env/tools/tools_check.sh --deep`(빌드 전 43 행 실패 0: `tools_check_pre.txt`), 출력은 단계마다 새 이름.
- **옵션 없는 경로(기본 경로)는 바이트 동일**(새 디렉터리, `compare.sh`):
  - relations, T8CF facts(`rerun.sh` → `rf_relcheck`): `relations.txt`·`keys.txt` 가 `sem_relcheck` 의 것과 바이트 동일, `relations.log` 빈 파일.
  - order, T8CF facts(`order.sh` → `rf_order`): 두 번 같은 바이트, `order.txt`·`edges.tsv` 가 `sem_order` 의 것과 바이트 동일, 로그 빈 파일.
  - 다시 뽑기 옵션 없음(`extract.sh off` → `rf_off_extract`), `sem_off_extract` 와: `basis.kz` 레코드 단위(`check_head.py --whole`: 도구 해시만), `facts.kz` 전체 dump(`check_whole.py`: 머리 해시 넷, STRINGS crc32, `extract.units_from` 만), facts 보기는 `extract.units_from` 한 줄만, `extract.txt` 1 줄, `relations.txt` 2 줄, `basis.txt`·`sha256.txt` 해시 줄만, `args.txt`·`relations.log` 같음.
  - 그 facts 위의 order·relations·키(`order.sh rf_off_extract rf_off_order`): 두 번 같음, `order.txt` 는 `sem_off_order` 와 2 번째 줄만, `edges.tsv` 같음, `relations.txt` = 다시 뽑기의 것, `keys.txt` 2 번째 줄만, 로그 빈 파일.
  - 계획(`plans.sh` → `rf_plans`; `plans.sh rf_off_extract rf_plans_off`): `compare_plans.py t8cf sem_plans/<b>`, `extract sem_plans_off/<b>` 의 허용만(생성기 정체성 줄, sidecar sha256 줄, module 표·실행 파일 목록 첫 줄, args.txt 의 출력 디렉터리; extract 쪽은 facts·basis 입력·artifact 줄과 그 경로).
- **옵션 있는 경로(다섯 빌드)**: 다시 뽑기(`extract.sh on` → `rf_on_extract`: 앞 단계의 옵션 있는 회차와 같은 `--object-access`, framework 자료 일곱·P0 선언 하나의 `--semantics`, 그리고 `--result-flow`), 앞 단계의 옵션 있는 회차 `sem_on_extract` 와(`compare.sh extract_on`):
  - `basis.kz` 레코드 단위(도구 해시만). `facts.kz` 머리(`check_head_rf.py --allow-size --new-sections 2`: 도구·artifact·basis 해시, 크기, `sections=` 가 둘 더함). **섹션은 레코드 글로**(`section_text_cmp.py`, 문자열 풀이 정렬된 정규 풀이라 바이트가 아닌 레코드 글로 비교함, 앞 단계 덧붙임 8.0b·8.0c 의 방식): 앞 회차의 모든 섹션이 같은 레코드 글, 다만 STRINGS 와 BASIS 는 다를 수 있고, FACT_EXPRS 는 앞 회차의 레코드 줄이 새 것의 첫 줄들이어야 한다(RESULT_FLOW 의 조건 식 행이 뒤에 더해짐); 새 섹션은 RESULT_FLOW 와 RESULT_EFFECT 둘. 새 회차의 섹션별 레코드 글 sha256 을 `section_text_on.txt` 에 남긴다(`section_text_sha_2.py`).
  - facts 보기(`compare_rf.py`): `[resultflow]`·`[resulteffect]` 절이 새로, `[basis]` 에 새 항목 넷(`extract.result_flow`, `count.result_flow`, `count.result_effect`, `count.result_flow_depth_cut`)과 `extract.units_from` 의 basis 해시, 그 밖은 같음.
  - `extract.txt`: 1 줄(facts 해시)과 새 줄 `result flow …`·`result effect …` 둘만. `relations.txt`(`compare_rel_rf.py`): 2 줄, 새 머리 두 줄, 행의 새 줄(`absence:`, `handling_label:`, `callee-check (result flow):`, `callee-check (result flow, success):`)만; 그 줄을 빼면 나머지는 같은 줄. `args.txt`: extract-only 줄만(끝에 ` --result-flow`). `basis.txt`·`sha256.txt` 해시 줄만. `relations.log` 빈 파일.
  - 그 facts 위의 order·relations·키(`order.sh rf_on_extract rf_on_order`): 두 번 같음; `order.txt`(`compare_ord_rf.py`): 2 줄, 새 머리 한 줄, `child without success:` 를 담은 줄은 그 앞이 같을 때(요약 줄, 간선의 notes, HB-lock 간선의 조건), `use the call of … (result tested|used)` 줄의 결과가 `unknown(the caller tests|uses the result; …)` 에서 다른 `unknown(…)` 이나 `caller-side(…)` 로, 새 `use caller-side:` 줄만; `edges.tsv`(`compare_ord_rf.py --tsv`): `child without success:` 뒤만 다른 조건 칸만; `relations.txt` = 다시 뽑기의 것, `keys.txt` 는 `sem_on_order` 와 2 번째 줄만, 로그 빈 파일. 새 HB-await 간선은 허용하지 않는다(아래 [I] 대로 다섯 빌드 모두 0 이어야 함; 나오면 덧붙임).
  - 계획(`plans.sh rf_on_extract rf_plans_on`): `compare_plans.py extract sem_plans_on/<b>` 의 허용만(계획 생성기는 새 섹션을 읽지 않음).
  - 이 회차는 중간 회차다: 압축 보존하지 않고 섹션별 레코드 글 sha256(`section_text_on.txt`)을 남긴다.
- **측정**(`compare.sh measure`, `measure_rf.py`; 모든 수는 `compare/` 의 남긴 파일에서): 섹션 행 수, 경로·표지별 행 수, 첫 경로 A21 행의 새 줄, child without success 의 갈래 수(앞 회차와 새 회차), 호출자 쪽 사용과 HB-await 간선 수, 오프라인 탐침의 D11(T?) 행 수 넷: (가) `s1_rev1.py` 를 tests_8c_fix facts 곁에 남은 `relations.txt`(그때의 relations 규칙이 쓴 것: 앞 집계의 입력)에, (나) 같은 facts 를 오늘의 relations 규칙으로 쓴 `rf_relcheck` 에, (다) 앞 단계의 옵션 있는 회차 `sem_on_order` 에, (라) `s1_rev2.py` 를 이 회차의 `rf_on_order` 에. 측정이고 판정이 아니다.
  - [I](scratchpad 점검 빌드, LLVM Release 구성, 감시 아래; 근거 아님, 지움; (가)~(다)는 남은 파일에 탐침을 돌려 본 값): T? 행 (가) 125/125/75/47/46(c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642; 앞 집계 `plan/ovm_rev1_20261116/S1_RETALLY.md` 의 D11 줄과 같음), (나) 69/69/33/47/46, (다) 69/69/32/46/45, (라) 47/46/18/44/43. (가)와 (나)의 차이는 그 뒤의 relations 규칙 개정(경로 A 행 127 → 63 등)에서 오고 이 단계의 것이 아니다. 이 단계의 효과는 (다) → (라)다.
  - [I] 표지(경로 A·B 행): c1_pre_tx H 14, J 14, R 9, T 7, T? 47, none 142; c1_post_tx H 15, J 14, R 9, T 7, T? 46; r2_core H 5, J 13, R 5, T 6, T? 20, none 131; c5_641 H 4, J 8, J? 16, R 14, T 3, T? 42, none 46; c5_642 H 4, J 7, J? 16, R 14, T 3, T? 41. RESULT_FLOW 약 12,900 행, RESULT_EFFECT 약 15,900 행(c1_pre_tx); c1 의 다시 뽑기 약 26 초.
  - [I] A21: `callee-check (result flow, success): OS_CountSemCreate: shown to reach the return: 0`(`osapi-countsem.c:103` 의 `OS_CountSemCreate_Impl` 의 0 이 `:106` 의 `OS_ObjectIdFinalizeNew` 를 지나 `:109` 의 return 으로). 그 행의 표지는 T?: lookup 실패에서 event-send `CFE_EVS_SendEvent` 형 3 [`CFE_EVS_EventType_ERROR`] `cf_cfdp.c:997` 은 complete, 그 결과의 return 은 complete, 위로 `cf_app.c:366` 의 exit-app 은 open(그 사이의 `CFE_ES_RunLoop` 이 같은 위치의 주소를 받아 매개변수 0 으로 저장할 수 있음). 그래서 event 보내기와 그 형이 행의 사실로 남는다.
  - [I] child without success(가능/찾지 못함/모름(위치 기반)): 앞 회차 c1 0/1/12(12), r2 0/2/9(9), c5 0/1/5(5) → 이 회차 c1 0/4/9(9), r2 0/5/6(6), c5 5/1/0(0). c5 의 가능 다섯은 OSAL 4.1.1 의 태스크 생성이 스레드를 만든 뒤 속성·detach 의 실패를 돌려주는 경로다 [I].
  - [I] 호출자 쪽 사용 0, 그 HB-await 간선 0(다섯 빌드; 앞 회차도 HB-await 0). 예산 출구가 return 이 아니고 루프를 벗어나 지역 플래그로 이어지는 대기(`CFE_ES_GetTaskFunction`, `CFE_ES_MainTaskSyncDelay`, `CFE_ES_ApplicationSyncDelay` 형)는 정해지지 않음으로 남는다(§5u 의 알려진 한계).
- **공간**: 각 빌드·사본·실행은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 빌드 필요량 = 앞 kz-static 빌드 dip 의 최대 256,942,080 + 새 출력 약 200 MB(다시 뽑기 둘 약 110 MB, order 넷과 둘째 실행 약 60 MB, relations 약 8 MB, 계획 셋 약 3 MB, 여유) + 한 시간 흐름 13 MB = 469,942,080 B. 사본은 필요량 100,000,000(도구 파일 하나를 읽어 xz 를 씀; 사본에서 푸는 바이너리는 없음). 실행 단계와 무거운 비교(extract, extract_on, measure)는 필요량 100,000,000. 점검 빌드(scratchpad)는 감시 아래에서 했고 지웠다.
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py`, `section_text_sha_2.py`(앞 단계의 것을 옮김), `check_head_rf.py`(앞 단계 `check_head_sem.py` 의 사본에 `--new-sections K`), `section_text_cmp.py`, `compare.sh`, `compare_rf.py`, `compare_rel_rf.py`, `compare_ord_rf.py`, `measure_rf.py`, `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`, `golden_scratch.sha256`; 빌드 전 점검 `tools_check_pre.txt`, `ledger_check_pre.txt`.

#### 9.0a 옵션 켠 회차 relations 비교의 덧붙임 (그 비교를 다시 돌리기 전에 적고 커밋함) [D][M]

`compare.sh extract_on`(`records/result-flow-20261116/compare_extract_on.txt`, rc 1)에서 c1_pre_tx·c1_post_tx·r2_core 의 `relations.txt` 비교가 선언 §9.0 의 허용 밖으로 나왔다(`compare/on_relations_<b>.txt`: 다른 줄 23/23/12; c5 두 빌드는 허용 안). 나머지 47 줄은 OK 다(basis, facts 머리, 섹션의 레코드 글, facts 보기, extract.txt, args.txt, basis.txt, sha256.txt, relations.log). 선언 밖의 차이이므로 여기서 멈추고(2026-11-18 검토 뒤 정정: 기록은 멈추지 않았음을 보인다. 측정 단계가 이 비교의 rc 1 바로 뒤, 이 덧붙임의 커밋 전에 돌았다: §9.7 의 정정, §12.7 `builds-01`) 까닭을 적는다.

- **무엇이 다른가**: 다른 줄은 모두 행의 `layers:` 줄이고, ` sem-unverified(` 앞의 글은 같고, 새 줄의 `sem-unverified(…)` 항목 집합이 앞 줄의 것을 모두 담고 더 많다(예: c1_pre_tx A30 `sem-unverified(OS_FileSysAddFixedMap:agreement, OS_mount:agreement)` → `sem-unverified(OS_DirectoryOpen:agreement, OS_FileSysAddFixedMap:agreement, OS_TranslatePath:agreement, OS_mount:agreement)`). 다른 종류의 줄은 없다(세 빌드의 다른 줄 58 모두 이 꼴; 이 덧붙임을 쓰기 전에 scratchpad 에서 줄마다 확인함).
- **까닭**: layers 칸의 `sem-unverified(…)` 는 "그 행이 이름 대는 함수"의 검증 안 된 framework 항목이다(`kairos/lib/llvm/relations.cpp:513-519` `row_sem_unverified`, 이름 댄 함수는 행을 쓰는 동안 함수 이름을 쓸 때마다 모임: `:143` `fn_notes_ = &row_fns_`). revision 10 의 새 줄(`absence:` 의 효과 사슬, `callee-check (result flow):`)이 함수를 더 이름 대므로(A30 의 `absence: -17 (no match: CF_CFDP_PlaybackDir_Initiate@… -> OS_DirectoryOpen -> OS_TranslatePath -> …)`) 그 함수들의 항목이 그 집합에 들어간다. 이는 앞 단계의 규칙(§8.0 4: 행이 이름 대는 함수의 항목) 그대로의 결과이고, 선언 §9.0 이 "그 줄을 빼면 나머지는 같은 줄"이라고 적어 이 귀결을 빠뜨렸다. 새 줄을 지운 비교 쪽의 결함이 아니라 선언의 허용이 좁았다.
- **고친 허용**: `layers:` 줄은 ` sem-unverified(` 앞이 같고 새 집합이 앞 집합을 모두 담을 때(더 많을 때) 허용한다. 그 밖은 §9.0 그대로. 머리의 `semantics of these facts … rows that name a function with framework semantics items N, of them with items not verified M` 줄은 허용하지 않는다(이번 비교에서 같았다).
- **다시 돌리는 것**: `compare.sh extract_on_rel`(새 단계; 출력 `compare_extract_on_rel.txt`, `compare/on_relations_2_<b>.txt`), 스크립트 `compare_rel_rf_2.py`(`compare_rel_rf.py` 의 사본에 위 허용을 더함; 앞 스크립트와 출력은 그대로 남김). 이 덧붙임을 커밋하기 전에 사본 스크립트를 세 빌드(c1_pre_tx, r2_core, c5_641)에 한 번 돌려 동작을 확인했고 그 출력은 남기지 않았다(§9.7).

### 9.1 바뀐 것 [F]

- 커밋 차례: `9334e97`(`model.hpp`·`spec.hpp`; 선언 전의 스냅숏), `d22ec58`(나머지 코드, fixture, 시험, 문서, 탐침 개정 2, 단위 점검 16; 선언 전의 스냅숏), `7e11c2a`(선언 §9.0, 비교·측정 스크립트, 빌드 전 목록; 빌드 전), `5a07987`(사본과 빌드 도중의 기록; 스냅숏), `7dcdc70`(빌드 기록, 새 golden 둘, 등록부 행; 다섯 빌드 전), `b2110c6`(덧붙임 9.0a 와 그때까지의 실행·비교 기록; 다시 비교하기 전), 그리고 이 절의 커밋.
- 규모(`d7415f2` 에서 `b2110c6` 까지, `kairos/`, `env/tools/`, 탐침): 27 파일, 3,555 줄 더함, 10 줄 지움. `ast_fn.cpp` +1,075(도달 정의로 결과의 사용, guard 행, 값마다의 CFG 걷기와 효과), 새 `result_flow.{hpp,cpp}` 155 줄(`result flow rules revision 1 (2026-11-16)`, 첫 회차의 반환 값), `serialize.cpp` +188(효과의 분류, 두 섹션과 BASIS 항목 넷), `relations.cpp` +282(revision 10: 표지, 효과 글, 위로의 합성), `order.cpp` +147(revision 3: 안쪽 생성 호출의 효과, 호출자 쪽 사용), `facts_text.cpp` +45(절 둘), `model.hpp` +64, `cmd_extract.cpp`·`main.cpp`(옵션), 시험 `test_static_cli.cpp` +58, fixture `resultflow.c` 249 줄, golden `resultflow_np.golden` 495 줄·`resultflow_order_np.golden` 241 줄, 탐침 `s1_rev2.py` 446 줄, `run_unit.sh` +22, 문서 `static_facts.md` §5u, `schema.md` 날짜 줄.
- 바꾸지 않은 것: `include/`, `cmake/`, 최상위 CMakeLists, `runtime/`, `instrument/`, `tests/lint/`, `semantics/`, 계획 생성기 규칙, 판정 규칙·어휘, 설명 스크립트, `one_build.sh`, `s1_rev1.py`. relations 의 `rules` 줄(revision 9)과 order 의 첫 줄(revision 2)은 두 섹션 없는 facts 에서 앞판 그대로다.

### 9.2 빌드 [M]

MC 에서 제자리, 목표별, 각각 공간 감시 아래(필요량 469,942,080), 모두 첫 시도에 rc 0. 사본 20:42:04Z, 빌드 시작 20:46:09Z(`build_start.txt`), 빌드 전 HEAD `7e11c2a`(`pre_mc.txt`), kairos 아래 커밋 안 된 줄 0(`post_mc.txt`).

| 목표 | 때 (UTC) | dip (B) |
|---|---|---|
| `kz-static` | 20:46:15–20:51:32 | 168,206,336 |
| `all` | 20:51:43–20:55:47 | 102,625,280 |

- 다시 링크된다고 적은 14 가 모두 바뀌었다(`relink_result.txt`). `not_relink.sha256` 110 모두 같은 바이트(kz_lint `3d4d5a8f…` 포함).
- 도구(`post_mc.txt`, `identity_kz-static.txt`): kz-static `12a3970f…` → `499748bc22881d9019e7dc31b9cc3a7aeb0740e16b5ad94207cc8e52503a163a`; `--identity` 의 `tool_sha256` `cccffd97…` → `221231bbe926e3f10732f5cbf8f9083b45e19d2f2b2bcb396026b037ead1b03b`, `tool_source` `e4ed3acf…` → `ad79bdaaa1e175d9cb8a55dc336c82ee000f200113b702a4ac7d3388d40de2b2`; `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 그대로. `CMakeCache.txt` `bc2f43b4…` 그대로.
- 도구 보존: 빌드 전 kz-static `12a3970f` 를 `/home/user/work/preserve/tools-20261116-j/kz-static.12a3970f.xz`(44,334,596 B)로 사본, 왕복 확인(`tool_copy/ROUNDTRIP.txt`: xz -t OK, 푼 sha256 같음, cmp 같음). 등록부: 사본 직후 in-place+copy(`tools_check_after_copy.txt` 43 행 실패 0), 빌드 뒤 copy, 새 `499748bc` in-place(`tools_check_after_build.txt` 44 행 실패 0).

### 9.3 시험 [M]

- 첫 ctest(`ctest_first.log`): 264 개 가운데 실패 2, 선언한 그 둘(`static_golden_resultflow_np`, `static_golden_resultflow_order_np`, golden 없음). 건너뜀 1.
- `KZ_GOLDEN_UPDATE=1` 로 그 둘만 썼다(`golden_update.log`). scratchpad 점검 빌드의 것과 `cmp` 같다(`041b4ab2…`, `a03cc621…`). 다른 golden 은 바뀌지 않았다(`git status` 에 새 파일 둘뿐).
- 갱신 뒤 전체 ctest(`ctest_full.log`): 264 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`).
- 확인 수(`ctest_counts.log`, golden 뒤): `test_static_cli` 1,099, `test_static_reasons` 692, `test_static_plan` 195, `test_core_schema` 34,017, `lint_selftest` 114 경우 실패 0, lint 시험 여덟 통과. 선언의 "첫 ctest 의 `test_static_reasons` 666" 은 첫 ctest 를 `-V` 없이 돌려 세지 않았다(§9.7).
- 단위 점검 `run_unit.sh` 16/16(`unit_tools.txt`), `step2a_tests_unit.sh` 7/7(`unit_step2a.txt`).
- **2026-11-18 검토 뒤 정정: §12.7 `builds-07`.** 선언 §9.0(과 §8.0a, §10.0)의 “lint 규칙 여덟 clean”은 lint 시험 여덟(규칙 일곱과 selftest)을 뜻한다(§8.3 이 그렇게 적음). 남긴 출력의 `lint <rule>: clean` 줄은 일곱이다.

### 9.4 바이트 동일성 [M]

| 비교 | 결과 |
|---|---|
| relations, T8CF facts (`compare_relations.txt`) | 15 OK: `relations.txt`·`keys.txt` 가 `sem_relcheck` 와 바이트 동일, 로그 빈 파일 |
| order, T8CF facts (`compare_order.txt`) | 25 OK: 두 번 같음, `sem_order` 와 바이트 동일 |
| 다시 뽑기 옵션 없음 vs `sem_off_extract` (`compare_extract.txt`) | 45 OK: `basis.kz` 허용 2(도구 해시), `facts.kz` 전체 dump 허용 6(c1_pre_tx 162,142 줄), facts 보기 `extract.units_from` 한 줄만(149,111 줄), 나머지 선언대로 |
| 그 facts 위 (`compare_onextract.txt`) | 40 OK |
| 계획 (`compare_plans.txt`) | 열 모두 "only declared differences"(T8CF 위 다섯, 옵션 없는 다시 뽑기 위 다섯) |
| 다시 뽑기 옵션 있음 vs `sem_on_extract` (`compare_extract_on.txt`) | 47 OK, BAD 3: c1 둘과 r2 의 `relations.txt`(선언 밖, §9.0a) |
| 같은 것의 relations, 9.0a 의 허용으로 (`compare_extract_on_rel.txt`) | 5 OK: layers 줄의 커진 `sem-unverified` 집합 23/23/12/0/0 밖의 다른 줄 0 |
| 그 facts 위 (`compare_onextract_on.txt`) | 40 OK |
| 계획 (`compare_plans_on.txt`) | 다섯 "only declared differences" |

- **옵션 없는 경로는 앞판과 같다**(위 첫 다섯 줄). 다른 것은 도구 정체성에서 오는 값과 계획의 생성기 정체성 줄뿐이다.
- **옵션 있는 경로**: 섹션은 빌드마다 앞 회차의 43 가운데 42 가 같은 레코드 글(FACT_EXPRS 는 앞 회차의 줄이 앞에 그대로, 뒤에 c1_pre_tx 7,604 줄·c5_642 3,715 줄 등이 더해짐), 다른 것 BASIS, 새 것 RESULT_FLOW·RESULT_EFFECT(`compare/on_sections_<b>.txt`; 새 회차의 섹션별 레코드 글 sha256 `section_text_on.txt`). facts 보기는 `[resultflow]`·`[resulteffect]` 와 BASIS 새 항목 넷뿐(`compare/on_<b>.txt`). relations 는 2 번째 줄, 머리 두 줄, 행의 새 줄(c1_pre_tx: `absence:` 63, `handling_label:` 233, `callee-check (result flow):` 55, `… success):` 5), layers 줄의 커진 집합뿐(`compare/on_relations_2_<b>.txt`). order 는 2 번째 줄, 머리 한 줄, child without success 를 담은 줄 53/53/30/84/186, `use the call of` 의 결과 4/6/4/0/2(모두 `unknown(…)` 에서 다른 까닭의 `unknown(…)`), 호출자 쪽 사용 0; `edges.tsv` 는 c5 의 HB-lock 조건 18/48 줄만(`compare/on_order_<b>.txt`, `compare/on_edges_<b>.txt`).
- **누적 목록과 등록부**: 빌드 전 `ledger_check_pre.txt` 104 행·`tools_check_pre.txt` 43 행 실패 0; 빌드 뒤 `ledger_check_after_build.txt` 104 행, `tools_check_after_build.txt` 44 행 실패 0(deep); 장부 행 열을 더한 뒤 `ledger_check_end.txt` 114 행 실패 0(새 행 `OBS_rf_relcheck`, `OBS_rf_order`, `OBS_rf_off_extract`, `OBS_rf_off_order`, `OBS_rf_on_extract`, `OBS_rf_on_order`, `OBS_rf_plans`, `OBS_rf_plans_off`, `OBS_rf_plans_on`, `TOOLS_20261116j`), `tools_check_end.txt` 44 행 실패 0.
- 바깥 이름 점검(`--tree`)은 아홉 출력 디렉터리, 사본 디렉터리, 기록 디렉터리에서 깨끗하다(`name_check_tree.txt`, 11 모두 rc 0).
- **2026-11-18 검토 뒤 정정: §12.7 `builds-02`.** “43 가운데 42 가 같은 레코드 글, 다른 것 BASIS”는 틀렸다. STRINGS 는 레코드 줄이 없어(`lines=0`) 글로는 같다고 세어졌지만 count 가 다르다(c1_pre_tx: 의미 회차 1,609,737 → 이 회차 3,740,581). 빌드마다 글과 count 가 같은 섹션은 43 가운데 40 이고 다른 것은 STRINGS(풀), BASIS, FACT_EXPRS(뒤에 더해진 행; c5 는 같음)다. 선언 §9.0 은 앞 회차와의 비교에서 STRINGS 의 차이를 허용했으므로 선언 위반은 아니다. 다시 견준 출력: `records/review-fixes-2-20261118/recheck/recheck_rounds.txt`.

### 9.5 측정 [M]

옵션 있는 다시 뽑기(`rf_on_extract`)와 그 위의 order·relations(`rf_on_order`), kz-static `499748bc`. 수의 출처: `compare/measure_<b>.txt`(`measure_rf.py`)와 탐침 출력 `compare/s1_*_<b>.out`·`.tsv`. 측정이고 판정이 아니다. 모든 값이 선언 §9.0 의 scratchpad [I] 와 같다.

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| RESULT_FLOW 행 (guard 행) | 12,916 (3,924) | 12,919 (3,925) | 9,622 (2,906) | 5,568 (1,887) | 5,561 (1,879) |
| RESULT_EFFECT 행 (open) | 15,930 (8,549) | 15,933 (8,554) | 11,198 (5,543) | 6,864 (4,660) | 6,867 (4,667) |
| 그 가운데 event-send / exit-app / terminal | 1,073 / 305 / 159 | 1,067 / 305 / 159 | 307 / 86 / 158 | 589 / 0 / 169 | 589 / 0 / 165 |
| 표지 (경로 A·B 행) | H 14, J 14, R 9, T 7, T? 47, none 142 | H 15, J 14, R 9, T 7, T? 46, none 142 | H 5, J 13, R 5, T 6, T? 20, none 131 | H 4, J 8, J? 16, R 14, T 3, T? 42, none 46 | H 4, J 7, J? 16, R 14, T 3, T? 41, none 46 |
| T? 행 (D11): (가) 앞 집계의 입력 | 125 | 125 | 75 | 47 | 46 |
| (나) 같은 facts, 오늘의 relations 규칙 | 69 | 69 | 33 | 47 | 46 |
| (다) 앞 단계의 옵션 있는 회차 | 69 | 69 | 32 | 46 | 45 |
| (라) 이 회차 (탐침 개정 2) | **47** | **46** | **18** | **44** | **43** |
| child without success: 가능/찾지 못함/모름(위치 기반), 앞 회차 → 이 회차 | 0/1/12(12) → 0/4/9(9) | 0/1/12(12) → 0/4/9(9) | 0/2/9(9) → 0/5/6(6) | 0/1/5(5) → 5/1/0(0) | 0/1/5(5) → 5/1/0(0) |
| 효과로 읽은 안쪽 생성 호출 | 34 | 34 | 23 | 8 | 8 |
| 호출자 쪽 사용 / 그 HB-await 간선 / 모든 HB-await 간선 | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 |

- **T?**: 앞 집계의 125/125/75/47/46 은 tests_8c_fix facts 곁에 남은 relations(그때의 규칙)에서 센 것이고, 같은 facts 를 오늘의 규칙으로 쓰면 69/69/33/47/46 이다((가)→(나)는 그 뒤의 relations 규칙 개정에서 옴). 이 단계의 효과는 (다) → (라): 69 → 47, 69 → 46, 32 → 18, 46 → 44, 45 → 43. 사실에서 온 표지가 있는 행 91/91/49/87/85(경로 A·B 행 233/233/180/133/131 가운데, 나머지는 `handling_label: none`).
- **첫 관측 경로 A21**(C1 관계 1, 키 `17975444d83f6589`, c1 두 빌드): `callee-check (result flow, success): OS_CountSemCreate: shown to reach the return: 0`. 성공 값 0 은 `osapi-countsem.c:103` 의 `OS_CountSemCreate_Impl` 결과가 `:106` 의 `OS_ObjectIdFinalizeNew`(매개변수 0 을 바꾸지 않고 돌려줌)를 지나 `:109` 의 return 에 닿는다. 같은 길로 -20 `OS_INVALID_SEM_VALUE`, -6 `OS_SEM_FAILURE` 가, `:94` 의 `OS_ObjectIdAllocateNew` 결과로 -35, -16, -15, -1 이(-33, -28, -14 는 present-only), -13 은 open 이다. `callee-check (set)` 은 여전히 완전성을 말한다(앞판 줄 그대로).
  - c1_pre_tx 의 표지 **T?**: 부재 값 -17 은 `cf_cfdp.c:993:19` 에서 event-send `CFE_EVS_SendEvent` 인자 1(event-type) 3 [`CFE_EVS_EventType_ERROR`] 에 닿고(complete), 위로 `cf_app.c:366:5` 의 exit-app 은 open(`cf_app.c:338:12` 의 `CFE_ES_RunLoop` 이 `CF_AppData_t.run_status` 의 주소를 받아 매개변수 0 으로 저장함). event 보내기와 그 형은 행의 사실로 남았다.
  - c1_post_tx 의 표지 **H**: 같은 lookup(`cf_cfdp.c:1003:23`)의 -17 은 재시도에 닿는다(지연 `OS_TaskDelay` 1011:17, 다시 부르는 호출 1003:23). 고친 판의 처리가 사실로 갈린다.
- **child without success**: 위치 기반 모름이 c1 12 → 9, r2 9 → 6, c5 5 → 0 으로 줄었다. c5 두 빌드의 가능 다섯은 OSAL 4.1.1 의 `OS_TaskCreate` 가 `pthread_create`(`osal/src/os/posix/osapi.c:645:19`)의 성공 값 0 뒤에 -1 `OS_ERROR` 를 돌려주는 return(`:672`, `:684`; 그 값이 정하지 않는 분기를 지나는 경로)과, 그것을 부르는 `CFE_ES_CreateChildTask` 가 안쪽 호출의 실패 뒤 상수를 돌려주는 return(`cfe/fsw/cfe-core/src/es/cfe_es_api.c:1149`)이다(`rf_on_order/c5_641/order.txt` 의 `child without success: possible:` 줄; HB-lock 조건 18/48 줄이 그 표지를 담음).
- **HB-await**: 다섯 빌드 모두 호출자 쪽 사용이 0 이라 새 간선이 없다. `use the call of` 의 결과는 c1 4(c1_post_tx 6), r2 4, c5_642 2 가 여전히 `unknown(…)` 이고, 까닭이 `unknown(a budget exit leaves the loop and goes on: the value the function returns after it is not followed)` 로 바뀌었다(예: `CFE_ES_GetTaskFunction` 을 부르는 `cfe_es_apps.c:552:9`, `CFE_ES_MainTaskSyncDelay` 를 부르는 `cfe_es_start.c:208:9`·`:223:9`). 선언 [I] 그대로다.
- **2026-11-18 검토 뒤 정정: §12.7 `rules-04`, `rules-05`, `rules-01`.** (1) exit-app 이 c5 두 빌드에서 0 인 까닭: 6.4.x 의 의미 자료는 `run-status` 역할을 `CFE_ES_ExitApp` 이 아니라 `CFE_ES_RunLoop` 의 0 번 매개변수(포인터)에 준다(`framework_semantics_cfe-6.4.1.txt:139`; ExitApp 의 `\param` 은 상수를 이름 대지 않음, `cfe_es.h:506`). 엔진은 그 역할만으로 효과를 `exit-app` 이라 불렀다(`serialize.cpp` 옛 `:1296-1297`). c5 의 종료는 `terminal` 로 세어졌고, RunLoop 호출이 exit-app 으로 적힌 행은 0 이라 틀린 효과 행은 없었다. (2) outcome 등급(success·failure·other)은 엔진의 수 범위로 정했고 그 범위 가운데 `0x40000000..0x7FFFFFFF` = other 와 “그 밖은 success” 에는 적힌 근거가 없었다; 작은 양수는 모두 success 였다(c5_641: `CFE_ES_RunLoop … success 1`). (3) 47 의 T? 행에 든 c1_pre_tx 의 A21 은 계획 생성기의 층 차례에서는 글 label R 로 층 1 이다(§11.5 의 정정). (1)(2)는 코드로 고쳤고 MC 도구는 앞판이다(§12.6).

### 9.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 20:42:04 | 2,303,225,856 | 사본 시작(dip 44,392,448) |
| 20:46:15 | 2,258,624,512 | `make kz-static`(가장 낮은 2,090,418,176, dip 168,206,336), `all`(102,625,280) |
| 21:00:04–21:06:01 | 2,110,902,272 → 1,922,506,752 | relations(dip 7,680,000), order(14,229,504), 다시 뽑기 옵션 없음(47,157,248), 그 order(21,925,888), 계획 둘(987,136, 995,328), 다시 뽑기 옵션 있음(72,048,640), 그 order(22,380,544), 계획(991,232) |
| 21:06:25–21:07:03 | 1,943,719,936 → 1,941,938,176 | 무거운 비교 셋(facts 보기·dump 는 scratchpad, 끝에 지움; 가장 낮은 1,910,489,088, `extract_on` 의 dip 33,177,600) |
| 이 절을 쓸 때 | 1,940,914,176 | 끝의 장부·등록부·이름 점검 뒤 |

- 하한(314,572,800)을 넘지 않았다. 빌드 dip 은 필요량 안이었다. 빌드는 목적 파일로 148,480,000 B 쯤 여유를 줄였다(kz-static 의 change 58,859,520 + all 의 89,620,480).
- 남긴 출력(장부 행): `rf_relcheck`, `rf_order`, `rf_off_extract`, `rf_off_order`, `rf_on_extract`, `rf_on_order`, `rf_plans`, `rf_plans_off`, `rf_plans_on` 합 166,046,889 B; 사본 44,334,596 B. 지운 것: scratchpad 의 점검 빌드와 그 출력(선언 전), 비교의 facts 보기와 dump(scratchpad), order 의 둘째 실행 디렉터리(첫 실행과 같은 바이트임을 비교한 뒤).

### 9.7 계획과 다르게 한 것

- **guard 행의 자리**: 호출의 guard 를 따로 섹션에 두지 않고 RESULT_FLOW 의 def none·use none 행으로 썼다(`include/` 를 바꾸지 않기 위해; 선언 §9.0, `schema.md` 날짜 줄).
- **효과의 위로 합성은 facts 가 아니라 relations 가 한다**: RESULT_EFFECT 는 호출 지점의 몸 안 효과만 적고, 호출자로 돌아간 실패 값의 효과는 relations 가 읽을 때 따라간다(행 수가 호출자 수에 곱해지지 않게; §5u).
- **선언 전의 스냅숏 커밋 둘**(`9334e97`, `d22ec58`)과 빌드 도중의 스냅숏 하나(`5a07987`)가 있다. 빌드·실행·비교는 모두 선언 커밋 `7e11c2a` 뒤다.
- **relations 비교의 허용 하나**(9.0a): layers 칸의 `sem-unverified` 집합이 새 줄이 이름 대는 함수만큼 커지는 것을 선언이 빠뜨렸다. 사본 스크립트로 다시 비교했다. 그 사본 스크립트를 덧붙임 커밋 전에 세 빌드에 한 번 돌려 동작을 확인했고(출력 남기지 않음), 남긴 비교는 커밋 뒤의 것이다.
- **첫 ctest 의 `test_static_reasons` 확인 수**(선언 666)는 첫 ctest 를 `-V` 없이 돌려 세지 않았다. 시험은 통과했고, golden 뒤 수 692 는 `ctest_counts.log` 에 있다.
- **T? 의 기준**: 계획의 "지금 125/125/75/47/46" 은 그때의 relations 규칙이 쓴 글에서 센 값이라, 이 단계의 효과는 같은 회차 꼴의 앞 단계 값 (다)와 견주었다(네 수를 모두 남김).
- **가벼운 비교 여섯**(relations, order, onextract, plans, onextract_on, plans_on)과 `extract_on_rel` 은 공간 감시 없이 돌렸다. 기록 디렉터리에 글 파일만 쓰고 그때 여유는 1,940,000,000 B 이상이었다. 무거운 비교 셋은 감시 아래 돌렸다(`df_compare_*.log`).
- **2026-11-18 검토 뒤 정정: §12.7 `builds-01`, `builds-03`.** 규칙 어긋남 둘을 적는다. (1) **측정이 덧붙임보다 먼저 돌았다.** `compare.sh extract_on` 이 rc 1 로 끝난 바로 뒤(`df_compare_extract_on.log` 의 끝 2026-11-16T21:07:01.937Z)에 측정 단계가 시작되었다(`df_compare_measure.log` 의 시작 21:07:01.947Z). 덧붙임 커밋 `b2110c6` 은 21:08:53Z 이고 그 커밋에 측정 출력(`compare/measure_*.txt`, `s1_*`)이 들어 있다. 선언 §9.0 은 선언 밖의 차이에서 멈춘다고 적었고 §9.0a 는 “여기서 멈추고”라 적었는데, 기록은 멈추지 않았음을 보인다: 그 글은 틀렸다. 측정은 덧붙임의 허용을 쓰지 않는다. (2) **고친 비교 스크립트를 커밋 전에 돌렸다**(세 빌드, 출력을 남기지 않음; §9.0a 와 이 절이 이미 적음). 남긴 커밋 뒤의 다시 비교(`compare_extract_on_rel.txt`, 5 OK)는 lead 의 판단(2026-11-18)대로 그대로 둔다; 다시 빌드하지 않는다. (3) **scratch 점검 빌드의 감시 기록이 없다.** §9.0 의 공간 줄과 `commands.txt:5-6` 은 감시 아래에서 했다고 적지만 `records/result-flow-20261116/` 에는 `df_scratch_*.log` 도 `scratch_guard/` 도 없다: 그 말은 남긴 출력으로 뒷받침되지 않는다.

### 9.8 열린 것

- **event type·run status 의 등급**: 효과는 event 보내기와 그 형 상수(예: `CFE_EVS_EventType_ERROR` 3)를 적지만, 그 형이 실패를 알리는 등급인지는 자료에 없어 event 보내기만인 효과는 T? 로 둔다. 계획 생성기가 event 보내기와 그 형을 계획에 올리는 것은 뒤 단계다.
- **예산 출구가 return 이 아닌 대기**: 루프를 벗어나 지역 플래그로 이어지는 대기(`CFE_ES_GetTaskFunction`, `CFE_ES_MainTaskSyncDelay`, `CFE_ES_ApplicationSyncDelay` 형)는 호출자 쪽 사용을 정하지 못해 HB-await 가 0 이다. 루프 출구의 지역 플래그 값을 따라가는 것(루프 조건의 정의 섹션 LOOP_DEFS)이 뒤 단계다.
- **open 효과의 큰 몫**: RESULT_EFFECT 의 open 이 c1 8,549/15,930 이다(그 값이 정하지 않는 분기, 따라가지 않은 return, 주소를 받는 몸 없는 호출). A21 의 exit-app 도 `CFE_ES_RunLoop` 의 매개변수 저장 때문에 open 이다. 그 함수의 저장이 run status 를 바꾸는지는 자료의 의미 항목(run-status 인자)과 저장 값을 잇는 규칙이 있어야 정해진다.
- **c5 의 child without success 가능 다섯**: 스레드를 만든 뒤의 실패가 실제로 일어나는 조건(속성·detach 의 실패)을 판정 규칙이 어떻게 다룰지 정해야 한다 [미확인].
- **J? 16**(c5): 부재 값이 정해지지 않은 버려지는 호출이다. 부재 값의 평가가 뒤 단계다.
- 이 회차는 중간 회차다: 섹션별 레코드 글 sha256(`section_text_on.txt`)을 남겼고, 압축 보존하지 않았다.
- **2026-11-18 검토 뒤 정정: §12.7 `rules-02`.** **기초 4 는 A21 에서 여전히 서지 않는다.** 마지막 회차의 A21 행에는 `callee-check (returns not followed): OS_CountSemCreate: OS_NotifyEvent returns the result of <indirect> …; OS_ObjectIdFinalizeNew returns param [operation_status]` 와 `callee-check (set): OS_CountSemCreate: not established (2 returns not followed, 14 constants not shown to reach the return)` 가 그대로 있다(`ll_final_extract/c1_pre_tx/c1_pre_tx/NP/relations.txt:635-636`). 판정 규칙 8.12 (ㄱ)은 두 줄이 모두 없을 때만 기초 4 를 세우고, 설명 스크립트(`env/obs1/explain.py:336-354`)는 `callee-check: <callee> returns`·`(returns not followed)`·`(set)` 줄만 읽는다: 새 줄 `callee-check (result flow, success)`(:648)를 읽는 쪽은 없다. 그래서 A21 의 기초 4 는 `record_insufficient(no success constant of the creator: the callee's returns are not all followed)` 이다. 남은 근거는 `OS_ObjectIdAllocateNew` 를 거쳐 올 수 있는 `OS_NotifyEvent` 의 간접 호출 결과(`osapi-countsem.c:94`)와 개정 9 의 줄들이다(:635 는 :647 이 따라간 FinalizeNew 의 매개변수를 여전히 “not followed” 로 적는다: 개정 9 의 줄은 RETURNS 와 RET_CONSTS 만 읽고 RESULT_EFFECT 를 읽지 않기 때문이며, 그대로 둔다; `static_facts.md` §5u 의 덧붙임). result flow 줄이 어떤 완전성 조건에서 기초 4 를 세울 수 있는지는 판정 규칙의 덧붙임이 정할 일이고(이름 결합을 구현하는 설명 판보다 먼저 커밋), 이 묶음에서는 판정 규칙 파일을 고치지 않았다(lead 의 판단).

## 10. 루프 플래그 술어와 핸들 단위 잠금 정체성, 마지막 옵션 회차: 추출 옵션 `--loop-defs`·`--lock-handles`, relations rules revision 11, order rules revision 4 (kz-static 빌드 하나)

대기 루프의 조건이 지역 플래그(호출 결과나 객체 읽기를 담은 지역, 몸 안에서 상수로 바뀌는 지역)를 읽을 때 그 정의로 술어를 세우고(LOOP_DEFS), 핸들로 잡는 잠금 연산마다 핸들·그 필드의 writer·의미 항목을 적는다(LOCK_HANDLES). relations 는 LOCK_HANDLES 가 있을 때만 경로 C 의 잠금을 핸들로 같은 mutex 로 읽고, order 는 두 섹션이 있을 때만 LOOP_DEFS 의 술어로 대기를 읽고 flag 를 통해 호출자 쪽 예산 값을 구한다. 호출자에 기대는 spawn 규칙의 덜 잡음을 따로 잰다(§6.10 의 4). 그리고 모든 추출 옵션을 켠 마지막 회차로 다섯 빌드를 다시 뽑아 중간 회차들과 섹션별로 견주고, 그 facts 를 뒤 단계의 입력으로 장부에 적는다. 기록: `../records/loop-lock-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `ll_*`. 도구 사본: 새 디렉터리 `/home/user/work/preserve/tools-20261116-k/`. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다.

### 10.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다.

- **무엇을 하는가** (`kairos/docs/static_facts.md` §5v):
  1. **옵션** `kz-static extract --no-profile --loop-defs`, `--lock-handles`(프로필 없는 방식에서만; 프로필 방식에서는 종료 코드 2 와 `kz-static extract: --loop-defs needs the profile-free mode (--no-profile)`, `--lock-handles` 도 같음). 옵션이 없으면 추출의 코드 경로와 출력은 앞판 그대로다(모든 새 읽기가 `spec.loop_defs`·`spec.lock_handles` 아래에 있음).
  2. **LOOP_DEFS**(섹션 140, `loop flag predicate rules revision 1 (2026-11-16)`): poll·count-bounded 루프 가운데 FACT_LOOPS 에 술어가 없고 조건이 지역을 읽는 것마다, 예산 지역(FACT_LOOPS 의 예산과 count-bounded 루프의 귀납 변수)을 뺀 조건 출구의 지역을 flag 규칙(루프 앞 상수 하나, 몸 안 guard 아래 다른 상수 하나, 출구가 루프 조건이거나 몸 바로 아래 조건 하나로 정해짐: `f == 몸 안 상수` → 그 guard)과 value 규칙(몸 안 정의 하나가 호출 결과나 객체 읽기이고 모든 사용이 같은 반복의 그 정의 뒤, 같은 `if` 문 안)으로 풀어 술어를 만든다. 정의마다 한 행과 루프 자신의 행(`local` 빈칸, `guard_root` = 술어, `pred` = 형태). FACT_LOOPS·AWAIT_L1 은 바뀌지 않는다.
  3. **LOCK_HANDLES**(섹션 144, `lock handle rules revision 1 (2026-11-16)`): LOCK_OPS 의 acquire·release 가운데 정체성 글에 `?(`(호출 결과 뒤로 사라짐)가 있는 것마다 한 행: 사슬에서 핸들을 넘기는 호출(SEMANTICS 의 인자 역할 `lock-object` 가 있는 callee 의 인자, 없으면 핸들 값 하나를 넘기는 가장 깊은 호출; 매개변수이면 사슬을 거슬러), 그 필드(전역 객체의 필드일 때), 그 필드의 하나뿐인 out-pointer writer, 의미 항목, 상태(`static-instance` = per-field 완전 + 한 번 도는 writer 하나 + 검증된 `lock-object` 항목; `semantics-unverified`, `writer-set-incomplete`, `not-static`), 전제(“핸들이 가리키는 객체가 두 연산 사이에 지워지고 다시 만들어지지 않는다”).
  4. **relations rules revision 11**(LOCK_HANDLES 가 있을 때만): 머리 한 줄 `rules with lock handles: relations rules revision 11 (2026-11-16), over LOCK_HANDLES …`. 경로 C 에서 static instance 가 아닌 잠금이 spawn 의 획득들과 읽기의 획득들에서 모두 같은 `static-instance` 핸들을 넘기면 static instance 처럼 읽고(그 밖의 proven-impossible 조건은 그대로), 같은 핸들이 다른 상태이면 undecidable 의 글에 적는다. `rules` 줄은 revision 9 그대로.
  5. **order rules revision 4**(LOOP_DEFS·LOCK_HANDLES 가 있을 때만): 머리 한 줄 `rules with loop definitions and lock handles: order rules revision 4 (2026-11-16), …`. FACT_LOOPS 에 술어가 없는 대기는 LOOP_DEFS 의 술어로 읽고(`predicate: through local definitions (LOOP_DEFS, form …)`), 그렇지 않은 대기의 predicate 줄 끝에 `; LOOP_DEFS <상태>`. 예산 출구가 루프를 벗어나 이어지고 술어가 flag 이면 함수의 루프 뒤 return 의 상수(flag 의 몸 안 정의로 정의된 대안은 뺌)를 호출자 쪽 예산 값으로 쓴다; flag 가 없으면 그 까닭 글에 `(no flag predicate in LOOP_DEFS)`. HB-lock 간선에 `lock identity source:` 줄. HB-await 의 hard 조건은 그대로(AWAIT_L1 의 await 인증)이다.
  6. **측정 보고** `kz-static order … --caller-dependent-report FILE`: order 글과 간선 목록은 바뀌지 않는다(시험이 두 글을 견줌). 보고는 호출자에 기대는 spawn 호출의 호출자 가운데 풀린 entry 를 넘기지 않는 것의 수, 그런 호출자가 그 호출의 자기 스레드 모두를 만든다고 둔 둘째 닫힘에서 값이 달라지는 짝과 행(이 규칙이 unordered 로 남긴 짝의 위 한계).
  7. `kz-static show` 의 절 `loopdefs`, `lockhandles`.
- **레이아웃 점검.** 쓰는 코드가 개정 12 의 `kz_fact_loopdef`·`kz_fact_lockh` 와 열거형 `KZ_LOOPDEF_WHERE/STATUS/FLAG`, `KZ_LOCKH_STATUS/FLAG` 에 그대로 맞는다. `include/` 는 바꾸지 않는다. LOOP_DEFS 의 루프 자신의 행(`local` 빈칸)은 따로 섹션을 두지 않으려는 쓰임이다(`docs/schema.md` 의 날짜 붙은 줄).
- **1차 근거.**
  - 첫 경로의 대기 둘이 술어 없이 남았다: c1 두 빌드의 `CFE_ES_GetTaskFunction` 의 루프(`/home/user/work/procrace/c1_build/bundle_1df262a/cfe/modules/es/fsw/src/cfe_es_apps.c:508-531`: `ReturnCode = CFE_ES_ERR_APP_REGISTER` 뒤 몸 안 `if (TaskRecPtr != NULL) { EntryFunc = TaskRecPtr->EntryFunc; if (CFE_RESOURCEID_TEST_DEFINED(TaskRecPtr->AppId) && EntryFunc != 0) ReturnCode = CFE_SUCCESS; }`, 출구 `if (ReturnCode == CFE_SUCCESS || Timeout <= 0) break;`, `return ReturnCode;` :539)와 c1_post_tx 의 CF 재시도(`/home/user/work/procrace/c1_build/post/src/cf/fsw/src/cf_cfdp.c:1001-1011`: `ret = OS_CountSemGetIdByName(...)`, `if (ret != OS_ERR_NAME_NOT_FOUND) break;`, `OS_TaskDelay`). 앞판 facts 의 FACT_LOOPS 는 둘 다 “the condition reads the local … which is not a recognised counter” 였다(`rf_on_extract` 의 `kz-static show --section loops`). 그 호출자 `CFE_ES_TaskEntryPoint` 는 결과가 `CFE_SUCCESS` 이고 포인터가 NULL 이 아닐 때만 실제 entry 를 부른다(같은 파일 :552-566).
  - 2 세대 잠금: c1_pre_tx 의 LOCK_IDS 는 OSAL 잠금을 모두 `&OS_impl_mutex_table[?(the result of OS_ObjectIndexFromToken)].id unknown undecidable` 하나로 적는다(`rf_on_extract` 의 `--section lockids` #3). `CFE_ES_LockSharedData` 는 `OS_MutSemTake(CFE_ES_Global.SharedDataMutex)` 를 부르고(`cfe/modules/es/fsw/src/cfe_es_api.c:2150`), 헤더는 `OS_MutSemTake` 를 “Acquire the mutex object referenced by sem_id” 로 적는다(`osal/src/os/inc/osapi-mutex.h` 의 `OS_MutSemTake` 선언 위 brief 줄, `bundle_1df262a`). 이 판의 의미 자료(semantics coverage rule revision 1)에는 그런 인자 역할이 없다(`semantics/framework_semantics_osal-f34759d.txt` 의 `OS_MutSemTake` 는 api·ret 줄뿐).
  - 호출자에 기대는 spawn 규칙의 덜 잡음을 재라는 lead 의 판단 §6.10 의 4.
- **코드**(`kairos/`): `lib/llvm/{model.hpp,spec.hpp,ast_fn.cpp,serialize.cpp,facts_text.cpp,rel_facts.hpp,rel_facts.cpp,relations.hpp,relations.cpp,order.hpp,order.cpp}`, `tools/kz-static/{cmd_extract.cpp,cmd_show.cpp,main.cpp}`; 시험 `tests/static/{test_static_cli.cpp,static_golden.cpp,CMakeLists.txt}`, 새 fixture `tests/static/fixtures/looplock.c`, 손으로 쓴 시험 의미 자료 `tests/static/data/looplock_fixture/framework_semantics_fixture-lock.txt`(인자 역할 `lock-object`; 생성된 자료가 아님을 머리에 적음); 문서 `docs/static_facts.md` §5v, `docs/schema.md` 날짜 줄.
  - 하지 않는 것: `include/`, `cmake/`, 최상위 CMakeLists, `runtime/`, `instrument/`, `tests/lint/`(kz_lint 다시 링크 안 됨), `semantics/` 의 자료와 coverage rule, 계획 생성기의 규칙, 판정 규칙과 어휘 자료, 설명 스크립트, `one_build.sh`(옵션은 `ONE_BUILD_EXTRACT_EXTRA`).
- **도구 해시와 정체성**: kz-static 의 파일 sha256(빌드 전 `499748bc22881d9019e7dc31b9cc3a7aeb0740e16b5ad94207cc8e52503a163a`), `--identity` 의 `tool_sha256`(빌드 전 `221231bb…`)·`tool_source`(빌드 전 `ad79bdaa…`)가 바뀐다. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 는 그대로 [I]. 나머지 도구의 정체성과 파일은 그대로.
  - 다시 링크되는 것 14(`relink_expected.sha256`, 빌드 전 sha256): `libkz-llvm.a`, kz-static, `test_static_{reasons,plan,unroll,flow,elf32,wrappers,gitrev,link,recipe,expr}`, `test_static_cli`, `static_golden`.
  - 다시 링크되지 않아야 하는 것 110(`not_relink.sha256`, 빌드 전 sha256; 앞 단계 목록과 같은 바이트임을 확인함; kz `913fcd99…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz-rt-dump `4390d4f1…`, kz-instr-clang `28db4017…`, kz_lint `3d4d5a8f…` 포함). 빌드 뒤 같은 sha256 이어야 한다.
  - `CMakeCache.txt` `bc2f43b4…` 그대로 [I](`tests/static/CMakeLists.txt` 가 바뀌어 다시 구성된다).
- **빌드**(MC `/home/user/work/merge-check/kz-llvm` 제자리, 목표별, 각각 공간 감시 아래): `make kz-static`, `make all`. 감시가 도중에 멈추면 적고, kz-static 이 새 `--identity` 를 적을 때까지 출력을 내지 않는다.
- **도구 보존**: 빌드 전에 kz-static `499748bc` 를 새 디렉터리 `/home/user/work/preserve/tools-20261116-k/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 공간 감시 아래; 사본에서 푸는 바이너리는 디스크에 쓰지 않고 파이프로 확인). 사본 직후 등록부의 kz-static `499748bc` 행을 in-place+copy 로, 빌드 뒤 copy 로 고치고 새 kz-static 을 in-place 로 더한 다음 빌드 뒤 등록부 점검을 돌린다. 장부: 사본 디렉터리 행 `TOOLS_20261116k`.
- **golden**(빌드 전 scratchpad 점검 빌드로 미리 만듦 [I, 근거 아님]; `golden_scratch.sha256`: `looplock_np.golden` `10f19fc4…`, `looplock_order_np.golden` `9b0debe4…`): 새 golden 둘. **첫 ctest 에서 실패는 둘이어야 한다**: `static_golden_looplock_np`, `static_golden_looplock_order_np`(golden 파일이 아직 없음). `KZ_GOLDEN_UPDATE=1` 로 그 둘만 쓴 뒤 scratchpad 의 것과 `cmp` 같아야 하고, 그 밖의 golden 은 바뀌지 않는다(`git status`; scratchpad 점검에서 기존 golden 31 개가 새 도구로 모두 같았음 [I]).
- **시험**: 전체 ctest 264 → 267(새: `static_fixture_compile_looplock`, `static_golden_looplock_np`, `static_golden_looplock_order_np`), golden 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`). 첫 ctest 와 golden 뒤 ctest 를 모두 `-V` 로 돌려 확인 수를 남긴 출력에서 센다: `test_static_cli` 1,099 → 1,151 [I]; `test_static_reasons` 첫 ctest 692, golden 뒤 731 [I](새 `looplock_np.golden` 이 relations golden 이고 `incomplete:` 줄을 담음); `test_static_plan` 195, `test_core_schema` 34,017, `lint_selftest` 114, lint 규칙 여덟 clean. 단위 점검 `env/tools/tests/run_unit.sh` 16/16, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤와 다섯 빌드 실행 뒤 각각 `env/tools/ledger_check.sh`(빌드 전 114 행 실패 0: `ledger_check_pre.txt`)와 `env/tools/tools_check.sh --deep`(빌드 전 44 행 실패 0: `tools_check_pre.txt`), 출력은 단계마다 새 이름.
- **옵션 없는 경로(기본 경로)는 바이트 동일**(새 디렉터리, `compare.sh`):
  - relations, T8CF facts(`rerun.sh` → `ll_relcheck`): `relations.txt`·`keys.txt` 가 `rf_relcheck` 와 바이트 동일, `relations.log` 빈 파일.
  - order, T8CF facts(`order.sh` → `ll_order`): 두 번 같음, `rf_order` 와 바이트 동일, 로그 빈 파일.
  - 다시 뽑기 옵션 없음(`extract.sh off` → `ll_off_extract`), `rf_off_extract` 와: `basis.kz` 레코드 단위(도구 해시만), `facts.kz` 전체 dump(`check_whole.py`), facts 보기 `extract.units_from` 한 줄만, `extract.txt` 1 줄, `relations.txt` 2 줄, `basis.txt`·`sha256.txt` 해시 줄만, `args.txt`·`relations.log` 같음.
  - 그 facts 위의 order·relations·키(`order.sh ll_off_extract ll_off_order`): 두 번 같음, `order.txt` 는 `rf_off_order` 와 2 번째 줄만, `edges.tsv` 같음, `relations.txt` = 다시 뽑기의 것, `keys.txt` 2 번째 줄만, 로그 빈 파일.
  - 계획(`plans.sh` → `ll_plans`; `plans.sh ll_off_extract ll_plans_off`): `compare_plans.py t8cf rf_plans/<b>`, `extract rf_plans_off/<b>` 의 허용만.
- **마지막 옵션 회차(다섯 빌드)**: 다시 뽑기(`extract.sh final` → `ll_final_extract`: `--object-access`, framework 의미 자료 일곱·P0 선언 하나의 `--semantics`, `--result-flow`, `--loop-defs`, `--lock-handles`), 마지막 중간 회차 `rf_on_extract` 와(`compare.sh final`):
  - `basis.kz` 레코드 단위(도구 해시만). `facts.kz` 머리(`check_head_rf.py --allow-size --new-sections 2`). **섹션은 레코드 글로**(`section_text_cmp_ll.py`): 앞 회차의 모든 섹션이 같은 레코드 글, 다만 STRINGS 와 BASIS 는 다를 수 있고 FACT_EXPRS 는 앞 회차의 줄이 새 것의 첫 줄들; 새 섹션은 LOOP_DEFS 와 LOCK_HANDLES 둘. 섹션별 레코드 글 sha256 을 `section_text_final.txt` 에 남긴다.
  - facts 보기(`compare_ll.py`): `[loopdefs]`·`[lockhandles]` 절이 새로, `[basis]` 에 새 항목 넷(`extract.loop_defs`, `count.loop_defs`, `extract.lock_handles`, `count.lock_handles`)과 `extract.units_from` 의 basis 해시, 그 밖은 같음.
  - `extract.txt`: 1 줄과 새 줄 `loop defs …`·`lock handles …` 둘만. `relations.txt`(`compare_rel_ll.py`): 2 줄과 새 머리 한 줄 `rules with lock handles:` 만(다섯 빌드에서 핸들로 읽는 경로 C 행은 0 [I]; 행이 바뀌면 덧붙임). `args.txt`: extract-only 줄만(끝에 ` --loop-defs --lock-handles`). `basis.txt`·`sha256.txt` 해시 줄만. `relations.log` 빈 파일.
  - 그 facts 위의 order·relations·키·보고(`order.sh ll_final_extract ll_final_order report`): 두 번 같은 바이트(`order.txt`, `edges.tsv`, `cdr.txt`); `order.txt`(`compare_ord_ll.py`): 2 줄, 새 머리 한 줄, `rules with result flow:` 줄은 수만, `summary edges listed` 는 합·HB-await·conditional 과 HB-seq 단계 수(줄지 않음; 조건 간선이 있는 닫힘이 HB-await 간선의 끝까지 감)만, `summary waits` 는 인식 수와 의존 사용만, 간선은 앞 것이 모두 같은 줄로(HB-lock 간선에 `lock identity source:` 한 줄 더함) 있고 새 간선은 HB-await 만, 대기는 같고 `predicate:`·`awaited:` 줄과 `use the call of` 줄의 결과만 바뀌며 `use caller-side:` 줄이 더해짐, `[rows]` 는 같은 바이트; `edges.tsv`(`--tsv`): 앞 줄 모두 있고 새 줄은 HB-await 만; `relations.txt` = 다시 뽑기의 것, `keys.txt` 는 `rf_on_order` 와 2 번째 줄만, 로그 빈 파일.
  - 계획(`plans.sh ll_final_extract ll_plans_final`): `compare_plans.py extract rf_plans_on/<b>` 의 허용만(생성기는 새 섹션을 읽지 않음).
  - **중간 회차와의 섹션 비교(계획 4.3)**(`compare.sh rounds`): 마지막 회차의 facts 를 중간 회차 셋(`objacc_on_extract`, `sem_on_extract`, `rf_on_extract`)의 컨테이너와 레코드 글로(`section_text_cmp_ll.py --new` 에 뒤 옵션들이 더한 섹션), 그리고 각 회차가 남긴 섹션별 레코드 글 sha256(`records/semantics-20261116/section_text_old_2.txt`, `section_text_new_2.txt`, `records/result-flow-20261116/section_text_on.txt`)을 `section_text_final.txt` 와(`rounds_cmp.py`). 다른 섹션은 BASIS(뒤 옵션의 항목과 도구 정체성)와 FACT_EXPRS(뒤 섹션이 뒤에 더한 뿌리; 앞 줄이 같음은 컨테이너 비교가 봄)여야 한다; 그 밖은 덧붙임. 다른 섹션이 그 둘뿐이면 그 회차의 섹션은 마지막 회차가 레코드 글로 다시 낸다고 장부 행의 비고에 적는다(그 회차의 인자로 다시 돌린 것은 아님). 어느 중간 회차도 지우지 않는다(지우는 것은 lead 의 일, 계획 4.3).
  - **마지막 facts 를 뒤 단계의 입력으로**: 장부 행 `OBS_ll_final_extract` 의 비고에 “the final option-on round: the facts the next steps read (kz-static <새 해시>, every extraction option)”를 적는다.
- **측정**(`compare.sh measure`, `measure_ll.py`; 모든 수는 `compare/` 의 남긴 파일에서): LOOP_DEFS 의 행·루프 상태, 인식한 루프와 첫 경로의 대기 둘(`cfe_es_apps.c:508`·r2 의 `:573`, `cf_cfdp.c:1001`), LOCK_HANDLES 의 상태별 행과 핸들 필드, 대기의 인식 수(중간 회차 → 마지막 회차), HB-await 간선, 호출자 쪽 사용, HB-lock 간선과 그 정체성 출처, 호출자에 기대는 spawn 규칙의 보고, relations 의 핸들 경로 C 행. 측정이고 판정이 아니다.
  - [I](scratchpad 점검 빌드, LLVM Release 구성, 감시 아래; 근거 아님, 지움): LOOP_DEFS 루프 5/6/4/5/5(c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642), 인식 1/2/1/0/0 — c1 두 빌드의 `CFE_ES_GetTaskFunction` 루프(form flag+value: `(and (ne (call-result CFE_ES_GetTaskRecordByContext) 0) (and (call-result CFE_ResourceId_IsDefined (seq field:CFE_ES_TaskRecord_t.AppId)) (ne field:CFE_ES_TaskRecord_t.EntryFunc 0)))`), c1_post_tx 의 CF 재시도(form value: `(ne (call-result OS_CountSemGetIdByName …) -17)`), r2_core 의 같은 ES 루프(`cfe_es_apps.c:573`); c5 두 빌드는 인식 0.
  - [I] LOCK_HANDLES 1,573/1,573/967/0/0 행, `static-instance` 0(다섯 빌드); `CFE_ES_LockSharedData` 의 핸들은 `CFE_ES_Global_t.SharedDataMutex`, 상태 not-static(FIELD_STATUS: writer 인스턴스 2, `cfe_es_start.c` 의 memset 과 `OS_MutSemCreate` out-pointer; `lock-object` 항목 없음). c5 두 빌드는 1 세대 잠금이 이미 static instance 라 행이 없다. relations 의 핸들 경로 C 행 0, HB-lock 의 핸들 간선 0; c5 의 HB-lock 18/48 은 `must-static-instance (LOCK_IDS)`.
  - [I] 대기 인식 5→6, 5→7, 3→4, 0→0, 1→1; HB-await 간선 0→120, 0→120, 0→64, 0, 0(모두 conditional: LOOP_DEFS 의 술어에는 AWAIT_L1 의 await 인증이 없음); 호출자 쪽 사용 0→4(c1 두 빌드, r2; `CFE_ES_TaskEntryPoint` 의 호출 넷, 예산 값 `3288334359` = `CFE_ES_ERR_APP_REGISTER`, skip). 행의 order.static 값은 바뀌지 않음.
  - [I] 호출자에 기대는 spawn: 그런 호출 5/5/5/2/2, 그 호출자 13/13/11/6/6, 풀린 entry 를 넘기지 않는 호출자 2/2/3/1/1; 둘째 닫힘에서 값이 달라지는 짝 0/0/461/0/0(r2_core: unordered → fixed_early_proved 239, → hard_before 222), 행 0.
- **공간**: 각 빌드·사본·실행은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 빌드 필요량 = 앞 kz-static 빌드 dip 의 최대 256,942,080 + 새 출력 약 200 MB(다시 뽑기 둘 약 125 MB, order 넷과 둘째 실행 약 65 MB, relations 약 8 MB, 계획 셋 약 3 MB) + 한 시간 흐름 13 MB = 469,942,080 B. 사본은 필요량 100,000,000(도구 파일 하나를 읽어 xz 를 씀; 푸는 바이너리는 파이프로만 확인하고 디스크에 쓰지 않음). 실행 단계와 무거운 비교(extract, final, rounds, measure)는 필요량 100,000,000. 점검 빌드(scratchpad)는 감시 아래에서 했고(`df_scratch_*.log`) 지웠다.
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py`, `check_head_rf.py`, `section_text_sha_2.py`(앞 단계의 것을 옮김), `section_text_cmp_ll.py`, `compare_ll.py`, `compare_rel_ll.py`, `compare_ord_ll.py`, `rounds_cmp.py`, `measure_ll.py`, `compare.sh`, `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`, `golden_scratch.sha256`; 빌드 전 점검 `tools_check_pre.txt`, `ledger_check_pre.txt`.

### 10.1 바뀐 것 [F]

- 커밋 차례: `08760d6`(지금까지의 코드, fixture, 시험 의미 자료; 선언 전의 스냅숏), `ad30cf0`(선언 §10.0, 문서 §5v, 변수 이름 고침, 비교·측정 스크립트, 빌드 전 목록; 빌드 전), `870c499`(사본·빌드 기록, 새 golden 둘, 등록부 행; 다섯 빌드 전), 그리고 이 절의 커밋.
- 규모(`dc632e7` 에서 `ad30cf0` 까지, `kairos/`): `ast_fn.cpp` +350 쯤(`emit_loop_defs`: 예산 지역을 뺀 조건 출구, flag·value 규칙, 정의 행과 루프 행), `serialize.cpp` +250 쯤(LOOP_DEFS·LOCK_HANDLES 행과 BASIS 항목 넷), `relations.cpp` +95(revision 11: `handle_at`, `common_handle`, 경로 C 의 핸들 갈래), `order.cpp` +200(revision 4: LOOP_DEFS 술어, `flag_budget_values`, `lock identity source:` 줄, 호출자에 기대는 spawn 의 둘째 닫힘과 보고), `facts_text.cpp` +46(절 둘), `model.hpp`·`spec.hpp`·`rel_facts.*`·`order.hpp`·`relations.hpp`, `cmd_extract.cpp`·`cmd_show.cpp`·`main.cpp`(옵션 둘과 `--caller-dependent-report`), 시험 `test_static_cli.cpp` +70, `static_golden.cpp` +11, fixture `looplock.c` 290 줄, 시험 의미 자료 14 줄, golden `looplock_np.golden`·`looplock_order_np.golden`(`870c499`), 문서 `static_facts.md` §5v, `schema.md` 날짜 줄.
- 바꾸지 않은 것: `include/`, `cmake/`, 최상위 CMakeLists, `runtime/`, `instrument/`, `tests/lint/`, `semantics/`, 계획 생성기 규칙, 판정 규칙·어휘, 설명 스크립트, `one_build.sh`. relations 의 `rules` 줄(revision 9)과 order 의 첫 줄(revision 2)은 두 섹션 없는 facts 에서 앞판 그대로다.

### 10.2 빌드 [M]

MC 에서 제자리, 목표별, 각각 공간 감시 아래(필요량 469,942,080). 사본 22:13:01Z, 빌드 시작 22:16:39Z, 빌드 전 HEAD `ad30cf0`(`pre_mc.txt`), kairos 아래 커밋 안 된 줄 0(`pre_mc.txt`, `post_mc.txt`). 첫 `make kz-static` 줄은 감시 스크립트를 상대 경로로 불러 rc 127 로 아무것도 돌지 않았고(`commands.txt` §1), 절대 경로로 다시 돌렸다.

| 목표 | 때 (UTC) | dip (B) |
|---|---|---|
| `kz-static` | 22:16:39–22:22:04 | 119,521,280 |
| `all` | 22:22:12–22:26:35 | 34,725,888 |

- 다시 링크된다고 적은 14 가 모두 바뀌었다(`relink_result.txt`). `not_relink.sha256` 110 모두 같은 바이트.
- 도구(`post_mc.txt`, `identity_kz-static.txt`): kz-static `499748bc…` → `9eb1395f3bd02fb4b4dc9b8be72b4e336845c3715481b0e38398b0dbd40f9452`; `--identity` 의 `tool_sha256` `221231bb…` → `0e1d8599cde099002d7670c67d8670c6e226fce19a9c09f150bb895b9aeab216`, `tool_source` `ad79bdaa…` → `a2365b1979f7dfe0ff191ae03c2aeb2ee35f6043e59cdf14b022064253899c6b`; `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 그대로. `CMakeCache.txt` `bc2f43b4…` 그대로.
- 도구 보존: 빌드 전 kz-static `499748bc` 를 `/home/user/work/preserve/tools-20261116-k/kz-static.499748bc.xz`(46,534,188 B)로 사본, 왕복 확인(`tool_copy/ROUNDTRIP.txt`: xz -t OK, 푼 sha256 같음, cmp 같음; 푼 것은 파이프로만). 등록부: 사본 직후 in-place+copy(`tools_check_after_copy.txt` 44 행 실패 0), 빌드 뒤 copy, 새 `9eb1395f` in-place(`tools_check_after_build.txt` 45 행 실패 0, deep).

### 10.3 시험 [M]

- 첫 ctest(`ctest_first.log`, `-V`): 267 개 가운데 실패 2, 선언한 그 둘(`static_golden_looplock_np`, `static_golden_looplock_order_np`, golden 없음). 건너뜀 1. 그 실행의 `test_static_reasons` 692, `test_static_cli` 1,151(`ctest_first.log` 의 줄).
- `KZ_GOLDEN_UPDATE=1` 로 그 둘만 썼다(`golden_update.log`). scratchpad 점검 빌드의 것과 `cmp` 같다(`10f19fc4…`, `9b0debe4…`; `commands.txt`). 다른 golden 은 바뀌지 않았다(`git status` 에 새 파일 둘뿐).
- 갱신 뒤 전체 ctest(`ctest_full.log`, `-V`): 267 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`).
- 확인 수(`ctest_counts.log`, golden 뒤): `test_static_cli` 1,151, `test_static_reasons` 731, `test_static_plan` 195, `test_core_schema` 34,017, `lint_selftest` 114 경우 실패 0, lint 규칙 일곱 clean(selftest 와 함께 lint 시험 여덟 통과).
- 단위 점검 `run_unit.sh` 16/16(`unit_tools.txt`), `step2a_tests_unit.sh` 7/7(`unit_step2a.txt`).

### 10.4 바이트 동일성 [M]

| 비교 | 결과 |
|---|---|
| relations, T8CF facts (`compare_relations.txt`) | 15 OK: `relations.txt`·`keys.txt` 가 `rf_relcheck` 와 바이트 동일, 로그 빈 파일 |
| order, T8CF facts (`compare_order.txt`) | 25 OK: 두 번 같음, `rf_order` 와 바이트 동일 |
| 다시 뽑기 옵션 없음 vs `rf_off_extract` (`compare_extract.txt`) | 45 OK: 도구 정체성의 값만 |
| 그 facts 위 (`compare_onextract.txt`) | 40 OK |
| 계획 (`compare_plans.txt`) | 열 모두 "only declared differences" |
| 마지막 옵션 회차 vs `rf_on_extract` (`compare_final.txt`) | 50 OK |
| 그 facts 위 (`compare_final_order.txt`) | 45 OK(order·간선·보고 두 번 같은 바이트) |
| 계획 (`compare_plans_final.txt`) | 다섯 "only declared differences" |
| 중간 회차 셋과 (`compare_rounds.txt`, `compare/rounds_sha.txt`) | 16 OK |

- **옵션 없는 경로는 앞판과 같다**(위 첫 다섯 줄). 다른 것은 도구 정체성에서 오는 값과 계획의 생성기 정체성 줄뿐이다.
- **마지막 회차 대 결과 흐름 회차**: 섹션은 빌드마다 앞 회차의 45 가운데 44 가 같은 레코드 글(FACT_EXPRS 는 c1_pre_tx +3, c1_post_tx +4, r2_core +3 행이 뒤에 더해짐, c5 두 빌드는 그대로), 다른 것 BASIS, 새 것 LOOP_DEFS(13/19/13/11/11 행)·LOCK_HANDLES(1,573/1,573/967/0/0 행)(`compare/final_sections_<b>.txt`; 섹션별 레코드 글 sha256 `section_text_final.txt`). facts 보기는 `[loopdefs]`·`[lockhandles]` 와 BASIS 새 항목 넷뿐(`compare/final_<b>.txt`). relations 는 2 번째 줄과 새 머리 한 줄뿐(`compare/final_relations_<b>.txt`). order 는 새 머리 한 줄, 새 HB-await 간선 120/120/64/0/0, HB-lock 간선의 `lock identity source:` 줄 0/0/0/18/48, 대기의 predicate 줄 5/6/4/5/5·awaited 줄 1/2/1/0/0·`use the call of` 결과 4/6/4/0/2·`use caller-side:` 줄 4/4/4/0/0, `[rows]` 같은 바이트(`compare/final_order_<b>.txt`); `edges.tsv` 는 HB-await 줄만 더해짐(`compare/final_edges_<b>.txt`).
- **중간 회차와의 섹션 비교(계획 4.3)**: 컨테이너로(`compare/rounds_{objacc,sem,rf}_<b>.txt`) 열다섯 모두 OK: 회차의 섹션 42/43/45 가운데 BASIS 밖 모두 같은 레코드 글, FACT_EXPRS 는 앞 줄이 같고 뒤에 더해짐(rf 회차의 c5 두 빌드는 그대로). 남긴 sha256 으로(`compare/rounds_sha.txt`) 열다섯 모두 다른 섹션이 BASIS·FACT_EXPRS 뿐(c5 의 rf 회차는 BASIS 뿐), 마지막 회차에 없는 섹션 0. 그래서 세 중간 회차의 장부 행 비고에 “마지막 회차가 레코드 글로 다시 냄(BASIS·FACT_EXPRS 밖; 그 회차의 인자로 다시 돌린 것은 아님)”을 더했다. 어느 회차도 지우지 않았다.
- **누적 목록과 등록부**: 빌드 전 `ledger_check_pre.txt` 114 행·`tools_check_pre.txt` 44 행 실패 0; 빌드 뒤 `ledger_check_after_build.txt` 114 행, `tools_check_after_build.txt` 45 행 실패 0(deep); 장부 행 열을 더한 뒤 `ledger_check_end.txt` 124 행 실패 0(새 행 `OBS_ll_relcheck`, `OBS_ll_order`, `OBS_ll_off_extract`, `OBS_ll_off_order`, `OBS_ll_final_extract`, `OBS_ll_final_order`, `OBS_ll_plans`, `OBS_ll_plans_off`, `OBS_ll_plans_final`, `TOOLS_20261116k`), `tools_check_end.txt` 45 행 실패 0(deep).
- **마지막 facts**: `OBS_ll_final_extract` 의 비고가 “the final option-on round: the facts the next steps read (kz-static 9eb1395f, every extraction option …)”이다. 뒤 단계(계획 생성기 다음 판, 주기)는 `/home/user/work/redesign/obs1/ll_final_extract/<b>/<b>/NP/facts.kz` 를 읽는다.
- 바깥 이름 점검(`--tree`)은 아홉 출력 디렉터리, 사본 디렉터리, 기록 디렉터리에서 깨끗하다(`name_check_tree.txt`, 11 모두 rc 0).
- **2026-11-18 검토 뒤 정정: §12.7 `builds-02`.** “45 가운데 44 가 같은 레코드 글”과 중간 회차 비교의 “BASIS·FACT_EXPRS 뿐”은 틀렸다: STRINGS 는 레코드 줄이 없어 글로는 같다고 세어졌지만 count 가 회차마다 다르다(c1_pre_tx: 3,740,581 → 3,759,771). 빌드마다 글과 count 가 같은 섹션은 45 가운데 42 이고(c5 는 FACT_EXPRS 가 같아 43) 다른 것은 STRINGS·BASIS·FACT_EXPRS 다. 선언 §10.0 의 중간 회차 비교는 STRINGS 를 빠뜨렸다: 덧붙임 §12.0 (가-1) 을 커밋한 뒤 count 도 견주는 `rounds_cmp_2.py` 로 다시 돌렸다(`records/review-fixes-2-20261118/recheck/recheck_rounds.txt`: 열다섯 OK, 다른 것 STRINGS·BASIS·FACT_EXPRS 뿐). 장부의 네 행(`OBS_objacc_on_extract`, `OBS_sem_on_extract`, `OBS_rf_on_extract`, `OBS_ll_final_extract`)의 비고도 고쳤다. STRINGS 의 내용은 그 비교가 보지 않으므로, 지금의 비고만으로 중간 회차를 다시 만들 수 있다거나 지워도 된다고 읽지 않는다.

### 10.5 측정 [M]

마지막 회차(`ll_final_extract`)와 그 위의 order·relations·보고(`ll_final_order`), kz-static `9eb1395f`. 수의 출처: `compare/measure_<b>.txt`(`measure_ll.py`)와 `compare/loopdefs_<b>.txt`. 측정이고 판정이 아니다. 모든 값이 선언 §10.0 의 scratchpad [I] 와 같다.

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| LOOP_DEFS 루프 (인식 / 거절·미검사) | 5 (1 / 4) | 6 (2 / 4) | 4 (1 / 3) | 5 (0 / 5) | 5 (0 / 5) |
| LOCK_HANDLES 행 (static-instance / writer-set-incomplete / not-static) | 1,573 (0 / 23 / 1,550) | 1,573 (0 / 23 / 1,550) | 967 (0 / 11 / 956) | 0 | 0 |
| 대기 인식: 결과 흐름 회차 → 마지막 회차 | 5 → 6 | 5 → 7 | 3 → 4 | 0 → 0 | 1 → 1 |
| HB-await 간선 (모두 conditional) | 0 → 120 | 0 → 120 | 0 → 64 | 0 | 0 |
| 호출자 쪽 사용 (flag 로 구한 예산 값) | 0 → 4 (1) | 0 → 4 (1) | 0 → 4 (1) | 0 | 0 |
| HB-lock 간선 (정체성 출처) | 0 | 0 | 0 | 18 (must-static-instance) | 48 (must-static-instance) |
| 핸들로 읽은 경로 C 행 | 0 | 0 | 0 | 0 | 0 |
| 호출자에 기대는 spawn 호출 / 그 호출자 / 풀린 entry 를 넘기지 않는 호출자 | 5 / 13 / 2 | 5 / 13 / 2 | 5 / 11 / 3 | 2 / 6 / 1 | 2 / 6 / 1 |
| 위 한계의 닫힘에서 값이 달라지는 짝 / 행 (짝 수) | 0 / 0 (24,609) | 0 / 0 (24,609) | 461 / 0 (8,566) | 0 / 0 (4,320) | 0 / 0 (4,311) |
| 행 값(order.static) | 결과 흐름 회차와 같음 | 같음 | 같음 | 같음 | 같음 |

- **이름 댄 두 대기**(`compare/measure_<b>.txt` 의 `named wait:`): c1 두 빌드의 `CFE_ES_GetTaskFunction` 루프(`cfe_es_apps.c:508:5`)와 r2_core 의 같은 루프(`:573:5`)가 form flag+value 로 인식된다: 술어 `(and (ne (call-result CFE_ES_GetTaskRecordByContext) 0) (and (call-result CFE_ResourceId_IsDefined (seq field:CFE_ES_TaskRecord_t.AppId)) (ne field:CFE_ES_TaskRecord_t.EntryFunc 0)))`, 깃발 `under-lock`, `call-result`(`compare/loopdefs_c1_pre_tx.txt`: `ReturnCode` before-loop 3288334359, in-body 0 의 guard, `EntryFunc` value, `TaskRecPtr` value). c1_post_tx 의 CF 재시도(`cf_cfdp.c:1001:13`)는 form value: `(ne (call-result OS_CountSemGetIdByName (seq (addr field:CF_Channel_t.sem_id) (seq field:CF_ChannelConfig_t.sem_name))) -17)`. c5 두 빌드에는 인식한 루프가 없다(거절 refused-unresolved 3, refused-address-taken 1, not-examined 1).
- **그 대기의 순서 간선**: 기다리는 필드 `CFE_ES_TaskRecord_t.AppId`·`.EntryFunc`, 다른 스레드의 writer 둘(`CFE_ES_StartAppTask` 의 `cfe_es_apps.c:619:9`, `:620:9`), 호출자 `CFE_ES_TaskEntryPoint` 의 호출(`cfe_es_apps.c:552:9`)에서 예산 값 3288334359(`CFE_ES_ERR_APP_REGISTER`, flag 의 루프 앞 값; 몸 안 정의 0 은 뺌)로 guard 를 평가해 그 아래 호출 넷(`:559`, `:560`, `:561`, `:566` 의 간접 호출)이 skip. 그래서 writer → 그 호출들의 HB-await 간선이 c1 두 빌드에 120, r2 에 64 생겼고, 모두 conditional 이다: L1 의 인증(AWAIT_L1 per-field)이 이 루프에 `star-closed` 이고 LOOP_DEFS 의 술어에는 await 인증이 없다(hard 조건 `l1 && …`, `order.cpp` waits). 행의 order.static 은 바뀌지 않았다(conditional 간선은 hard 닫힘에 들지 않고, 이 간선들의 끝은 relations 행의 사용이 아님).
- **핸들 정체성**: c1·r2 의 행 가운데 `static-instance` 0, `semantics-unverified` 0 이다. `CFE_ES_LockSharedData` 의 핸들 `CFE_ES_Global_t.SharedDataMutex`(그 필드로 1,573 행 가운데 722)는 FIELD_STATUS 의 writer 인스턴스 2(memset `cfe_es_start.c:81` 과 `OS_MutSemCreate` out-pointer `:92`)라 not-static 이고, 의미 자료에 `lock-object` 항목이 없다는 것도 전제 글에 적힌다. 핸들이 전역 객체의 필드가 아닌 행(토큰의 `obj_type` 등) 718. c5 두 빌드는 1 세대 잠금이 LOCK_IDS 에서 이미 static instance 라 행이 없고, HB-lock 18/48 은 모두 `must-static-instance (LOCK_IDS)`.
- **호출자에 기대는 spawn 규칙의 덜 잡음**(§6.10 의 4): 풀린 entry 를 넘기지 않는 호출자 2/2/3/1/1(c1: `OS_TimeBaseCreate_Impl` 의 `os-impl-timebase.c:350:19`, `OS_TimeBaseCreate` 의 `osapi-timebase.c:141:23`; `ll_final_order/<b>/cdr.txt` 의 `[callers]`). 그런 호출자가 그 호출의 자기 스레드 모두를 만든다고 둔 닫힘에서 값이 달라지는 짝은 r2_core 에서만 461(unordered → fixed_early_proved 239, → hard_before 222), 행 값이 바뀌는 행은 다섯 빌드 모두 0 이다(그 짝들이 든 행에는 다른 unordered·unknown 짝이 남음). 461 은 이 규칙이 unordered 로 남긴 짝의 위 한계이고, 그 가운데 실제로 순서가 있는 짝의 수는 정해지지 않았다 [미확인].
- **2026-11-18 검토 뒤 정정: §12.7 `semantics-1`, `semantics-7`, `semantics-8`, `semantics-9`.** (1) **HB-await 120(c1)·64(r2)의 writer 스레드는 일부가 틀렸다.** 모든 간선의 from_actor 가 `main (process)` 인데, spawn entry 인 기다리는 스레드 다섯(`BP_CLA_InputTask`, `BP_CLA_OutputTask`, `CFE_ES_BackgroundTask`, `CFE_TIME_Local1HzTask`, `CFE_TIME_Tone1HzTask`)의 task 레코드는 만드는 응용 스레드가 쓴다(`CFE_ES_CreateChildTask` → `CFE_ES_StartAppTask`, `cfe_es_api.c:1329`; 호출자 `BP_AppMain`·`CFE_ES_TaskMain`·`CFE_TIME_TaskMain`). c1 의 120 가운데 그 다섯으로 가는 40 은 그 레코드를 쓰지 않는 스레드에서 온 간선이고, 실제 writer 에서 오는 간선은 빠져 있다; “다른 스레드의 writer 둘”도 틀렸다. 까닭: `order.cpp` 가 writer 의 스레드가 기다리는 스레드들 가운데 하나이면 모두에게서 뺐다. 모든 간선이 conditional 이라 행 값은 달라지지 않는다. (2) **CF 재시도 대기의 awaited 필드**(`CF_Channel_t.sem_id`, `CF_ChannelConfig_t.sem_name`)는 조회 호출의 out 인자와 열쇠다: 그 대기가 기다리는 것은 callee 의 이름 표다. 다른 스레드의 writer 가 0 이라 틀린 간선은 없었다. (3) **핸들 722 행**: `CFE_ES_Global_t.SharedDataMutex` 를 핸들로 가진 722 행 가운데 mutex family(`OS_impl_mutex_internal_record_t.id`)는 383 이고, 337 은 객체 종류 표 잠금(`OS_impl_objtype_lock_t.mutex`), 2 는 timebase 의 `handler_mutex` family 다: 뒤 둘은 그 핸들이 가리키는 잠금이 아니다. 그리고 63 행은 잠금 연산이 아닌 함수의 인자를 핸들로 적었다(`CFE_ES_ExitApp` 의 종료 상태, `CF_DoSuspRes` 의 명령 메시지 등; 검토의 집계). 모두 not-static 이라 order·relations 의 결과는 달라지지 않는다. (4) **LOOP_DEFS 정의 행의 깃발**은 정의마다가 아니라 지역마다 찍혔다(루프 앞 정의의 `under-lock`, 상수 정의의 `call-result`). 넷 다 코드로 고쳤고(`static_facts.md` §5w) MC 도구는 앞판이다. scratch 점검 [I, 근거 아님; 앞판 도구가 낸 마지막 회차 facts 를 고친 order 로 읽음; `records/review-fixes-2-20261118/scratch_measure.txt`]: HB-await 간선 줄 120 → 284(c1 두 빌드), 64 → 116(r2), order.static 이 바뀌는 행 0.

### 10.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 22:13:01 | 1,933,901,824 | 사본 시작(dip 46,555,136) |
| 22:16:39 | 1,887,170,560 | `make kz-static`(가장 낮은 1,767,649,280, dip 119,521,280), `all`(34,725,888) |
| 22:31:05–22:37:43 | 1,861,468,160 → 1,671,274,496 | relations(dip 7,675,904), order(14,209,024), 다시 뽑기 옵션 없음(47,161,344), 그 order(21,856,256), 계획 둘(987,136 씩), 마지막 회차(72,339,456), 그 order 와 보고(23,965,696), 계획(991,232) |
| 22:37:50–22:39:21 | 1,671,213,056 → 1,692,893,184 | 비교 열(모두 감시 아래; 가장 큰 dip `final` 70,455,296, facts 보기는 scratchpad 에 쓰고 지움) |
| 22:41:47 | 1,692,766,208 | 장부·등록부·이름 점검 뒤 |

- 하한(314,572,800)을 넘지 않았다. 빌드 dip 은 필요량 안이었다.
- 남긴 출력(장부 행): `ll_relcheck` 7,630,029, `ll_order` 7,058,482, `ll_off_extract` 46,967,354, `ll_off_order` 14,689,595, `ll_final_extract` 72,159,659, `ll_final_order` 15,953,440, `ll_plans` 886,291, `ll_plans_off` 886,583, `ll_plans_final` 894,413 B(합 167,125,846); 사본 46,534,188 B. 지운 것: scratchpad 의 점검 빌드와 그 출력(선언 전), 비교의 facts 보기(scratchpad), order 의 둘째 실행 디렉터리(같은 바이트임을 비교한 뒤).

### 10.7 계획과 다르게 한 것

- **핸들 정체성의 자리**: 설계 연구는 핸들 정체성을 order 출력의 칸으로만 두었다. HB-lock 간선은 relations 경로 C 의 proven-impossible 에서 오므로(`order.cpp` `lock_edges`), 핸들 정체성은 추출의 LOCK_HANDLES(개정 12 의 섹션 144)와 relations 경로 C 의 갈래(revision 11)에 두고, order 에는 `lock identity source:` 줄만 더했다. 의미 항목은 SEMANTICS 의 인자 역할 `lock-object` 로 읽는다(엔진에 API 이름 없음).
- **실제 빌드에서 핸들 정체성이 서지 않는다**: 생성된 의미 자료에 `lock-object` 역할이 없어(coverage rule revision 1) `static-instance` 가 나올 수 없고, 그 갈래는 손으로 쓴 fixture 시험 자료로만 시험했다(`looplock_np` 의 경로 C 두 행 proven-impossible, HB-lock hard 둘). 의미 자료가 있었어도 c1·r2 의 공유 잠금 핸들은 writer 인스턴스 둘이라 not-static 이다.
- **HB-await 는 conditional 이다**: 설계 연구의 기대([I])는 hard 였다. LOOP_DEFS 의 술어에는 AWAIT_L1 의 await 인증이 없어 hard 조건을 채우지 않는다. 인증 규칙을 LOOP_DEFS 술어로 넓히지 않았다(AWAIT_L1 은 바뀌지 않는다는 선언).
- **첫 `make kz-static` 줄**이 감시 스크립트를 상대 경로로 불러 rc 127(아무것도 돌지 않음); 절대 경로로 다시 돌렸다(`commands.txt` §1).
- **선언 전의 스냅숏 커밋 하나**(`08760d6`)가 있다. 그 뒤 선언 커밋 `ad30cf0` 에 변수 이름 고침(lint case-ids)이 들어갔고, 빌드·실행·비교는 모두 그 뒤다. scratchpad golden 은 이름 고침 전 빌드로 만들었고(출력이 같은 고침), MC 빌드의 golden 과 `cmp` 같았다.
- **가벼운 비교도 감시 아래** 돌렸다(앞 단계는 감시 없이 돌림).
- **호출자에 기대는 spawn 의 측정**은 위 한계(둘째 닫힘)로 쟀다: 실제로 순서가 있는 짝의 수가 아니다.
- **2026-11-18 검토 뒤 정정: §12.7 `builds-04`.** scratch 의 다섯 빌드 추출과 facts 보기는 공간 감시 기록 없이 돌렸다(남긴 `df_scratch_*` 는 cmake·make 뿐).

### 10.8 열린 것

- **의미 자료의 `lock-object` 역할**: 버전에 묶인 헤더의 brief 줄(`osapi-mutex.h` 의 `OS_MutSemTake` “Acquire the mutex object referenced by sem_id”, `OS_MutSemGive` “Releases the mutex object referenced by sem_id”)에서 이 역할을 만드는 것은 semantics coverage rule 의 다음 개정과 생성된 자료의 다시 만들기다(그 규칙을 어느 자료보다 먼저 커밋하는 차례). 그때까지 LOCK_HANDLES 는 실제 빌드에서 static-instance 를 주지 않는다.
- **핸들 필드의 writer 둘**: `CFE_ES_Global_t.SharedDataMutex` 는 전역의 memset(0)과 생성 호출의 out-pointer 두 인스턴스다. 앞선 0 채움을 초기값으로 읽어 “한 번 도는 writer 하나”로 칠지는 LOCK_IDS 의 값 읽기 규칙과 함께 정할 일이다 [미확인: 그렇게 읽어도 건전한지].
- **2 세대 잠금 집합**: LOCKSET 은 정체성을 모르는 해제(다른 형의 잠금이라도)가 있는 호출 뒤 OSAL 잠금을 잃는다(c1 `CFE_ES_GetTaskFunction` 안 `:513` 뒤 held 0). 핸들 단위의 잡힘 집합은 잠금 집합 분석을 핸들로 다시 돌려야 하고, 그 전에는 경로 C 가 핸들 잠금을 볼 일이 드물다.
- **LOOP_DEFS 술어의 L1 인증**: HB-await 가 hard 가 되려면 그 술어의 읽기에 대한 L1 (i)/(ii) 인증이 있어야 한다. AWAIT_L1 을 바꾸지 않는 새 섹션이나 order 쪽 규칙으로 두는 것이 뒤 단계다.
- **호출자에 기대는 spawn 의 위 한계 461**(r2_core): 그 짝 가운데 실제로 순서가 있는 짝은 정해지지 않았다 [미확인]. 풀린 entry 를 넘기지 않는 호출자 셋(`OS_TimeBaseCreate` 사슬 등)의 entry 를 푸는 것이 그 수를 줄이는 길이다.
- **c5 의 대기**: 6.4.x 의 대기 다섯은 flag·value 규칙에 맞지 않는다(refused-unresolved 3, refused-address-taken 1, not-examined 1). 그 꼴(주소를 넘겨 채우는 지역, 여러 정의)을 받을지는 뒤 단계다.
- 앞 단계에서 넘어온 열린 것(event type 의 등급, open 효과, c5 의 child without success, J?, 이름 결합)은 그대로다.
- **마지막 facts**: `ll_final_extract` 가 뒤 단계의 입력이다(장부 `OBS_ll_final_extract`). 계획 생성기의 다음 판(manifest 개정 4)은 이 facts 를 읽는다.
- **2026-11-18 검토 뒤 정정: §12.7 `semantics-5`, `semantics-6`, `semantics-8`, `semantics-10`.** 이 절이 적지 않은 한계(앞판 도구): flag 의 몸 안 정의가 switch 의 case 아래이면 술어 `1` 의 recognised 가 나왔다(실제 빌드의 인식된 루프 셋은 그 꼴이 아님); RESULT_FLOW 의 guard 행은 주소가 잡힌 지역의 정의를 하나뿐인 도달 정의로 적었고, switch 의 case 아래의 호출에는 guard 행이 없었으며, 같은 callee 의 두 호출을 한 값으로 읽었다(c1_pre_tx·c5_641 에서 영향받은 guard 행 0: 검토의 집계); 핸들의 대체 규칙은 잠금 연산이 아닌 호출의 인자를 핸들로 적었다. 코드로 고쳤다(§5w); call-result 마디에 site 를 싣는 것은 하지 않았다(§12.8).

### 10.9 세션 재시작 뒤 확인 [M]

이 절의 일을 한 세션이 결과를 돌려주기 직전에 다시 시작되었다. 일을 다시 하지 않고 기록과 디스크를 대조했다(`../records/loop-lock-20261116/verify_after_restart.txt`; 앞의 점검 출력은 그대로 두고 새 파일 이름에 씀).

- MC 의 바이너리 일곱과 `CMakeCache.txt` 가 `post_mc.txt` 의 sha256 과 같다(kz-static `9eb1395f…`). `kz-static --identity` 가 `identity_kz-static.txt` 와 같다. 사본 `kz-static.499748bc.xz` 는 `xz -t` OK, sha256 `de44cdf4…` 로 `tool_copy/ROUNDTRIP.txt` 와 같다.
- 출력 디렉터리 아홉(`ll_*`)이 `ll_*.sha256` 과 모두 같고 파일 수·바이트가 §10.6 의 장부 행과 같다.
- `commands.txt` 의 실행과 비교가 모두 rc 0 이고 각 출력(`*_run.txt`, `compare_*.txt`, `compare/` 71 파일, 공간 감시 기록 `df_*.log` 모두 `verdict=completed`)이 있다. §10.3–10.6 의 수를 `ctest_*.log`·`compare/measure_<b>.txt` 와 대조해 다른 것이 없었다.
- 다시 돌린 점검: `ledger_check_verify.txt` 124 행 실패 0, `tools_check_verify.txt` 45 행 실패 0(deep).
- 빠졌던 것: 기록 디렉터리 `README.md` 의 표가 빌드 전 파일까지만 있었다. 빌드 뒤 파일의 줄과 이 확인의 줄을 더했다.

## 11. 관측 계획의 manifest 개정 4 와 생성기 개정 3: `kzs-sites 4`, 경로 A 대조 선택, 값 요청, 대기 규칙과 객체 규칙 (kz-static·kz·kz-rt-dump·kz-instr-clang 빌드 하나)

호스트 파서가 `kzs-sites 4`(계획 머리, site 키, 행 키, 호출의 열과 차례, load 지점, need·gap 문, spawn 묶음, `decoder=spawn-token`)를 읽게 하고, 판 1–3 은 앞과 똑같이 읽게 둔다. 계획 생성기에 `--manifest 4` 로 고르는 개정 3 을 더한다: lead 의 판단 §6.10 의 2 대로 경로 A 의 대조 행을 행 id 가 아니라 규칙으로 고르고(규칙 글은 어느 계획보다 먼저 `037c047` 로 커밋함) 층 차례와 따로 기록된 예산 아래 계획하며, 값 요청을 따로 적고 그 sha256 을 sidecar 에 둔다. 대기 루프의 조건 읽기와 나가는 문장, 경로 C 읽기의 load·첨자 호출·잠금 호출, 이름 공간 자료가 이름 대는 객체 종류의 삭제·회수 자리(판정 규칙 8.2 (마)), 효과 호출의 의미 역할 인자를 사실과 자료에서 계획한다. `--manifest` 없이(또는 `3`) 쓰면 개정 2 의 출력 그대로다. 기록: `../records/manifest-v4-20261116/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `mv_*`. 도구 사본: 새 디렉터리 `/home/user/work/preserve/tools-20261116-l/`. cFS 를 부팅하지 않는다. 파생 빌드와 플러그인 빌드를 하지 않는다. push 하지 않는다.

### 11.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다.

- **무엇을 하는가** (`kairos/docs/plan_manifest.md` §7–§8, `kairos/docs/event_semantics.md` §3 의 판 4 단락):
  1. **파서 `kzs-sites 4`**(`runtime/tools/manifest/kz_rt_manifest.{hpp,cpp}`): `plan <id> facts= relrules= obsrules= semantics= select= budget= [control=] [ref=]`(manifest 에 하나, 그 파일의 첫 site 앞), site 의 `skey=`(16 hex, 유일)·`row=`·`col=`·`nth=`(callee= 가 있을 때)·`field=`(`place=load` 에만), `place=load`(`field=`·`line=` 필요, callee 없음, `phase=point`), `need`·`gap` 문(planned 는 선언된 sites=, not-planned 는 reason=), `spawn <site> wrapper= start=arg<k> arg=arg<k> [out=arg<k>]`(callee 가 있는 at-call site 에만), `identity … decoder=spawn-token`(그 site 의 spawn 문 뒤, 슬롯 없음). 판 1–3 파일의 받음과 거부는 앞과 같고, 판 4 낱말은 판 1–3 에서 `… needs 'kzs-sites 4'` 로 거부된다. 머리 `kzs-sites 5` 는 거부된다(`… or 'kzs-sites 4'`).
  2. **생성기 개정 3**(`lib/llvm/plan_gen.{hpp,cpp}`, `tools/kz-static/{cmd_plan.cpp,main.cpp}`; `plan rules revision 3 (2026-11-16)`): `--manifest 4 --control-budget all|rows:<n> --control-budget-reason TEXT --requests-out FILE [--namespaces F]…`. 개정 2 의 선택·차례·예산·관측 규칙은 그대로 쓰고, 그 위에 (가) manifest `kzs-sites 4`(plan 문, skey·row·col·nth, 한 줄의 같은 callee 두 호출은 gap 이 아니라 두 site), (나) 경로 A 대조 선택(§7 의 규칙 글; sidecar 의 `control-budget` 줄이 첫 row 줄 앞, row 줄의 `control=`·`control-state=`), (다) 계획된 경로 A 행마다 `creator:` 줄·`not a pair: key constants differ` 줄·(probe-create 행이면) `not a pair: probe-create` 줄의 생성 호출을 creator-call site 로(8.12 (ㄴ)(ㅁ)), (라) 대기 규칙 `wait-cond`·`wait-exit`(행의 site 가 있는 함수에서 그 site 앞의 poll·count-bounded 루프; FACT_LOOPS 나 LOOP_DEFS 가 술어를 알아볼 때만 조건이 읽는 호출 결과의 after-call, 읽는 필드의 load, break·return·goto 출구의 line-start; 조건 출구는 문장이 아니라 gap; 아니면 `wait-result … not-planned reason=plan_missing(wait predicate not recognised)`), (마) 객체 규칙: 경로 C 의 `read-load`·`object-index`(OBJECT_ACCESS 의 index root, 없으면 READS 의 것의 호출 결과 첨자)·`lock-call`(생성자와 읽는 함수에서 LOCK_OPS 연산의 호출), 경로 A 의 `object-delete`·`object-retire`(`--namespaces` 로 준 이름 공간 자료에서 행의 조회·생성 callee 가 lookup·create·copy·create-check 인 항목의 delete 함수의 입구와 그 retire 호출의 after-call, clear 의 store; 항목이 빌드에 적용되는지는 자료의 file sha256 과 facts 의 FILE_HASHES·정의 줄로), (바) `effect-call`(RESULT_EFFECT 의 event 보내기·syslog·응용 종료·terminal 효과의 호출 at-call, SEMANTICS arg-role 이 이름 대는 인자), (사) 값 요청 파일 `kz-plan-requests 1`(handling label T?·J?·F 이거나 absence 줄에 open 효과가 있는 계획된 경로 A·B 행: after-call 의 결과·out, 효과 호출과 역할 인자, 분기는 site 종류가 아니어서 not-planned), (아) 열린 규칙의 이름 댄 not-planned(경로 A·B 의 잠금 호출, 분기, 호출 없는 효과). sidecar 머리 `kz-plan-sidecar 2`, 출력 경로를 적지 않음(값 요청 파일은 이름과 sha256 만). 생성기는 행 id·키·사례 값·API 이름을 받지 않는다; spawn 묶음의 래퍼 이름 `kz_rt_posix_create` 는 이 저장소 런타임의 함수 이름이고 그 런타임 단계가 만든다 [I].
  3. **경로 A 대조 선택 규칙**(control selection rule revision 1, `plan_manifest.md` §7, 커밋 `037c047`, 이 선언보다 앞, 어느 계획보다 앞): (가) `handling: probe-create` 줄이 있는 경로 A 행, (나) 생성자 site(`creator:` 줄과 `not a pair: key constants differ` 줄이 이름 대는 생성 호출)가 하나라도 있는 경로 A 행; (가) 다음 (나), relations 글의 차례; 예산 `all` 또는 `rows:<n>`, 줄 `control-budget value= reason= rows= planned= cut= complete=yes|no`. `complete=no` 인 계획의 부팅은 8.12 (ㄷ)의 사전 시험이 아니다.
  4. **`env/tools/plan_regen.sh`**: `--requests-out` 이 있는 args 면 값 요청 파일을 scratch 에 같은 파일 이름으로 다시 만들어 바이트로 견주고 원본에 쓰지 않는다(고치지 않으면 다시 만들기가 남긴 출력을 덮음). 단위 점검 `env/tools/tests/run_unit.sh` 의 점검 17(가짜 생성기).
- **1차 근거.**
  - lead 의 판단 §6.10 의 2(대조 선택을 규칙으로, 층 차례와 따로, 예산은 생성 전에 sidecar 에, 규칙 글은 어느 계획보다 먼저 커밋, 생성기는 행 id·키·사례 값을 받지 않음)와 §6.10 의 4(키 상수 배제로 빠진 생성자 site 의 계획은 manifest 개정 4 의 생성기).
  - 판정 규칙 8.12 (ㄷ)(사전 시험의 조건), (ㄴ)(존재 탐침 행의 c), (ㅁ)(`not a pair: key constants differ` 의 생성 호출도 행의 생성자 site), 8.2 (마)와 8.7 의 표(“삭제·회수 site 의 계획 … 관측 계획 생성기의 다음 판(객체 규칙: 생성·삭제 연산을 자료에서 읽음)”).
  - 개정 2 의 계획에서 경로 A 의 계획된 행은 c1_pre_tx 의 A21 하나다(`/home/user/work/redesign/obs1/ll_plans_final/<b>/plan.side` 의 `state=planned` 행; §6.7 `rules-04`).
  - 삭제 자리의 자료: `kairos/semantics/framework_namespaces_osal-f34759d.txt` 의 `kind OS_OBJECT_TYPE_OS_COUNTSEM` 항목 `delete OS_CountSemDelete def=osal/src/os/shared/src/osapi-countsem.c:118 at=…:129`(그 줄은 `OS_ObjectIdFinalizeDelete(return_code, &token)`, `/home/user/work/procrace/c1_build/bundle_1df262a/osal/src/os/shared/src/osapi-countsem.c:129`), OSAL 4.1.1 판의 `table … clear …` 줄들(`framework_namespaces_osal-4.1.1.txt`).
  - 생성 토큰을 위한 생성 호출의 래퍼 묶음과 `decoder=spawn-token` 은 런타임·플러그인 단계가 읽을 형식이다; 이 단계는 형식과 생성만 하고, kzgcc 0.3 은 `kzs-sites 4` 를 읽지 않는다 [I].
- **코드**(`kairos/`): `runtime/tools/manifest/kz_rt_manifest.{hpp,cpp}`, `runtime/tests/test_rt_manifest.cpp`, `lib/llvm/plan_gen.{hpp,cpp}`, `tools/kz-static/{cmd_plan.cpp,main.cpp}`, `tests/static/{test_static_plan.cpp,CMakeLists.txt}`, 새 fixture `tests/static/fixtures/planv4.c`, 손으로 쓴 시험 자료 `tests/static/data/planv4_framework.txt`, `tests/static/data/planv4_fixture/{framework_semantics_fixture-plan.txt,framework_namespaces_fixture-plan.txt}`(생성된 자료가 아님을 머리에 적음), 문서 `docs/plan_manifest.md` §7(커밋 `037c047`)·§8, `docs/event_semantics.md` §3 의 판 4 단락. 저장소의 그 밖: `env/tools/plan_regen.sh`, `env/tools/tests/run_unit.sh`(점검 17).
  - 하지 않는 것: `include/`, `cmake/`, 최상위 `CMakeLists.txt`, `runtime/src`·`runtime/include`(런타임 라이브러리), `instrument/`(플러그인과 kz-instr-clang 의 소스), `lib/llvm` 의 추출·relations·order 규칙, `semantics/` 의 자료와 coverage rule, 판정 규칙과 어휘 자료, 설명 스크립트, `one_build.sh`.
- **도구 해시와 정체성**(빌드 전 `pre_mc.txt`):
  - kz-static: 파일 sha256(빌드 전 `9eb1395f3bd02fb4b4dc9b8be72b4e336845c3715481b0e38398b0dbd40f9452`), `--identity` 의 `tool_sha256`(빌드 전 `0e1d8599…`)·`tool_source`(빌드 전 `a2365b19…`)가 바뀐다.
  - kz: 파일(빌드 전 `913fcd99…`)과 `--identity` 의 `tool_sha256`(빌드 전 `c19937fd…`)·`tool_source`(빌드 전 `fb7f09f9…`)가 바뀐다(`runtime/` 의 변경, `lib/trace/CMakeLists.txt:26`).
  - kz-rt-dump(빌드 전 `4390d4f1…`), kz-instr-clang(빌드 전 `28db4017…`): 다시 링크되어 파일 sha256 이 바뀐다(`runtime/CMakeLists.txt:99`, `instrument/clang/CMakeLists.txt:7` 이 kzrt-manifest 를 링크). kz-hold 는 정적 라이브러리 `probe/hold/libkz-hold.a`(`rt_plan.cpp` 가 manifest 머리를 읽음)로 다시 만들어진다; 그것만 쓰는 실행 파일은 MC 에 없고 kz 와 시험이 링크한다.
  - `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 는 그대로 [I]. kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz_lint `3d4d5a8f…` 는 그대로. 플러그인 둘(`56a96b79`, `27a49a6a`)은 다시 빌드하지 않는다.
  - 다시 링크되는 것 50(`relink_expected.sha256`, 빌드 전 sha256; 바뀔 수 있음): kzrt-manifest·kz-llvm·kz-trace·kz-hold 정적 라이브러리와 그것을 링크하는 실행 파일 모두(링크 파일 `link.txt` 에서 셈: kz-static, kz, kz-rt-dump, kz-instr-clang, `test_rt_manifest`, `tests/cfs-free` 의 시험 31, `tests/static` 의 `test_static_{elf32,expr,flow,gitrev,link,plan,reasons,recipe,unroll,wrappers}`). 반드시 바뀌는 것: kz-static, kz, kz-rt-dump, kz-instr-clang, `test_rt_manifest`, `test_static_plan`, `test_static_reasons`, 네 라이브러리 [I].
  - 다시 링크되지 않아야 하는 것 74(`not_relink.sha256`, 빌드 전 sha256: 앞 단계 목록 124 에서 위 50 을 뺀 것; kz-dump, kz-phi, kz_lint, `static_golden`, `test_static_cli` 포함). 빌드 뒤 같은 sha256 이어야 한다. `CMakeCache.txt` `bc2f43b4…` 그대로 [I](`tests/static/CMakeLists.txt` 가 바뀌어 다시 구성된다).
- **빌드**(MC `/home/user/work/merge-check/kz-llvm` 제자리, 목표별, 각각 공간 감시 아래): `make kz-static`, `make all`. 감시가 도중에 멈추면 적고, 도구가 새 `--identity` 를 적을 때까지 출력을 내지 않는다.
- **도구 보존**(PI 결정: in-place MC 재빌드는 사본을 압축하고 왕복 확인한 뒤 중간 도구를 덮을 수 있다; 결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함): 빌드 전에 kz-static `9eb1395f`, kz `913fcd99`, kz-instr-clang `28db4017` 를 새 디렉터리 `/home/user/work/preserve/tools-20261116-l/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 공간 감시 아래; 푸는 바이너리는 파이프로만). kz-rt-dump `4390d4f1` 는 이미 사본이 있다(`/home/user/work/preserve/tools-20261109/kz-rt-dump.4390d4f1.xz`, 등록부 in-place+copy). 사본 직후 등록부의 세 행을 in-place+copy 로, 빌드 뒤 네 행을 copy 로 고치고 새 도구 넷을 in-place 로 더한 다음 빌드 뒤 등록부 점검을 돌린다. 장부: 사본 디렉터리 행 `TOOLS_20261116l`.
- **golden**(빌드 전 scratchpad 점검 빌드로 미리 만듦 [I, 근거 아님]; `golden_scratch.sha256`: `planv4_np.plan.golden` `dd26c7dd…`): 새 golden 하나. **첫 ctest 에서 실패는 하나여야 한다: `test_static_plan`**(golden 파일이 아직 없음). `KZ_GOLDEN_UPDATE=1` 로 그 시험만 돌려 쓴 뒤 scratchpad 의 것과 `cmp` 같아야 하고, 그 밖의 golden 은 바뀌지 않는다(`git status`; scratchpad 점검에서 `plangen_np.plan.golden` 이 새 도구로 같은 바이트였음 [I]).
- **시험**: 전체 ctest 267 → 268(새: `static_fixture_compile_planv4`), golden 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`). 첫 ctest 와 golden 뒤 ctest 를 `-V` 로 돌려 확인 수를 남긴 출력에서 센다: `test_static_plan` golden 뒤 195 → 275 [I], `test_static_reasons` 731 → 751 [I](생성기의 사유 표에 다섯 줄), `test_static_cli` 1,151, `test_core_schema` 34,017, `lint_selftest` 114, lint 규칙 일곱 clean; `rt_manifest` 의 요약 줄에 “version 4: a plan with 4 sites, 2 needs, 1 gap and a spawn binding accepted, 38 rejected” [I]. 단위 점검 `env/tools/tests/run_unit.sh` 16 → 17/17, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤와 실행 뒤 각각 `env/tools/ledger_check.sh`(빌드 전 124 행 실패 0: `ledger_check_pre.txt`)와 `env/tools/tools_check.sh --deep`(빌드 전 45 행 실패 0: `tools_check_pre.txt`), 출력은 단계마다 새 이름.
- **판 1–3 과 옛 계획의 회귀**(모두 새 디렉터리; `compare.sh`):
  - **파싱**(`compare.sh manifest`): `regress/build_printers.sh` 가 연구용 점검 프로그램 `regress/manifest_print.cpp` 를 두 번 컴파일한다(감시 아래, scratchpad): 커밋 `d56ca0d` 의 파서(git show)와 작업 트리의 파서. 두 프로그램을 같은 묶음 129 개(저장소의 `.kzs` 22, 앞 단계들의 생성된 계획 105, 파생 빌드가 묶은 계획과 그 binding 목록 둘)에 돌려(`regress/regress.sh`, `regress/groups.sh`) 판 1–3 의 모든 칸과 오류 문장이 바이트로 같아야 한다 [I: scratchpad 에서 129/129, 혼자 읽으면 오류인 binding 목록 넷도 같은 오류].
  - **유도되는 묶음**(`regress_tools.sh`, `compare.sh tools`): 빌드 전의 MC 도구로 `mv_regress_tools_old`, 빌드 뒤의 도구로 `mv_regress_tools_new` 를 만든다: C1 부팅 dump 의 `kz-rt-dump --manifest`(첫 관측 경로의 C1 계획 `plangen/c1_pre_tx` 와 파생 빌드의 binding 목록)·`kz convert`(같은 manifest 와 B′ basis)·`kz-dump`, C5 6.4.1 부팅 dump 의 같은 셋(C5 프로필의 draft manifest 와 binding 목록). `compare_tools.py`: `rc.txt` 의 첫 줄(도구 해시)과 kz-dump 글의 `tool_sha256=` 줄만 다를 수 있다.
  - **기본 경로의 계획**(`plans.sh t8cf mv_plans_v3_t8cf`, `plans.sh <ll_final_extract> mv_plans_v3_final`, `compare.sh plans_v3`): `compare_plans.py t8cf` 로 `ll_plans`·`ll_plans_final` 과(같은 facts): 생성기 정체성 줄, sidecar sha256 줄, module 표·실행 파일 목록의 첫 줄, args.txt 의 출력 디렉터리만.
- **개정 3 의 계획**(facts: 마지막 옵션 회차 `ll_final_extract`, 장부 `OBS_ll_final_extract`):
  - `plans.sh <ll_final_extract> mv_plans_v4 v4`: 다섯 빌드, 층 예산 `tier:D07`(앞 계획과 같은 이유·공개), 대조 예산 `all`(이유: §6.10 의 2, 8.12 (ㄷ); 사건 양은 아직 재지 않음), `kairos/semantics/framework_namespaces_*.txt` 다섯, 값 요청 `plan.req`. 다시 만들기(`compare.sh regen`, `plan_regen.sh`)가 manifest·sidecar·값 요청을 바이트로 다시 내야 한다. 새 파서로 판 4 로 읽혀야 한다(`compare.sh parse_v4`).
  - `plans.sh <ll_final_extract> mv_plans_v4_all v4all`: 같은 인자에 층 예산 `all`(부팅용 계획이 아닌 측정; 경로 C 규칙이 실제 빌드에서 무엇을 내는지).
- **측정**(`compare.sh measure`, `measure_v4.py`; 모든 수는 `compare/measure_<b>.txt` 에서): 빌드마다 경로 A 행 수, 대조 집합(a, b, a+b), 계획된 대조 행, 층·대조·어느 쪽으로든 계획된 경로 A 행, 그 가운데 관계 1(사용 지점 키 `17975444d83f6589`) 밖의 행, probe-create 행과 그 상태, 개정 2 계획의 경로 A 계획된 행, 규칙별 site 수, need(종류·상태)와 gap, 값 요청, lifetime 줄, 그리고 측정 계획의 같은 수. 측정이고 판정이 아니다.
  - [I](scratchpad 점검 빌드, LLVM Release 구성, 감시 아래; 근거 아님, 지움; 순서 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642): 경로 A 행 63/63/25/66/66; 대조 집합 5/5/4/15/15(a+b 1/1/0/0/0, b 4/4/4/15/15), 모두 계획됨(`complete=yes`); 어느 쪽으로든 계획된 경로 A 행 5/5/4/15/15, 관계 1 밖 4/4/4/15/15(개정 2: 경로 A 계획된 행 1/0/0/0/0); C1 의 A20(probe-create, 키 `35f07539e7606e2e`·c1_post_tx `bdcc6978b9a41c8c`)은 층에서 잘리고 대조로 계획됨; 관계 1(A21)은 c1_pre_tx 에서 층과 대조 모두, c1_post_tx 에서 대조로 계획됨. site 291/291/65/201/193(개정 2: 267/253/42/147/139). c1 두 빌드의 `object-delete` 는 `OS_CountSemDelete` 의 입구와 `osapi-countsem.c:129` 의 retire 호출(관계 1·A20), c5 두 빌드는 맞는 항목이 없음(15 행 not-planned); c1_post_tx 의 관계 1 에 CF 재시도 대기(`cf_cfdp.c:1001:13`, LOOP_DEFS value)의 `wait-result` planned(조회의 after-call 과 break 줄), 그 밖의 대기 need 는 not-planned. 측정 계획(층 예산 all)에서 경로 C 의 `read-load` 43/43/41/9/8, c5 두 빌드의 `read-subscript` planned 32.
- **공간**: 각 빌드·사본·실행은 감시 아래(`env/tools/space_guard.sh`, 2 초마다, 하한 314,572,800). 빌드 필요량 = 앞 kz-static 빌드 dip 의 최대 256,942,080 + 새 출력 약 30 MB(계획 넷 약 12 MB, 회귀 출력 약 4 MB, 여유) + 다시 링크되는 큰 실행 파일 셋(kz, kz-instr-clang, kz-rt-dump)의 임시 쓰기 약 180 MB + 한 시간 흐름 13 MB = 479,942,080 B. 사본은 필요량 200,000,000(도구 셋 약 386 MB 를 읽어 xz 약 80 MB 를 씀; 푸는 바이너리는 파이프로만). 실행 단계와 점검 프로그램의 컴파일은 필요량 100,000,000. 점검 빌드(scratchpad)는 감시 아래에서 했고(`scratch_guard/`, 가장 큰 dip 183,627,776) 빌드 전에 지운다(golden 사본만 남김).
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `plan_one.sh`, `plans.sh`, `compare_plans.py`(앞 단계의 것을 옮김), `regress_tools.sh`, `compare_tools.py`, `regress/{manifest_print.cpp,build_printers.sh,groups.sh,regress.sh}`, `measure_v4.py`, `compare.sh`, `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`, `golden_scratch.sha256`; 빌드 전 점검 `tools_check_pre.txt`, `ledger_check_pre.txt`.

### 11.0.1 덧붙임 1 (2026-11-18, 빌드 뒤·어느 계획 생성보다 먼저 커밋함) [D]

선언 11.0 은 그대로다(`declared_before_build.txt`, sha256 은 `commands.txt` 첫 줄). 이 덧붙임은 그 뒤에 생긴 것 넷을 적는다. 이 글을 쓰는 지금 다섯 빌드의 계획·회귀 비교·측정은 아직 하나도 돌리지 않았다.

1. **세션이 멈췄다가 다시 이어졌다.** 사본, 옛 도구의 회귀 실행(`mv_regress_tools_old`), `make kz-static`, `make all` 은 2026-11-16T23:49:23Z–23:58:33Z 에 끝났고(`commands.txt` 의 1·2 절, 감시 기록 `df_copy.log`·`df_regress_tools_old.log`·`df_build_kz-static.log`·`df_build_all.log` 모두 `verdict=completed rc=0`), 그 뒤의 것은 2026-11-18 에 한다. 빌드는 다시 하지 않는다. 다시 하지 않는 근거(2026-11-18 에 잼): `make -n all` 의 마른 실행에 컴파일·링크 줄이 0(`make_dryrun_after_build.log`), 다시 링크되는 것 50 의 mtime 이 모두 빌드 시작(2026-11-16T23:55:08Z) 뒤, 다시 링크되지 않아야 하는 74 는 sha256 그대로(`relink_result.txt`), `kz-static --identity`·`kz --identity` 가 새 값을 적음(`identity_kz-static.txt`, `identity_kz_post.txt`), `kairos/` 의 소스는 선언 커밋 `5011169` 뒤로 바뀌지 않음(`post_mc.txt` 의 uncommitted 0; `git diff 5011169 HEAD -- kairos` 빈 출력). 빌드 뒤 기록(`post_mc.txt`)의 시각과 HEAD 는 2026-11-18 의 것이다. 세션이 멈추기 전에 빌드 뒤에 돌린 것은 없다(`commands.txt` 가 `make all rc=0` 에서 끝남).

2. **실행의 차례.** `regress/groups.sh` 는 `/home/user/work/redesign/obs1/*/*/plan.kzs` 를 모두 묶음으로 넣는다. 선언의 묶음 수 129(저장소의 `.kzs` 22, 앞 단계들의 생성된 계획 105, binding 목록이 딸린 둘)는 이 단계의 계획이 생기기 전의 수다(2026-11-18 에 다시 셈: 105, 22). 그래서 파싱 회귀(`compare.sh manifest`)를 이 단계의 어느 계획보다 먼저 돌린다. 그 다음 차례: 새 도구의 유도 묶음(`mv_regress_tools_new`, `compare.sh tools`), 기본 경로의 계획 둘(`mv_plans_v3_t8cf`, `mv_plans_v3_final`, `compare.sh plans_v3`), 개정 3 의 계획 둘(`mv_plans_v4`, `mv_plans_v4_all`), `compare.sh regen`·`parse_v4`·`measure`, 아래 3 의 측정.

3. **더한 측정 하나: 경로별 분모**(도구를 바꾸지 않는다; 도구에 주는 입력도 없다). 스크립트 `measure_paths.py`·`measure_paths.sh`(이 덧붙임과 함께 커밋): 빌드마다, 같은 facts(마지막 옵션 회차)의 계획 넷(`ll_plans_final/<b>` 개정 2 의 남긴 계획, `mv_plans_v3_final/<b>` 이 단계 도구의 기본 경로, `mv_plans_v4/<b>`, `mv_plans_v4_all/<b>`)의 sidecar 에서 경로 A·B·C 마다 관계 행 수, 층 차례로 계획된 행(row 줄의 `state=planned`), 대조 선택으로 계획된 행(`control-state=planned`), 어느 쪽으로든 계획된 행, 어느 쪽으로도 계획되지 않은 행을 그 행 자신의 `need row <key> observation not-planned reason=…` 줄의 사유와 층·disposition·label 로 묶은 수, 그리고 모든 행의 경로 × label × 층 × order class 표를 낸다(`compare_paths.txt`, `compare/paths_<b>.txt`). 측정이고 판정이 아니다. 선택 규칙은 이 단계에서 바꾸지 않는다.
   - 코드에서 읽은 것(측정 전; `kairos/lib/llvm/plan_gen.cpp`): 경로 A 의 label 은 H·R·J·T?·E·0(:143–149), 경로 B 는 J·T?·E·0(:179–183), 경로 C 는 F·0·E 뿐이다(:211). 층 1 은 disposition D07 이고(:246) D07 은 label 이 R 이나 J 일 때만 선다(:234). 그러므로 경로 C 의 행은 어떤 facts 에서도 층 1 에 들 수 없고, order class 가 RELOC 이면 층 2, 아니면 층 3 이다(:246). 층 예산 `tier:D07` 은 층 1 까지만 계획한다(:381, :347). 대조 선택은 경로 A 행만 본다(:1335). 따라서 층 예산 `tier:D07` 과 대조 예산 `all` 의 계획에서 경로 C 의 계획된 행은 구조상 0 이고, 경로 B 는 대조로 계획되는 행이 0 이다 [I: 측정 전]. 경로 C 행이 계획되는 것은 층 예산 `tier:directed`(층 2 까지, :382)·`all`·`rows:<n>` 에서다.

4. **장부에 넣을 것**(실행 뒤, 끝 점검 전): 출력 디렉터리 일곱(`mv_regress_tools_old`, `mv_regress_tools_new`, `mv_regress_manifest`, `mv_plans_v3_t8cf`, `mv_plans_v3_final`, `mv_plans_v4`, `mv_plans_v4_all`)의 sha256 목록 `mv_*.sha256` 과 행 `OBS_mv_*`, 사본 디렉터리 행 `TOOLS_20261116l`. 파싱 회귀의 점검 프로그램 둘과 다시 만들기의 출력은 scratchpad 에 두며 근거가 아니다(그 비교의 글 `compare_manifest.txt`·`compare_regen.txt` 와 출력 디렉터리 `mv_regress_manifest` 가 근거다).

### 11.0.2 덧붙임 2 (2026-11-18, 선언 밖의 차이 하나를 밝히고 선언함; 다시 돌리기 전에 커밋함) [D]

**멈춘 곳.** `compare.sh tools`(유도되는 묶음의 회귀: 빌드 전 도구의 `mv_regress_tools_old` 와 빌드 뒤 도구의 `mv_regress_tools_new`)가 rc 1 로 끝났다(`compare_tools.txt`: 선언 밖의 차이 4 줄). 선언 11.0 은 "`rc.txt` 의 첫 줄(도구 해시)과 kz-dump 글의 `tool_sha256=` 줄만 다를 수 있다"고 적었다. 그 비교에서 멈추고 아래를 밝힌 뒤 이 덧붙임을 커밋했다. 앞의 출력(`compare_tools.txt`, `mv_regress_tools_new`)은 그대로 둔다.

**차이.** 네 줄 모두 컨테이너의 `artifact_sha256` 이다: `c1.run.kzt.txt` 의 3,823 째 줄과 `c5.run.kzt.txt` 의 2,721 째 줄(`artifact_sha256=…`), `c1.convert.log` 의 10 째 줄과 `c5.convert.log` 의 11 째 줄(`wrote <OUT>/<c>.run.kzt artifact_sha256 …`). 그 밖의 줄은 같다(`compare_tools.txt`: `c1.dump.txt` 1,082 줄·`c5.dump.txt` 1,190 줄은 차이 0, kz-dump 글 둘은 허용된 `tool_sha256=` 한 줄과 이 한 줄씩뿐).

**까닭**(코드). `artifact_sha256` 은 컨테이너 파일 전체의 sha256 이다(`kairos/lib/core/container_reader.cpp:136` `c.hash_ = sha256(b.data(), b.size())`, 찍는 곳 `lib/core/dump.cpp:230`). 컨테이너 머리에는 그것을 쓴 도구의 해시 `tool_sha256` 과 머리의 `header_crc` 가 들어 있다(`kairos/include/kz/schema/kz_schema.h:279-293`: magic 8 바이트, uint32 여섯, uint64 셋, `basis_sha256` 32, `profile_sha256` 32 다음의 `tool_sha256` 32 바이트가 파일 오프셋 120–151, `header_crc` 가 152–155). kz 의 `tool_sha256` 이 바뀌는 것은 선언한 변화이고(11.0 의 도구 해시와 정체성: `c19937fd…` → 빌드 뒤 `80064cc3…`, `identity_kz_post.txt`), 파일 전체의 해시는 그 바이트를 따라 바뀐다. 선언이 이 따름 결과를 허용 목록에 넣지 않은 것이 빠뜨림이다. 내용의 차이가 아니라는 것은 바이트로 잰다: 두 컨테이너는 크기가 같고(234,528 B, 149,872 B) `cmp -l` 이 내는 다른 바이트는 각각 36 개, 오프셋 120–155(1 부터 세어 121–156) 뿐이다(2026-11-18 에 잼; 아래 비교가 남긴 출력으로 다시 적는다).

**선언.** 유도되는 묶음의 회귀에서 다를 수 있는 것은 다음뿐이다: `rc.txt` 의 첫 줄; `c1.run.kzt.txt`·`c5.run.kzt.txt` 의 `tool_sha256=` 줄과 `artifact_sha256=` 줄; `c1.convert.log`·`c5.convert.log` 의 `wrote <OUT>/<c>.run.kzt artifact_sha256 …` 줄; 컨테이너 `c1.run.kzt`·`c5.run.kzt` 는 크기가 같고 다른 바이트가 모두 오프셋 120–155 안. 그 밖의 줄과 바이트는 같아야 한다. 비교 스크립트 `compare_tools_2.py`(이 덧붙임과 함께 커밋; `compare_tools.py` 는 그대로 둠)를 감시 아래 돌려 `compare_tools_2.txt` 에 남긴다. 도구도, 두 출력 디렉터리도 다시 만들지 않는다.

### 11.0.3 덧붙임 3 (2026-11-18, 점검 둘을 더함; 그것을 돌리기 전에 커밋함) [D]

이 글을 쓰는 지금 선언 11.0 과 덧붙임 1·2 의 실행은 모두 rc 0 으로 끝났다(`commands.txt` 4 절). 아래 둘은 선언에 없던 점검이고, 도구와 앞의 출력은 바꾸지 않는다.

1. **판 4 읽기의 요약 줄.** `compare.sh parse_v4` 는 rc 0 이고 계획 열 개가 모두 첫 줄 `version 4` 를 냈다(`compare_parse_v4.txt`). 그러나 그 단계는 점검 프로그램의 표준 출력을 `head -1` 에 잇는다. `head` 가 첫 줄 뒤에 관을 닫으므로 프로그램은 출력 도중에 끝나고, 맨 끝에 표준 오류로 내는 판 4 요약 줄(`v4 plan= needs= gaps= spawn= load= skey=`)에 이르지 못한다: 요약 파일 `compare/parse_<dir>_<b>.v4` 열 개 가운데 아홉이 비었다(r2_core 의 `mv_plans_v4` 하나만 줄이 있음). 점검 프로그램은 모든 입력을 다 읽은 뒤에야 첫 줄을 내므로(`regress/manifest_print.cpp`: `parse_manifest` 가 실패하면 `error …` 한 줄만 냄) "판 4 로 읽힌다"는 선언의 조건은 그 출력으로 섰지만, 요약은 남지 않았다. 스크립트의 결함이다. 새 스크립트 `parse_v4_full.sh`(이 덧붙임과 함께 커밋; `compare.sh` 는 그대로 둠)는 관을 끝까지 읽고, 요약 줄을 `compare/parsefull_<dir>_<b>.v4` 에 남기며, 그 수(need·gap·spawn 문, `place=load` site, `skey=` site, plan id)를 계획 글에서 `grep` 으로 센 수와 견준다(`compare_parse_v4_full.txt`). 열 개가 모두 같아야 한다.

2. **첫 관측 경로의 계획의 경로별 분모.** 덧붙임 1 의 3 과 같은 스크립트 `measure_paths.py` 를 첫 관측 경로 단계가 남긴 계획 다섯(`/home/user/work/redesign/obs1/plangen/<b>/plan.side`, 그 단계의 facts 와 층 예산 `tier:D07`)에 돌린다(`measure_paths_first.sh` → `compare_paths_first.txt`, `compare/paths_first_<b>.txt`). 그 계획의 계획된 행이 경로별로 몇이었는지를 같은 잣대로 적기 위해서다(그 단계의 결과 문서는 층 1 곧 D07 의 행 수를 12/11/3/8/7 로 적었다: `plan/observation_path_results_20261116.md` 의 층별 행 수 줄). 측정이고 판정이 아니다. 덧붙임 1 의 3 에서 코드로 읽은 것대로면 경로 C 의 층 차례 계획 행은 다섯 빌드 모두 0 이다 [I: 측정 전].

### 11.1 바뀐 것 [F]

- 커밋 차례: `037c047`(경로 A 대조 선택 규칙의 글, `kairos/docs/plan_manifest.md` §7; 어느 계획보다 먼저), `5011169`(선언 §11.0, 파서·생성기·fixture·시험·문서, 스크립트와 빌드 전 목록; 빌드 전), `dad3e06`(세션이 멈춘 때의 사본·빌드 기록 스냅숏), `a2c8dbc`(빌드 뒤 기록, 새 golden, 등록부 행, 덧붙임 1; 어느 계획 생성보다 먼저), `6474cd5`(파싱 회귀, 덧붙임 2; 다시 돌리기 전), `9ff7b95`(도구 비교의 다시 돌림, 다섯 빌드의 계획과 측정, 덧붙임 3; 그 점검 둘 전), 그리고 이 절의 커밋.
- 규모(`d56ca0d` 에서 `5011169` 까지, `kairos/` 와 `env/tools/`; `git diff --stat`): 17 파일 +1,786 −66. `lib/llvm/plan_gen.cpp` +822(개정 3: `kzs-sites 4` 출력, 대조 선택 `select_controls`, 생성자 site, 대기·객체·효과 규칙, 값 요청, sidecar 2), `plan_gen.hpp` +20, `runtime/tools/manifest/kz_rt_manifest.cpp` +195·`.hpp` +70(판 4 의 문과 칸), `runtime/tests/test_rt_manifest.cpp` +117, `tools/kz-static/cmd_plan.cpp` +39·`main.cpp` +3, `tests/static/test_static_plan.cpp` +192, fixture `planv4.c` 216 줄과 손으로 쓴 시험 자료 셋(32 줄), `docs/plan_manifest.md` +93, `docs/event_semantics.md` +1, `env/tools/plan_regen.sh` +13, `env/tools/tests/run_unit.sh` +22. golden `tests/static/data/planv4_np.plan.golden` 254 줄(`a2c8dbc`).
- 바꾸지 않은 것: `include/`, `cmake/`, 최상위 CMakeLists, `runtime/src`·`runtime/include`, `instrument/`, `lib/llvm` 의 추출·relations·order 규칙, `semantics/` 의 자료와 coverage rule, 판정 규칙과 어휘 자료, 설명 스크립트, `one_build.sh`. 층 차례의 규칙(label·order class·disposition·층, `plan_gen.cpp:113-246`)과 대조 선택 규칙(`:1331-1354`)은 `5011169` 뒤로 손대지 않았다.
- 생성기가 받는 옵션은 `--basis --framework --select --order --budget --budget-reason --budget-disclosure --manifest --control-budget --control-budget-reason --namespaces --requests-out` 와 출력 경로 넷뿐이다(`tools/kz-static/cmd_plan.cpp`, 계획의 `args.txt`). 행 id·키·사례 값을 받는 옵션이 없다.

### 11.2 빌드 [M]

MC 에서 제자리, 목표별, 각각 공간 감시 아래(필요량 479,942,080). 빌드 전 HEAD `5011169`(`pre_mc.txt`), kairos 아래 커밋 안 된 줄 0(`pre_mc.txt`, `post_mc.txt`). 사본·옛 도구의 회귀 실행·빌드는 2026-11-16 에, 그 뒤의 것은 세션이 멈췄다가 이어진 2026-11-18 에 했다(덧붙임 1). 빌드는 다시 하지 않았다: `make -n all` 의 마른 실행에 컴파일·링크 줄이 0 이다(`make_dryrun_after_build.log`; `make -q all` 은 rc 1 인데, 마른 실행이 이름 대는 명령은 echo·depend 줄과 `cmake -P VerifyGlobs.cmake` 하나뿐).

| 일 | 때 (UTC) | dip (B) |
|---|---|---|
| 사본 셋 (`df_copy.log`) | 11-16 23:49:23–23:52:49 | 85,532,672 |
| 옛 도구의 회귀 실행 (`df_regress_tools_old.log`) | 11-16 23:54:59–23:55:01 | 1,490,944 |
| `make kz-static` (`df_build_kz-static.log`) | 11-16 23:55:08–23:55:52 | 31,473,664 |
| `make all` (`df_build_all.log`) | 11-16 23:55:56–23:58:33 | 55,345,152 |

- 다시 링크된다고 적은 50 은 모두 빌드 시작 뒤의 mtime 이고, 16 이 바뀌었으며 34 는 같은 바이트로 다시 링크되었다(`relink_result.txt`). 바뀐 16: 선언이 반드시 바뀐다고 적은 11(kz-static, kz, kz-rt-dump, kz-instr-clang, `test_rt_manifest`, `test_static_plan`, `test_static_reasons`, 정적 라이브러리 넷)과 `tests/cfs-free` 의 `test_kzrt_app_request`·`test_kzrt_identity`·`test_kzrt_recovered`·`test_neutral_kzrt`·`test_rt_plan_convert`. `not_relink.sha256` 74 모두 같은 바이트.
- 도구(`post_mc.txt`, `identity_kz-static.txt`, `identity_kz_post.txt`): kz-static `9eb1395f…` → `1a47abd05b8289bce90e6da8906d3b84d9bec48befc99634295d5350cd261086`(`--identity` 의 `tool_sha256` `0e1d8599…` → `b6a64d83…`, `tool_source` `a2365b19…` → `88806cfe…`); kz `913fcd99…` → `fd40bc4dc76939abbd3a03ae1d3a6fbe5d387f49625e8b74f9066d13e0c140fe`(`tool_sha256` `c19937fd…` → `80064cc3…`, `tool_source` `fb7f09f9…` → `500185a6…`); kz-rt-dump `4390d4f1…` → `40e23d4db230806c08742f7a33da5abda67dce7eb6e0f2d81532d890cb859ddc`; kz-instr-clang `28db4017…` → `36becd040abec0b0bd302d59ae26b88252ad1e7c65ad1fb26cc31c5da86343f6`. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz_lint `3d4d5a8f…`, `CMakeCache.txt` `bc2f43b4…` 는 그대로다. 플러그인은 빌드하지 않았다.
- 도구 보존: 빌드 전 kz-static `9eb1395f`(46,996,316 B)·kz `913fcd99`(17,780,736 B)·kz-instr-clang `28db4017`(20,725,428 B)를 `/home/user/work/preserve/tools-20261116-l/` 에 xz 로 사본하고 왕복 확인(`tool_copy/ROUNDTRIP.txt`: 셋 다 xz -t OK, 푼 sha256 같음, cmp 같음; 푼 것은 파이프로만). kz-rt-dump `4390d4f1` 은 앞선 사본이 있다. 등록부: 사본 직후 세 행 in-place+copy(`tools_check_after_copy.txt` 45 행 실패 0, deep), 빌드 뒤 덮인 네 행을 copy 로, 새 도구 넷을 in-place 로(`tools_check_after_build.txt` 49 행 실패 0, deep).

### 11.3 시험 [M]

- 첫 ctest(`ctest_first.log`, `-V`): 268 개 가운데 실패 1, 선언한 그 하나(`test_static_plan`: golden 없음, “1 of 275 checks FAILED”). 건너뜀 1(`test_probe_anchor_reached`).
- `KZ_GOLDEN_UPDATE=1` 로 그 시험만 돌려 golden 을 썼다(`golden_update.log`). scratchpad 점검 빌드의 것과 `cmp` 같다(`dd26c7dd…`, `golden_scratch.sha256`; `commands.txt`). 다른 golden 은 바뀌지 않았다(`git status` 에 새 파일 하나뿐).
- 갱신 뒤 전체 ctest(`ctest_full.log`, `-V`): 268 개, 실패 0, 건너뜀 1.
- 확인 수(`ctest_counts.log`, golden 뒤): `test_static_plan` 275, `test_static_reasons` 751, `test_static_cli` 1,151, `test_core_schema` 34,017, `lint_selftest` 114 경우 실패 0, lint 규칙 일곱 clean. `rt_manifest` 의 요약 줄: “version 4: a plan with 4 sites, 2 needs, 1 gap and a spawn binding accepted, 38 rejected”(판 1–3 의 수는 앞과 같은 줄: 18 malformed rejected; version 2 의 8 accepted·18 rejected, 5 placed·7 rejected; version 3 의 4 accepted·13 rejected). 모두 선언의 [I] 와 같다.
- 단위 점검 `run_unit.sh` 17/17(`unit_tools.txt`; 점검 17 은 값 요청 파일이 있는 계획의 다시 만들기), `step2a_tests_unit.sh` 7/7(`unit_step2a.txt`).

### 11.4 바이트 동일성과 회귀 [M]

| 비교 | 결과 |
|---|---|
| 판 1–3 의 파싱, 옛 파서 대 새 파서 (`compare_manifest.txt`, `mv_regress_manifest/compare.txt`) | 묶음 129, 같음 129, 다름 0 (옛 파서가 오류로 읽는 묶음 4 도 같은 오류 글) |
| 유도되는 묶음, 옛 도구 대 새 도구, 첫 비교 (`compare_tools.txt`) | **rc 1: 선언 밖 4 줄** → 멈춤, 덧붙임 2 |
| 같은 비교, 덧붙임 2 뒤 (`compare_tools_2.txt`) | 아홉 줄 모두 OK: 글 일곱은 선언한 줄만, 컨테이너 둘은 다른 바이트 36(오프셋 120–155)뿐 |
| 기본 경로의 계획, T8CF facts 와 마지막 옵션 회차 (`compare_plans_v3.txt`) | 열 모두 "only declared differences" |
| 개정 3 계획의 다시 만들기 (`compare_regen.txt`) | 다섯 모두 PASS: manifest·sidecar·값 요청이 같은 sha256, module 표·실행 파일 목록 같음 |
| 개정 3 계획의 판 4 읽기 (`compare_parse_v4.txt`, `compare_parse_v4_full.txt`) | 열 모두 `version 4`; 끝까지 읽은 요약의 수가 계획 글에서 센 수와 열 모두 같음 |

- **판 1–3 은 앞과 똑같이 읽힌다.** 점검 프로그램 둘(커밋 `d56ca0d` 의 파서와 작업 트리의 파서; 소스 sha256 `printers_sources.sha256`)이 저장소의 `.kzs` 22, 앞 단계들의 생성된 계획 105, binding 목록이 딸린 계획 둘에서 모든 칸과 오류 문장을 같은 바이트로 낸다. 옛 파서가 오류로 읽는 넷은 혼자 읽으면 site 선언이 없는 binding·identity 목록이다(`instrument/clang/out/{c1,r2}/binds.kzs`, `instrument/gcc/sites/{c1_post_1df262a,r2_v701}.identity.kzs`).
- **옛 계획에서 유도되는 묶음은 같다.** C1 부팅 dump 와 C5 6.4.1 부팅 dump 를 남긴 판 3 계획과 binding 목록으로 다시 읽은 `kz-rt-dump --manifest` 의 글(1,082 줄, 1,190 줄)은 옛 도구와 새 도구에서 차이 0 이다. `kz convert` 의 컨테이너를 kz-dump 로 푼 글(3,824 줄, 2,722 줄)은 `tool_sha256=` 줄과 `artifact_sha256=` 줄만 다르고, 컨테이너 자체는 크기가 같고(234,528 B, 149,872 B) 머리의 `tool_sha256`·`header_crc`(오프셋 120–155) 36 바이트만 다르다.
- **멈춘 곳 하나(덧붙임 2).** 첫 비교는 `artifact_sha256` 네 줄에서 rc 1 로 멈췄다. 그 값은 컨테이너 파일 전체의 sha256 이고(`kairos/lib/core/container_reader.cpp:136`) 머리에 도구 해시가 들어 있으므로(`kairos/include/kz/schema/kz_schema.h:279-293`), 선언한 도구 해시의 변화를 따라 바뀐다. 선언이 이 따름 결과를 허용 목록에 넣지 않았다. 까닭을 밝히고 덧붙임 2 를 커밋한(`6474cd5`) 뒤 새 스크립트로 다시 견주었다. 첫 출력은 그대로 남겼다.
- **기본 경로(`--manifest` 없음)는 개정 2 의 출력 그대로다.** 다른 것은 생성기 정체성 줄, sidecar sha256 줄, module 표·실행 파일 목록의 첫 줄뿐이다(빌드마다 `plan.kzs`·`plan.side`·`plan.mod`·`plan.exe` 한 줄씩).
- **누적 목록과 등록부**: 빌드 전 `ledger_check_pre.txt` 124 행·`tools_check_pre.txt` 45 행 실패 0; 빌드 뒤 `ledger_check_after_build.txt` 124 행, `tools_check_after_build.txt` 49 행 실패 0(deep); 장부 행 여덟을 더한 뒤 `ledger_check_end.txt` 132 행 실패 0(새 행 `OBS_mv_regress_tools_old`, `OBS_mv_regress_tools_new`, `OBS_mv_regress_manifest`, `OBS_mv_plans_v3_t8cf`, `OBS_mv_plans_v3_final`, `OBS_mv_plans_v4`, `OBS_mv_plans_v4_all`, `TOOLS_20261116l`), `tools_check_end.txt` 49 행 실패 0(deep).
- 바깥 이름 점검(`--tree`)은 출력 디렉터리 일곱, 사본 디렉터리, 기록 디렉터리에서 깨끗하다(`name_check_tree.txt`, 9 모두 rc 0).

### 11.5 측정 [M]

facts 는 마지막 옵션 회차(`ll_final_extract`, 장부 `OBS_ll_final_extract`), 도구는 kz-static `1a47abd0`. 계획 `mv_plans_v4/<b>`(층 예산 `tier:D07`, 대조 예산 `all`, 이름 공간 자료 다섯, 값 요청 `plan.req`)와 측정 계획 `mv_plans_v4_all/<b>`(층 예산 `all`; 부팅용이 아님). 수의 출처: `compare/measure_<b>.txt`(`measure_v4.py`), `compare/paths_<b>.txt`·`compare_paths.txt`(`measure_paths.py`), `compare_parse_v4_full.txt`. 측정이고 판정이 아니다. 아래 표의 모든 값이 선언 §11.0 의 scratchpad [I] 와 같다([I] 에 없던 것: r2_core 의 `object-delete`, 경로별 분모).

**(1) 경로 A 대조 선택**

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| 경로 A 행 | 63 | 63 | 25 | 66 | 66 |
| 대조 집합 (a+b / b) | 5 (1 / 4) | 5 (1 / 4) | 4 (0 / 4) | 15 (0 / 15) | 15 (0 / 15) |
| 대조로 계획된 행 / 잘린 행 / `complete` | 5 / 0 / yes | 5 / 0 / yes | 4 / 0 / yes | 15 / 0 / yes | 15 / 0 / yes |
| 층 차례로 계획된 경로 A 행 (개정 2 의 계획과 같음) | 1 (A21) | 0 | 0 | 0 | 0 |
| 어느 쪽으로든 계획된 경로 A 행 | 5 | 5 | 4 | 15 | 15 |
| 그 가운데 관계 1(키 `17975444d83f6589`) 밖 | 4 | 4 | 4 | 15 | 15 |
| probe-create 행 | A20: 층 cut, 대조 planned | A20: 층 cut, 대조 planned | 없음 | 없음 | 없음 |
| site (개정 2 의 같은 facts 계획) | 291 (267) | 291 (253) | 65 (42) | 201 (147) | 193 (139) |
| `creator-call` site (개정 2) | 8 (4) | 8 (0) | 4 (0) | 6 (0) | 6 (0) |
| spawn 문 (`decoder=spawn-token`) | 1 | 1 | 1 | 1 | 1 |

- **C1 의 A20 과 관계 1 밖의 행이 이제 계획에 든다.** c1_pre_tx 의 대조 집합은 A20(`35f07539e7606e2e`, probe-create, a+b), A21(관계 1, b), A22(`f371687b47120887`), A26(`ad9fc1058923aad9`), A63(`b881269bea290f8b`)이고 모두 계획되었다. c1_post_tx 는 같은 다섯(A20 의 키는 `bdcc6978b9a41c8c`)인데 관계 1 은 층 차례로는 계획되지 않고 대조로만 계획된다. 개정 2 의 계획에서 경로 A 의 계획된 행은 c1_pre_tx 의 A21 하나였다.
- r2_core 의 대조 넷(A2–A5) 가운데 A2–A4 는 order class NOPAIR 의 층 3 행이다: `creator:` 줄은 없고 `not a pair: key constants differ` 줄의 생성 호출이 site 로 세어져 (나)에 든 행이다(판정 규칙 8.12 (ㅁ)). c5 두 빌드의 대조 15 는 모두 (나)이고 관계 1 은 그 facts 의 행이 아니다.
- 대조 예산은 `all` 이고 sidecar 의 `control-budget` 줄은 첫 row 줄보다 앞에 있다(`measure_<b>.txt` 의 `control-budget` 줄: `rows= planned= cut=0 complete=yes`, `unresolved="whether the control rows fit the record pool (no event volume measured)"`). 사건 양은 재지 않았다.

**(2) 규칙별로 계획된 것**(need 의 종류·상태, `measure_<b>.txt`)

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| `object-delete` planned / not-planned | 2 / 3 | 2 / 3 | 3 / 1 | 0 / 15 | 0 / 15 |
| `wait-result` planned / not-planned | 0 / 13 | 1 / 12 | 0 / 7 | 0 / 23 | 0 / 22 |
| `event-send` planned / not-planned | 3 / 2 | 3 / 2 | 0 / 4 | 8 / 7 | 8 / 7 |
| `lock-result` not-planned (경로 A·B: 이 개정에 규칙 없음) | 13 | 13 | 7 | 23 | 22 |
| gap | 1 | 2 | 0 | 5 | 5 |
| 값 요청: 행 / 요청 / planned | 4 / 36 / 30 | 4 / 30 / 24 | 1 / 6 / 3 | 12 / 41 / 29 | 12 / 41 / 29 |
| 측정 계획(층 예산 all): site | 1,461 | 1,461 | 1,179 | 700 | 682 |
| 측정 계획: `read-load` site / `read-subscript` planned | 43 / 0 | 43 / 0 | 41 / 0 | 9 / 32 | 8 / 32 |
| 측정 계획: `lock-result` planned (경로 C) | 112 | 112 | 79 | 39 | 38 |
| 측정 계획: `wait-result` planned | 5 | 6 | 5 | 0 | 0 |

- **삭제·회수 자리(판정 규칙 8.2 (마))**: c1 두 빌드에서 관계 1 과 A20 의 `object-delete` 가 계획되었다: `OS_CountSemDelete` 의 입구(site 257)와 `osapi-countsem.c:129` 의 `OS_ObjectIdFinalizeDelete` after-call(site 258, `col=23 nth=1`); 자료는 `framework_namespaces_osal-f34759d.txt` 58 째 줄의 `OS_OBJECT_TYPE_OS_COUNTSEM delete`(sidecar 의 `lifetime` 줄 `applies`). r2_core 는 A2–A4 에 `OS_rmfs` 의 입구와 `osapi-filesys.c:360` 의 retire 호출(`framework_namespaces_osal-7.0.1.txt` 73 째 줄, `OS_OBJECT_TYPE_OS_FILESYS`). 계획되지 않은 것은 모두 `plan_missing(no namespace data entry)`: c1 의 A22·A26(조회·생성이 `OS_TimeBaseGetIdByName`·`OS_TimeBaseCreate`)과 A63(`OS_FileOpenCheck`·`OS_mv`), r2_core 의 A5, c5 두 빌드의 15 행(조회·생성이 `CFE_SB_DeletePipeWithAppId` 와 `CFE_SB_Subscribe`·`SubscribeEx`·`Unsubscribe` 13 행, `CFE_ES_DeleteCDS`·`CFE_ES_RegisterCDSEx` 2 행).
- **대기 규칙**: c1_post_tx 의 관계 1 에 CF 재시도 대기(`cf_cfdp.c:1001:13`, LOOP_DEFS value)의 `wait-result` 가 planned(site 282·286: 조회의 after-call 과 나가는 문장)이고, 조건 출구는 문장이 아니어서 gap `record_insufficient(the exit is no statement)` 하나다. 그 밖의 행의 `wait-result` 는 not-planned(`no poll or count-bounded loop (FACT_LOOPS) at or before the row's sites in their functions`).
- **spawn 묶음**: 빌드마다 행위자 규칙이 계획한 thread 생성 호출의 at-call 하나에 `spawn <site> wrapper=kz_rt_posix_create start=arg2 arg=arg3 out=arg0` 와 `identity <site> decoder=spawn-token` 이 있다(P0 표의 `pthread_create` 항목에서). c5 두 빌드에는 gap `link_unlinked(no creation-argument channel)` 하나가 그대로 있다(생성 인자 0). 이 문을 읽는 런타임 래퍼와 플러그인은 아직 없다 [I: 뒤 단계가 만든다].
- **판 4 의 수**(`compare_parse_v4_full.txt`, 파서가 읽은 수 = 계획 글에서 센 수): `mv_plans_v4` 의 need 49/49/29/99/96, gap 1/2/0/5/5, spawn 1/1/1/1/1, `place=load` 0, `skey=` site 291/291/65/201/193; `mv_plans_v4_all` 의 need 1,386/1,386/1,062/695/685, gap 54/55/22/18/18, spawn 1/1/2/1/1, `place=load` 45/45/43/9/8, `skey=` 1,461/1,461/1,179/700/682.

**(3) 경로별 분모: 층 차례와 대조 선택이 닿는 곳**(`compare_paths.txt`, `compare/paths_<b>.txt`; 칸은 관계 행 / 층 차례로 계획 / 대조로 계획 / 어느 쪽으로든 / 어느 쪽으로도 아님)

| 계획 | 경로 | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|---|
| 개정 3, 층 `tier:D07`, 대조 `all` (`mv_plans_v4`) | A | 63 / 1 / 5 / 5 / 58 | 63 / 0 / 5 / 5 / 58 | 25 / 0 / 4 / 4 / 21 | 66 / 0 / 15 / 15 / 51 | 66 / 0 / 15 / 15 / 51 |
| | B | 170 / 8 / 0 / 8 / 162 | 170 / 8 / 0 / 8 / 162 | 155 / 3 / 0 / 3 / 152 | 67 / 8 / 0 / 8 / 59 | 65 / 7 / 0 / 7 / 58 |
| | C | 134 / 0 / 0 / 0 / 134 | 134 / 0 / 0 / 0 / 134 | 115 / 0 / 0 / 0 / 115 | 39 / 0 / 0 / 0 / 39 | 38 / 0 / 0 / 0 / 38 |
| | 모두 | 367 / 9 / 5 / 13 / 354 | 367 / 8 / 5 / 13 / 354 | 295 / 3 / 4 / 7 / 288 | 172 / 8 / 15 / 23 / 149 | 169 / 7 / 15 / 22 / 147 |
| 개정 2, 층 `tier:D07` (`ll_plans_final` = `mv_plans_v3_final`) | A / B / C 의 계획된 행 | 1 / 8 / 0 | 0 / 8 / 0 | 0 / 3 / 0 | 0 / 8 / 0 | 0 / 7 / 0 |
| 첫 관측 경로의 계획 (`plangen`, 그 단계의 facts; `compare_paths_first.txt`) | A / B / C 의 행과 계획된 행 | 127·170·134 행, 5 / 7 / 0 | 127·170·134 행, 4 / 7 / 0 | 68·155·115 행, 0 / 3 / 0 | 66·73·39 행, 0 / 8 / 0 | 66·71·38 행, 0 / 7 / 0 |
| 측정 계획, 층 `all` (`mv_plans_v4_all`) | A / B / C 의 계획된 행 | 63 / 170 / 134 | 63 / 170 / 134 | 25 / 155 / 115 | 66 / 67 / 39 | 66 / 65 / 38 |

- **계획되지 않은 행의 사유는 하나다.** 어느 쪽으로도 계획되지 않은 행(354/354/288/149/147)은 모두 그 행의 `need row <key> observation not-planned reason=budget_not_run(row outside the budget)` 줄을 가진다. 다른 사유는 없다. 층·disposition·label 로 나누면(`compare/paths_<b>.txt` 의 `rows planned by neither`): c1_pre_tx 의 경로 A 58 = 층 3 의 D11/T? 44, D05/0 9, D06/J 2, D06/R 2, D12/E 1; 경로 B 162 = 층 2 의 D12/E 132, D11/T? 22, D08/J 5, D05/0 3; 경로 C 134 = 층 2 의 D12/E 112, 층 3 의 D12/E 22. c5_641 의 경로 C 39 = 층 2 의 D12/E 8 과 D10/F 3, 층 3 의 D10/F 7, D12/E 7, D03/E 4, D03/F 4, D01/E 4, D01/F 2. c5_642 의 경로 C 38 은 모두 층 3 이다(D01/E 9, D01/F 5, D03/E 6, D03/F 4, D10/F 7, D12/E 7).
- **경로마다 받을 수 있는 label 과 층(코드, `kairos/lib/llvm/plan_gen.cpp`).** 경로 A 는 H·R·J·T?·E·0(:143–149), 경로 B 는 J·T?·E·0(:179–183), 경로 C 는 F·0·E(:211) 를 받는다. 층 1 은 disposition D07 뿐이고(:246), D07 은 label 이 R 이나 J 이고 order class 가 RELOC 일 때만 선다(:234; ENF·FIXED·COND 의 D01–D03, H 의 D04, 0 의 D05, 짝 없는 D06, J·INDEPENDENT 의 D08 이 먼저 걸러짐, :227–233). 그래서 **경로 C 의 행은 어떤 facts 에서도 층 1 에 들 수 없다**: order class 가 RELOC 이면 층 2, 아니면 층 3 이다. 층 예산 `tier:D07` 은 층 1 까지만 계획하고(:381, :347), 대조 선택은 경로 A 행만 본다(:1335). 따라서 지금까지 만든 `tier:D07` 계획(첫 관측 경로의 것, 개정 2 의 것, 이 단계의 `mv_plans_v4`) 어디에도 경로 C 의 계획된 행이 없는 것은 그 빌드들에 볼 경로 C 관계가 없어서가 아니라 층 규칙과 예산의 조합이 그 경로를 뺀 결과다. 측정이 코드와 맞는다: 다섯 빌드의 경로 C 행에서 본 label 은 E(c1·r2)와 E·F(c5)뿐, 층은 2·3 뿐(c5_642 는 3 뿐)이고 층 차례 계획 행은 0 이다(`compare/paths_<b>.txt` 의 `path C:` 줄).
- **경로 C 가 닿는 예산.** 층 예산 `tier:directed`(층 2 까지, :382)는 경로 C 의 층 2 행 112/112/79/11/0 을 계획할 것이고(행의 층은 예산과 무관하게 sidecar 에 적힘; 그 예산의 계획은 만들지 않았다), 층 3 의 22/22/36/28/38 은 `all` 이나 `rows:<n>` 에서만 든다. c5 의 label F 16 행(641: 층 2 에 3, 층 3 에 13; 642: 모두 층 3)이 그렇다. 층 예산 `all` 의 측정 계획에서 경로 C 의 규칙은 실제로 site 를 낸다(위 (2): `read-load` 43/43/41/9/8, `lock-result` planned 112/112/79/39/38, c5 의 `read-subscript` planned 32).
- 경로 B 는 label J 로 층 1 에 든다(계획된 8/8/3/8/7 이 모두 J·RELOC). 경로 B 에는 대조 선택이 없다.
- **2026-11-18 검토 뒤 정정: §12.7 `rules-01`, `semantics-2`, `semantics-11`.** (1) **A21 의 label 이 한 실행 안에서 둘이다.** c1_pre_tx 의 sidecar 는 `row 17975444d83f6589 A21 tier=1 … label=R … disposition=D07 state=planned`(`mv_plans_v4/c1_pre_tx/plan.side:73`), 값 요청은 `id=A21 label=T?`(`plan.req:5`)다. 층 차례는 행의 글에서 읽은 label(`handling: terminal` 줄 → R, `plan_gen.cpp:126-148`)을 쓰고, 값 요청만 사실에서 유도한 `handling_label:` 줄을 읽는다(같은 행: `handling_label: T? … exit-app at cf_app.c:366:5 is open`). 사실 label 로는 A21 이 D11 이라 `tier:D07` 예산 밖이고 대조 행으로만 계획될 것이다: “층 차례로 계획된 경로 A 행 1 (A21)”은 글 label 에 기댄 값이다. lead 의 판단(2026-11-18): 층 예산 `tier:D07` 은 글 label 을 그대로 쓴다(앞 예산의 다시 만들기 점검용); 생성기는 sidecar 행 줄에 두 label 을 쓴다(`label=`·`fact-label=`, plan rules revision 4, 소스만); 사실 label 로 계획을 다시 만들지 않으며, handling label 을 읽지 않는 선택 규칙은 뒤 단계다. (2) **c5 두 빌드의 대조 15 가운데 13(A20–A32)은 지우는 호출 때문에 (나)에 들었다.** 그 행들의 `creator:` 줄은 `cfe_sb_priv.c:123` 의 `CFE_SB_DeletePipeWithAppId` 다(`mv_plans_v4/c5_641/plan.kzs:533-534` 의 `creator-call` site): 그 호출은 pipe 를 지우고(`cfe_sb_api.c:431` `PipeId = CFE_SB_INVALID_PIPE`), 만드는 호출 `CFE_SB_CreatePipe`(`:235`)는 그 행들의 site 가 아니다. 위 글의 “조회·생성이 `CFE_SB_DeletePipeWithAppId` 와 …” 도 그 함수가 지운다는 것을 적지 않았다. relations rules revision 12(소스만)는 그런 호출을 생성자 site 로 세지 않는다. scratch 점검 [I, 근거 아님; `scratch_measure.txt`]: c5 두 빌드의 (나) 15 → 2(A20–A32 가 빠짐), 그 13 행의 order.static unordered → unknown, c1·r2 의 대조 집합은 그대로(5/5/4). **C5 의 계획과 행마다의 기대는 이 고침이 든 도구로 다시 만든 뒤에 커밋한다.** (3) **`event-send` planned 3(c1_pre_tx 의 관계 1)**: site 셋 가운데 조회의 실패·부재 값의 효과는 하나(`cf_cfdp.c:997`)이고, 둘(`:975`, `:985`; `CFE_SB_CreatePipe`·`CFE_SB_SubscribeLocal` 실패의 이벤트)은 성공 값의 open 효과다(다음 채널의 반복에서 닿음; site 의 ref 가 `on success 0 … (open: …)` 로 적음). plan rules revision 4(소스만)는 뒤 둘을 `event-send-other-outcome` need 로 따로 둔다.

### 11.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 11-16 23:49:23 | 1,684,516,864 | 사본 시작(dip 85,532,672) |
| 11-16 23:55:08 | 1,597,448,192 | `make kz-static`(가장 낮은 1,565,974,528, dip 31,473,664), `all`(가장 낮은 1,537,474,560, dip 55,345,152) |
| 11-16 23:58:33 | 1,580,347,392 | 빌드 끝 |
| 11-18 04:17:37 | 1,576,833,024 | 이어서 시작: 마른 실행, 빌드 뒤 기록, 등록부·장부 점검 |
| 11-18 04:22:32–04:25:13 | 1,575,919,616 → 1,570,615,296 | 첫 ctest(dip 5,853,184), golden, 전체 ctest(8,081,408), 단위 점검 둘 |
| 11-18 04:26:49–04:29:36 | 1,569,755,136 → 1,538,945,024 | 점검 프로그램 둘의 컴파일, 파싱 회귀(dip 16,478,208), 새 도구의 회귀 실행, 계획 넷(가장 큰 dip `mv_plans_v4_all` 6,983,680), 다시 만들기, 측정 |
| 11-18 04:33:41 | 1,537,904,640 | 끝 점검(등록부 deep) 뒤 |

- 하한(314,572,800)을 넘지 않았다. 감시 기록 30 개(`df_*.log`)와 점검 빌드의 40 개(`scratch_guard/`)가 모두 `verdict=completed` 다. 빌드 dip 은 필요량(479,942,080) 안이었다(가장 큰 것 55,345,152).
- 남긴 출력(장부 행): `mv_regress_tools_old` 1,451,851, `mv_regress_tools_new` 1,451,851, `mv_regress_manifest` 15,465,021, `mv_plans_v3_t8cf` 886,531, `mv_plans_v3_final` 894,503, `mv_plans_v4` 1,267,187, `mv_plans_v4_all` 6,881,805 B(합 28,298,749); 사본 셋 85,502,480 B. 지운 것: scratchpad 의 점검 빌드와 그 출력(빌드 전), 점검 프로그램 둘과 다시 만들기의 출력·단위 점검의 임시 디렉터리(scratchpad, 비교를 남긴 뒤).

### 11.7 계획과 다르게 한 것

- **세션이 멈췄다가 이어졌다.** 빌드까지는 2026-11-16, 빌드 뒤의 모든 것은 2026-11-18 이다. 빌드를 다시 하지 않고 완결을 재서 이어갔다(덧붙임 1). `post_mc.txt` 의 시각과 HEAD 는 2026-11-18 의 것이고, 그 사이 `kairos/` 는 바뀌지 않았다.
- **선언 밖의 차이 하나로 멈췄다(덧붙임 2).** 유도되는 묶음의 첫 비교가 `artifact_sha256` 네 줄에서 rc 1 이었다. 선언이 도구 해시의 따름 결과를 허용 목록에 빠뜨린 것이다. 까닭을 코드로 밝히고, 덧붙임을 커밋한 뒤, 컨테이너의 바이트 비교를 더한 새 스크립트로 다시 견주었다. 도구와 출력은 다시 만들지 않았다.
- **`compare.sh parse_v4` 의 요약 줄이 남지 않았다(덧붙임 3).** 그 단계가 점검 프로그램을 `head -1` 에 이어 요약 파일 열 가운데 아홉이 비었다. 선언의 조건(첫 줄 `version 4`)은 섰다. 끝까지 읽는 점검을 새 스크립트로 더해 요약을 남기고 계획 글의 수와 견주었다.
- **더한 측정 둘**: 경로별 분모(덧붙임 1 의 3; lead 가 이 단계에 더한 것)와 첫 관측 경로의 계획에 대한 같은 측정(덧붙임 3 의 2). 도구는 바꾸지 않았고 선택 규칙도 바꾸지 않았다.
- **`relink_expected` 50 가운데 34 는 같은 바이트로 다시 링크되었다.** 선언은 “바뀔 수 있음”으로 적었고 반드시 바뀐다고 적은 11 은 모두 바뀌었다.
- **선언에 없던 것 하나가 계획에 나왔다**: r2_core 의 `object-delete` planned 3(`OS_rmfs`). 선언의 [I] 는 c1 과 c5 만 적었다. 측정으로 적는다.
- 설계 연구는 대기 규칙을 “대기의 조건 읽기와 나가는 갈래”로 적었다. 나가는 갈래 가운데 조건 출구는 문장이 아니어서 site 가 아니라 gap 이다(`record_insufficient(the exit is no statement)`); 선언 §11.0 이 그렇게 적었고 c1_post_tx 에서 하나 나왔다.
- 플러그인은 `kzs-sites 4` 를 읽지 않으므로 이 계획들로 파생 빌드도 부팅도 하지 않았다. 계획의 site 가 실제로 묶이는지(GCC 의 호출 열과 `col=`·`nth=` 의 대응 포함)는 재지 않았다 [미확인].
- **2026-11-18 검토 뒤 정정: §12.7 `builds-05`.** 이 단계는 kz-static 을 바꾸고도(`9eb1395f` → `1a47abd0`) relations·order 의 기본 경로 회귀를 선언하지도 돌리지도 않았다; “추출·relations·order 규칙은 그대로”는 소스 차이와 fixture 시험에 기댄 말이었다. 2026-11-18 에 그 도구로 돌렸다(§12.0 (가), §12.4): T8CF facts 와 마지막 옵션 회차 facts 의 relations·키·order·간선·보고 71 점검이 `ll_relcheck`·`ll_order`·`ll_final_order` 와 `cmp` 같다. 다시 뽑기는 돌리지 않았다.

### 11.8 열린 것

- **층 규칙이 경로 C 를 층 1 에서 구조적으로 뺀다**(§11.5 (3)). label F·0·E 뿐인 경로 C 는 D07 이 될 수 없고, `tier:D07` 예산에서는 대조 선택도 경로 A 뿐이라 경로 C 의 계획된 행이 다섯 빌드 모두 0 이다. c5 의 사례가 속한 경로이므로(label F 16 행, 641 은 층 2 에 3) C5 주기의 계획을 만들기 전에 정해야 한다: 경로 C 의 대조 선택 규칙을 둘지, 층 예산을 `tier:directed` 로 둘지(그러면 경로 A·B 의 층 2 도 함께 든다: c5_641 에서 경로 B 59, 경로 C 11), 층 규칙에 경로 C 의 우선 층을 둘지. 어느 쪽이든 규칙 글을 어느 계획보다 먼저 커밋하고 예산을 사건 양의 측정 뒤에 정하는 차례다. 이 단계에서는 선택 규칙을 바꾸지 않았다.
- **경로 B 의 대조 선택이 없다.** 경로 B 는 label J·RELOC 의 행만 층 1 로 계획되고(8/8/3/8/7), 층 2 의 E·T?·0 행(c1 에서 157)은 `tier:D07` 에서 계획되지 않는다. 대조가 필요한지는 경로 B 의 연결 규칙이 설 때 함께 정할 일이다.
- **대조 예산과 사건 양.** 대조 집합은 다섯 빌드 모두 통째로 계획되었다(`complete=yes`). 기록 풀에 드는지는 재지 않았다(sidecar 의 `unresolved`). C1 주기의 부팅 전에 행마다의 기대([I])를 커밋하고, 풀이 넘치면 그 부팅은 사전 시험이 아니다(§6.10 의 2).
- **이름 공간 자료에 항목이 없는 삭제 자리**: `OS_TimeBase*`(c1 의 A22·A26, r2_core 의 A5), `OS_FileOpenCheck`·`OS_mv`(c1 의 A63), c5 두 빌드의 15 행(SB 의 pipe·구독, ES 의 CDS). 조건 (마)를 이 행들에서 보려면 semantics coverage rule 의 이름 공간 자료에 그 종류의 delete·clear 항목이 있어야 한다(자료의 다음 개정; 엔진에 이름을 넣지 않음).
- **대기 규칙이 닿는 곳이 좁다.** 계획된 `wait-result` 는 c1_post_tx 의 하나(측정 계획에서 5/6/5/0/0)다. 조건 출구는 site 종류가 없어 gap 이고, c5 의 대기 다섯은 LOOP_DEFS 가 알아보지 못한다(§10.8).
- **경로 A·B 의 잠금 호출, 분기, 호출 없는 효과**는 이 개정에 규칙이 없다(`lock-result` not-planned 13/13/7/23/22, 값 요청의 branch not-planned 6/6/3/12/12).
- **판 4 를 읽는 쪽이 없다.** 런타임의 생성 래퍼(`kz_rt_posix_create`)와 `decoder=spawn-token`, 플러그인의 `col=`·`nth=`·`place=load`·`spawn` 묶음은 뒤 단계다. 그때까지 `mv_plans_v4` 는 형식과 생성의 근거일 뿐 관측의 근거가 아니다. c5 의 `link_unlinked(no creation-argument channel)` gap 은 그 토큰이 생긴 뒤에 다시 잰다.
- **계획이 자료 파일의 절대 경로를 적는다**(site 의 `ref=` 와 sidecar 의 `lifetime`·`namespace-data` 줄). 다시 만들기는 같은 경로에서 바이트로 같았다. 주기의 연결 점검이 이 줄을 어떻게 견줄지는 그 단계의 선언에서 정한다.
- `compare.sh parse_v4` 의 `head -1` 결함은 스크립트에 그대로 있다(선언과 함께 커밋된 스크립트라 고치지 않고 `parse_v4_full.sh` 를 더함). 다음 단계가 이 스크립트를 옮겨 쓸 때는 끝까지 읽는 쪽을 쓴다.
- 앞 단계에서 넘어온 열린 것(의미 자료의 `lock-object` 역할, LOOP_DEFS 술어의 L1 인증, event type 의 등급, open 효과, c5 의 child without success, J?, 이름 결합의 구현)은 그대로다.
- **2026-11-18 검토 뒤 정정: §12.7 `rules-01`, `rules-03`, `rules-06`, `builds-08`, `builds-09`.** (1) 위 넷째 항목(“이름 공간 자료에 항목이 없는 삭제 자리”)의 까닭이 틀렸다: `OS_TimeBase*` 의 세 대조 행(c1 의 A22·A26, r2_core 의 A5)은 TIMEBASE 항목에 **delete 줄이 있는데도**(`framework_namespaces_osal-f34759d.txt:101`) 계획되지 않았다. 그 항목에 `lookup`·`create` 줄이 없어(종류 상수를 직접 넘기는 파일; §8.5 의 정정) 행의 조회·생성 callee 와 맞는 항목이 없기 때문이다. c5 의 SB pipe 13 행은 relations rules revision 12 뒤에는 `key clearing:` 줄의 호출이 object-delete site 가 된다(소스만). (2) A21 의 두 label(§11.5 의 정정). (3) `compare_measure.txt` 의 빌드별 `rc=0` 은 근거가 아니다(`compare.sh:45-46` 이 `|| rc=1` 뒤의 `$?` 를 찍음; 끝 줄의 rc 만 뜻이 있음). 다시 돌림: `recheck/recheck_measure.txt`(다섯 빌드 rc 0, 남긴 파일과 같은 바이트). (4) `compare_plans.py` 의 rules 줄 허용은 선언보다 넓었다(쓰인 적 없음): `recheck/recheck_plans.txt`(쉰다섯 “only declared differences”). (5) 판정 규칙 8.7 의 입력 가운데 아직 없는 것은 §12.9 의 표.

## 12. 검토 뒤 고침 (둘째 묶음) (2026-11-18)

§7–§11 의 검토가 확인한 지적(판정 규칙·자료 `rules-01`–`rules-08`, 빌드와 비교 `builds-01`–`builds-05`·`builds-07`–`builds-10`, 의미 `semantics-1`–`semantics-11`)을 고친다. 지적마다 무엇이 틀렸고 무엇을 바꿨고 무엇을 다시 돌렸는지는 §12.7 의 표에 둔다. 앞 절의 글은 지우지 않고, 고친 자리에 날짜 붙은 표시(“2026-11-18 검토 뒤 정정: §12.7 <id>”)를 단다. 기록: `../records/review-fixes-2-20261118/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/` 아래 새 디렉터리 `rf2_*`. cFS 를 부팅하지 않는다. 파생 빌드를 하지 않는다. push 하지 않는다. 이 묶음을 묶는 lead 의 판단(2026-11-18)은 §12.0 (다)에 옮겨 적는다.

### 12.0 선언 [D]

#### 12.0 (가) 빌드 전의 점검: 지금 도구의 기본 경로 회귀 (돌리기 전에 적고 커밋함)

지금 MC 의 도구(kz-static `1a47abd05b8289bce90e6da8906d3b84d9bec48befc99634295d5350cd261086`, manifest 개정 4 단계의 빌드)만 쓰고 Kairos 를 빌드하지 않는다. 그 단계는 kz-static 을 바꾸고도(`9eb1395f` → `1a47abd0`, 커밋 `5011169` 가 `lib/llvm/plan_gen.{cpp,hpp}`·`tools/kz-static/{cmd_plan.cpp,main.cpp}` 를 고침) relations 와 order 의 기본 경로 회귀를 선언하지도 돌리지도 않았다(`builds-05`). 도구를 다시 덮기 전에 그 도구 자신의 기록을 남긴다.

- 스크립트 `prebuild/prebuild_regress.sh`(이 선언과 함께 커밋), 공간 감시 아래(`env/tools/space_guard.sh`, 필요량 60,000,000: 출력 약 31 MB 와 여유). 새 출력 디렉터리 셋:
  - `rf2_pre_relcheck/<b>`: T8CF facts(`/home/user/work/redesign/step2a/tests_8c_fix/<b>/NP/facts.kz`)의 `relations --keys`. 선언: `relations.txt`·`keys.txt` 가 `ll_relcheck/<b>` 의 것과 `cmp` 같음, `relations.log` 빈 파일.
  - `rf2_pre_order/<b>`: 같은 facts 의 `order --edges`. 선언: `order.txt`·`edges.tsv` 가 `ll_order/<b>` 의 것과 `cmp` 같음, `order.log` 빈 파일.
  - `rf2_pre_final_order/<b>`: 마지막 옵션 회차(`ll_final_extract/<b>/<b>/NP/facts.kz`)의 `order --edges --caller-dependent-report` 와 `relations --keys`. 선언: `order.txt`·`edges.tsv`·`cdr.txt`·`relations.txt`·`keys.txt` 가 `ll_final_order/<b>` 의 것과 `cmp` 같고, `relations.txt` 는 그 다시 뽑기의 것(`ll_final_extract/<b>/<b>/NP/relations.txt`)과도 같음, 로그 둘 빈 파일.
- 선언: 도구 확인 1 줄과 다섯 빌드의 점검 14 씩, 모두 71 줄이 OK. 아래 밖의 차이는 결함이고, 결함이면 멈추고 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 빌드로 가지 않는다. 다시 뽑기(`extract`)는 이 점검에 넣지 않는다: 빌드 뒤의 다섯 빌드 다시 뽑기가 `ll_off_extract`·`ll_final_extract` 와 견준다(§12.0 (나)).
- 장부 행 `OBS_rf2_pre_relcheck`, `OBS_rf2_pre_order`, `OBS_rf2_pre_final_order`(sha256 목록은 기록 디렉터리).

#### 12.0 (가-1) 앞 단계의 비교를 고친 스크립트로 다시 돌림, 그리고 선언 §10.0 의 덧붙임 (돌리기 전에 적고 커밋함) [D]

앞 단계의 스크립트와 출력은 그대로 두고, 고친 사본을 새 이름으로 `recheck/` 에 두어 새 파일에 다시 돌린다(`recheck/recheck.sh <step>` → `recheck/recheck_<step>.txt`, 공간 감시 아래). 도구도 출력 디렉터리도 다시 만들지 않는다.

1. **중간 회차의 섹션 비교(`builds-02`)와 선언 §10.0 의 덧붙임(2026-11-18).** `records/loop-lock-20261116/rounds_cmp.py:44-45` 는 섹션의 레코드 글 sha256 만 견주고 count 를 보지 않는다. STRINGS 섹션은 kz-dump 글에 레코드 줄이 없어(모든 회차에서 `lines=0`, `text_sha256=e3b0c442…` = 빈 글의 sha256) 그 비교에서 늘 “같은 레코드 글”로 세어졌지만, 그 count(문자열 풀의 바이트 수)는 회차마다 다르다(c1_pre_tx: `records/semantics-20261116/section_text_new_2.txt` 1,609,737, `records/result-flow-20261116/section_text_on.txt` 3,740,581, `records/loop-lock-20261116/section_text_final.txt` 3,759,771). 선언 §10.0 의 중간 회차 비교는 “다른 섹션은 BASIS 와 FACT_EXPRS 여야 한다; 그 밖은 덧붙임”이라 적어 STRINGS 를 빠뜨렸다. **덧붙임**: 중간 회차와 마지막 회차의 섹션 비교에서 다를 수 있는 섹션은 STRINGS(풀은 참조된 문자열을 한 번씩 담으므로 뒤 옵션이 더한 섹션의 문자열만큼 커진다; 레코드 줄이 없어 글로는 견줄 수 없고 count 가 다름), BASIS(뒤 옵션의 항목과 도구 정체성), FACT_EXPRS(뒤 섹션이 뒤에 더한 뿌리)다. 그 밖의 섹션은 레코드 글과 count 가 모두 같아야 한다. `recheck/rounds_cmp_2.py`(count 도 견줌; 설명된 차이에 STRINGS). 선언: 열다섯(회차 셋 × 빌드 다섯) 모두 OK, 회차의 섹션 42/43/45 가운데 글과 count 가 같은 것 39/40/42 이고(c5 의 결과 흐름 회차는 FACT_EXPRS 가 같아 43), 다른 것은 STRINGS·BASIS·FACT_EXPRS 뿐 [I: 돌리기 전]. STRINGS 의 내용이 같은지는 이 비교가 보지 않는다(풀의 바이트는 섹션별 글에 없음): 풀을 가리키는 모든 레코드의 글이 같다는 것이 근거다.
2. **계획 비교의 허용(`builds-08`).** `compare_plans.py`(의미·결과 흐름·루프 단계와 manifest 단계의 사본, `:23-24`)는 rules 줄에서 `plan rules revision [12]` 와 `relations rules revision [0-9]` 의 차이를 받는데, 선언 §8.0–§11.0 은 rules 줄의 차이를 허용하지 않았다. `recheck/compare_plans_2.py` 는 그 두 무늬를 뺀다(rules 줄과 manifest 첫 줄은 두 쪽이 같아야 함). 그 단계들이 견준 계획 짝 열하나(각각 다섯 빌드)를 다시 견준다: `objacc_plans`↔`sem_plans`, `objacc_plans_off`↔`sem_plans_off`, `objacc_plans_on`↔`sem_plans_on`, `sem_plans`↔`rf_plans`, `sem_plans_off`↔`rf_plans_off`, `sem_plans_on`↔`rf_plans_on`, `rf_plans`↔`ll_plans`, `rf_plans_off`↔`ll_plans_off`, `rf_plans_on`↔`ll_plans_final`, `ll_plans`↔`mv_plans_v3_t8cf`, `ll_plans_final`↔`mv_plans_v3_final`. 선언: 쉰다섯 모두 “only declared differences” [I: 넓은 허용이 쓰인 적이 없다는 검토의 읽기].
3. **manifest 단계의 측정 줄(`builds-09`).** `records/manifest-v4-20261116/compare.sh:45-46` 은 `python3 … || rc=1` 뒤에 `rc=$?` 를 찍어, `measure_v4.py` 가 실패해도 빌드별 줄은 `rc=0` 이다. `recheck.sh measure` 는 상태를 먼저 잡아 찍고, 새로 낸 `recheck/measure/measure_<b>.txt` 를 남긴 `compare/measure_<b>.txt` 와 `cmp` 한다. 선언: 다섯 빌드 모두 rc 0 이고 같은 바이트.

#### 12.0 (나) kz-static 빌드: 선언하지 않았고 빌드하지 않았다 [M]

이 묶음의 코드 고침은 kz-static 빌드 하나로 싣기로 되어 있다(§12.0 (다)의 (a)). **그 빌드의 선언은 이 절을 쓰는 지금 적지 않았고, MC 의 kz-static 은 다시 빌드하지 않았다.** MC 의 도구는 그대로 `1a47abd0…` 이다. 한 것은 소스의 고침과 fixture·시험, 그리고 scratch 점검 빌드(감시 아래, `scratch_guard/`)까지다. 까닭과 남은 차례는 §12.8·§12.9 에 적는다. 따라서 §12.1 의 코드는 **MC 의 바이너리보다 앞선 소스**이고, 이 절 아래의 [I] 값은 scratch 점검 빌드의 것(근거 아님)이다.

#### 12.0 (나-1) kz-static 빌드의 선언 (2026-11-19, 빌드 전에 적고 커밋함) [D]

위 (나)가 남긴 빌드를 한다. 아래 밖의 차이는 결함이다. 결함이면 그 비교에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다. 기록: `../records/review-fixes-2-mc-20261119/`(`README.md`). cFS 를 부팅하지 않고, push 하지 않는다.

- **소스.** 빌드할 소스는 이 선언을 커밋하는 HEAD 의 `kairos/` 다. `kairos/` 의 코드는 `d564a01` 뒤로 바뀌지 않았고, 그 뒤 커밋 셋(`5c460eb`, `0eaf478`, `2e4827a`)은 `kairos/docs/` 의 글만 고쳤다(`git log 00d1cc7..HEAD -- research/cfs_procrace/kairos`). §12.1 의 고침 전부가 이 빌드로 도구에 들어간다.
- **scratch 점검 빌드**(근거 아님, [I] 의 출처): 앞 에이전트가 2026-11-18 10:29Z 에 같은 소스(당시 HEAD `8249ba6`; 그 뒤 `kairos/` 에서 바뀐 파일은 위 문서 넷뿐, 파일 시각으로 확인)로 scratchpad 에 LLVM Release 구성으로 빌드하고, 다섯 빌드의 옵션 없는 회차와 마지막 옵션 회차를 다시 뽑고 relations·order·계획을 돌렸다(감시 기록 `scratch_guard/001-016`). 이 실행이 그 출력을 읽어 허용 차이를 출력 종류마다 분류했다(`scratch_guard/017-029`, 보고 `declared/`, 요약 `declared_summary.txt`). 그 scratch kz-static 의 `--identity` 는 `tool_sha256=73c3491cead05ce1f4f6adb999970a41e680569182848a644646fca65613ecff`, `tool_source=2b8834da4c83efe08b12d090bf2db4b269edf14d67fac0b8da25cc3ef012c13b` 이다(`pre_mc.txt`). 주의: 그 scratch make 의 감시 기록은 가장 낮은 여유 305,602,560 B 로 하한 314,572,800 아래를 적었다(`scratch_guard/002_scratch-make.log`; 다른 에이전트의 일이 병행; 감시는 멈추지 않음). 이 실행의 일이 아니지만 적는다.
- **도구 해시와 정체성**(`pre_mc.txt`, 빌드 전):
  - kz-static: 파일 sha256 `1a47abd0…` 이 바뀐다. `--identity` 는 `tool_sha256` `b6a64d83…` → `73c3491c…`, `tool_source` `88806cfe…` → `2b8834da…` 여야 한다 [I: scratch 의 값; 정체성은 경로를 상대로 둔 소스 파일의 sha256 에서 나오고 컴파일러에 기대지 않는다, `kairos/cmake/KzSourceIdentity.cmake:3-10`, `kairos/include/kz/core/identity.hpp:63`]. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 그대로.
  - 그대로인 도구: kz `fd40bc4d…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz_lint `3d4d5a8f…`, kz-rt-dump `40e23d4d…`, kz-instr-clang `36becd04…`(파일과 `--identity` 모두).
  - 다시 링크되는 것 14(`relink_expected.sha256`, 빌드 전 sha256): dry run(`make -n all`, `make_dryrun_before_build.log`: 다시 컴파일되는 소스 13, 링크 스크립트 6)과, MC 의 `link.txt` 가운데 `libkz-llvm.a` 를 이름 대는 것(그 아카이브 자신과 실행 파일 11)을 합친 것: `lib/llvm/libkz-llvm.a`, `tools/kz-static/kz-static`, `tests/static/{static_golden,test_static_cli}`(소스가 바뀜), `tests/static/test_static_{elf32,expr,flow,gitrev,link,plan,reasons,recipe,unroll,wrappers}`. 반드시 바뀌는 것: `libkz-llvm.a`, kz-static, `static_golden`, `test_static_cli`, `test_static_plan`, `test_static_reasons` [I]; 나머지 여덟은 같은 바이트로 다시 링크될 수도 있다.
  - 다시 링크되지 않아야 하는 것 110(`not_relink.sha256`, 빌드 전 sha256; 앞 단계 목록 124 에서 위 14 를 뺀 것): 빌드 뒤 같은 sha256. `CMakeCache.txt` 의 sha256 그대로 [I](소스가 바뀌어 다시 구성된다).
- **도구 보존**(PI 결정: in-place MC 재빌드는 사본을 압축하고 왕복 확인한 뒤 중간 도구를 덮을 수 있다; 결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함): 빌드 전에 kz-static `1a47abd0` 을 새 디렉터리 `/home/user/work/preserve/tools-20261119/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 감시 아래, 필요량 150,000,000). 사본 직후 등록부(`records/TOOLS.tsv`)의 그 행을 in-place+copy 로, 빌드 뒤 copy 로 고치고 새 kz-static 을 in-place 로 더한다. 장부 행 `TOOLS_20261119`.
- **빌드**(MC `/home/user/work/merge-check/kz-llvm` 제자리, 각각 감시 아래, 필요량 480,000,000: 앞 kz-static 빌드 dip 의 최대 256,942,080 과 다시 링크되는 큰 실행 파일의 임시 쓰기와 여유): `make kz-static`, `make all`. 도구가 새 `--identity` 를 적을 때까지 출력을 내지 않는다.
- **첫 ctest**(전체 268, `-V`): 실패는 정확히 다음 16 이어야 한다(`ctest_first_expected_fail.txt`): `static_golden_{relations,relations_np,relpaths_np,relsound_np,relincomplete_np,relincomplete_cut_np,relcallback_np,order_np,objaccess,objaccess_off,semantics_np,resultflow_np,resultflow_order_np,looplock_np,looplock_order_np}`, `test_static_plan`. 근거: scratch 점검 빌드의 첫 ctest(정적·lint 78 개, 같은 HEAD golden; `scratch/ctest_first_scratch.log`: 이 16 이 실패, `test_static_reasons` 는 통과). 그 밖의 190 개는 다시 링크되는 바이너리를 쓰지 않으며, 빌드 전 MC 에서 돌려 실패 0·건너뜀 1(`test_probe_anchor_reached`)이었다(`ctest_nonstatic_prebuild.log`).
  - **(2026-11-19 덧붙임, `builds-05`)** 위의 '그 밖의 190 개는 다시 링크되는 바이너리를 쓰지 않으며' 는 맞지 않는다. 그 190 가운데 `tool_identity_schema_revision` 은 `$<TARGET_FILE:kz-static>` 을 부르고(`kairos/tests/unit/CMakeLists.txt:41-46`), kz-static 은 이 빌드에서 다시 링크되었다. 그 시험은 빌드 전 MC 에서 옛 바이너리로만 돌았고(`ctest_nonstatic_prebuild.log:142-143`) scratch 점검 빌드의 첫 ctest 에는 없다(`scratch/ctest_first_scratch.log` 에 0 줄; `../records/closure-notes-20261119/check.txt` E). 결과는 선언대로였다(첫 ctest 의 실패는 선언한 16, golden 뒤 268 가운데 실패 0; §12.11 (나)). [§26.1]
- **golden**: 그 16 시험만 `KZ_GOLDEN_UPDATE=1` 로 돌려 쓴다. 바뀌는 golden 은 17(`tests/static/data/` 의 `relations`, `relations_np`, `relpaths_np`, `relsound_np`, `relincomplete_np`, `relincomplete_cut_np`, `relcallback_np`, `order_np`, `objaccess`, `objaccess_off`, `semantics_np`, `resultflow_np`, `resultflow_order_np`, `looplock_np`, `looplock_order_np` 의 `.golden` 과 `plangen_np.plan.golden`, `planv4_np.plan.golden`)이고 각각 `../records/review-fixes-2-20261118/golden_scratch/` 의 같은 이름 파일과 `cmp` 같아야 한다(`golden_scratch.sha256`). `kairos/` 의 그 밖의 파일은 바뀌지 않는다(`git status`).
- **시험**: golden 뒤 전체 ctest 268, 실패 0, 건너뜀 1. 확인 수는 남긴 출력에서 센다 [I: `../records/review-fixes-2-20261118/scratch_ctest_static_2.log`]: `test_static_cli` 1,184, `test_static_reasons` 763, `test_static_plan` 277, `lint_selftest` 114 경우 실패 0, lint 규칙 일곱 clean, `test_core_schema` 34,017(그대로). 단위 점검 `env/tools/tests/run_unit.sh` 17/17, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤와 실행 뒤 각각 `env/tools/ledger_check.sh`(빌드 전 135 행 실패 0: `ledger_check_pre.txt`)와 `env/tools/tools_check.sh --deep`(빌드 전 49 행 실패 0, 스트림 없이: `tools_check_pre.txt`). 출력은 단계마다 새 이름.
- **다섯 빌드의 실행**(`run.sh <step>`, 단계마다 감시 아래 필요량 120,000,000, 새 디렉터리 `/home/user/work/redesign/obs1/rf2_*`): `relcheck`(T8CF facts 의 `relations --keys` → `rf2_relcheck`), `order`(같은 facts 의 `order --edges` 두 번 → `rf2_order`), `extract_off`·`extract_final`(`env/redesign/one_build.sh`; 옵션 없이 → `rf2_off_extract`, 남긴 마지막 회차와 같은 인자: `--object-access`, `--semantics` 자료 여덟, `--result-flow --loop-defs --lock-handles` → `rf2_final_extract`), `off_order`·`final_order`(새 facts 의 order 두 번과 relations), `plans_t8cf`·`plans_final`(`--manifest` 없는 계획), `plans_v4`(manifest 4, 층 예산 `tier:D07`, 대조 예산 `all`, 이름 공간 자료 다섯, 값 요청), `plans_v4_all`(층 예산 `all`, 측정용), `plans_v4_kept`(남긴 `ll_final_extract` facts 를 새 생성기로). 자료 파일은 하나씩 이름 대고 단계마다 `data_inputs.sha256` 과 대조한다. 입력 글의 바뀜 둘을 미리 적는다: 대조 예산의 이유 글은 §12.10 3 항의 말("control runs of design rows … not a test of the name-binding rule")로 쓰고, 측정 계획의 예산 이유 글은 개정 번호를 빼고 "a measurement of the plan rules on every row of the build (not a plan for a boot)" 로 쓴다(`plan_one.sh`).
- **비교**(`compare.sh <step>`, 감시 아래; 보고는 새 디렉터리 `compare/`, 덮어쓰지 않음). 기준은 앞 도구 `1a47abd0` 의 남긴 출력이다: relations·keys 는 `rf2_pre_relcheck`, T8CF order 는 `rf2_pre_order`(지운 `ll_relcheck`·`ll_order` 와 같은 바이트, §12.6), 다시 뽑기는 `ll_off_extract`·`ll_final_extract`, 그 order 는 `ll_off_order`·`ll_final_order`, 계획은 `mv_plans_v3_t8cf`·`mv_plans_v3_final`·`mv_plans_v4`·`mv_plans_v4_all`.
  1. **허용 차이, 출력 종류와 칸마다**(`rules/<kind>.rules` 를 `cdiff.py` 가 읽음; 줄을 다중 집합으로 견주고, 다른 줄은 모두 선언된 class 에 들어가야 한다: `other=0`). 종류별 class: relations(`relations.rules`: rules 줄의 개정, 머리 줄 넷, `summary A` 셋, 이름 공간의 `publication writes`·`key-parameter functions`·`uses`·`left out`, 행의 `key clearing`(새)·`dependency`·`creator`·`not a pair`·`callee-check`·`flip`·`feasibility`·`incomplete`·`layers`), keys(facts 줄, rules 줄), order(`order.rules`: 도구 줄과 relations 줄의 개정, facts 줄, `summary`, 행의 `order.static`·`pair`·`pairs`·`threads started by`·`opposite`·`awaited`, 옵션 규칙 줄, 간선의 머리·`from`·`to`·`condition`·`premise`·`scope`·`instance key`·`wait`, 대기의 `use`, 절의 수 줄), edges(HB-await·HB-seq 간선 줄), cdr(머리의 개정, facts 줄, `pairs of the rows`), extract.txt(facts 줄, semantics·result flow·lock handles 의 수 줄), basis.txt·sha256.txt(basis 줄, 컨테이너 둘과 kz-static 의 sha256·경로), facts 보기(`view.rules`: 행 번호·흐름 행 참조·outcome 등급·조건과 깃발의 네 칸, 그리고 절 basis·semantics·resultflow·resulteffect·atype·lockhandles·loopdefs 의 줄), 계획 파일(`plan.rules`: 머리의 개정·sha256·경로, args 의 입출력 경로와 두 이유 글, 문장 종류별 class). 컨테이너는 절마다(`sections_cmp.py`, kz-dump 로 읽음): 옵션 없는 회차는 BASIS 1 레코드(`extract.units_from` 의 basis 접두; 도구 정체성이 바뀌어 basis 의 sha256 이 바뀜)와, c5 두 빌드는 FACT_ATYPE 5 레코드(가변 인자 탈출의 사유 글, `semantics-3`)와 STRINGS 의 수; 마지막 회차는 BASIS(26 → 28 레코드가 다름; `extract.outcome_classes` 등), FACT_EXPRS, FACT_ATYPE, LOOP_DEFS, SEMANTICS, RESULT_FLOW, RESULT_EFFECT, LOCK_HANDLES(c5 는 없음)와 STRINGS 의 수. 그 밖의 절은 같은 레코드 글이어야 한다.
  2. **수까지**(`compare.sh declared`): 모든 보고의 `cdiff`·`class`·`header`·`section`·`count`·`summary` 줄이 `declared/` 의 같은 이름 보고(scratch 출력과 남긴 출력의 비교, 이 선언과 함께 커밋)와 같아야 한다. 보고 275 개 [I]. 종류별 줄 수의 요약은 `declared_summary.txt`(예: 옵션 없는 회차의 relations 는 빌드마다 앞 쪽에만 있는 줄/새 쪽에만 있는 줄 19/15·19/15·150/102·150/102·9/8 (c1_post_tx·c1_pre_tx·c5_641·c5_642·r2_core 차례), T8CF order 3/3·3/3·55/29·55/29·3/3, 옵션 없는 회차의 facts 보기 1/1·1/1·6/6·6/6·1/1; 마지막 회차의 order 257/2067·256/2066·58/32·58/32·144/722).
  3. **쌍둥이**(`compare.sh twin`, `twin.py`): MC 빌드의 출력은 scratch 점검 빌드의 출력과 파일마다 같은 바이트여야 한다. 다를 수 있는 것은 scratch 쪽 문자열 넷의 바꿈(출력 뿌리의 경로, scratch kz-static 의 경로, 그 파일 sha256, 실행기가 적은 저장소 커밋 줄)과 `status.txt` 첫 줄의 시각뿐이다. 실행기의 `space_guard.log` 와 order 의 둘째 실행(`again/`, 첫 실행과 같은 바이트임을 확인한 뒤 `compare.sh` 가 지움)은 견주지 않는다. 컨테이너(facts.kz, basis.kz)는 도구 정체성이 같으므로 바이트로 같아야 한다 [I]. 자체 점검 `selftest_twin.txt`.
  4. 그 밖: order 의 두 실행이 같은 바이트, 로그 빈 파일, 다시 뽑기의 `relations.log` 가 남긴 것과 같은 바이트, `status.txt` 가 `result PASS`, `plan.log` 가 남긴 것과 같은 바이트, `args.txt`(다시 뽑기)가 남긴 것과 같음.
- **측정**(비교가 모두 OK 인 뒤; `measure.py`, 수는 남긴 출력에서만; 판정이 아님): §12.10 이 물은 것: 하나의 outcome 규칙 아래의 handling label 과 label 이 움직인 모든 행, exit-app·terminal·event 의 수, SEMANTICS 의 검증된 행 수, LOCK_HANDLES 의 나뉨, decay 상태의 움직임, writer 행위자마다의 HB-await 간선, 생성과 삭제를 가른 뒤의 c5 대조 집합과 c1·c5 에서 key-clearing call 이 된 호출(소스와 대조), 계획의 site·need 수. 예상 움직임은 재기 전까지 [I] 다(§12.5 의 [예비] 수).
- **공간**: 빌드 전 여유 1,050,628,096 B(하한 314,572,800). 필요량: 사본 150,000,000, 빌드 480,000,000, 실행 단계 120,000,000 씩(새 출력 약 199 MB 를 남김: scratch 출력의 크기), 비교의 facts 보기 150,000,000(빌드마다 지움). 빌드 전에 scratch 빌드 트리(약 296 MB)를 지운다. scratch 출력(약 199 MB)은 쌍둥이 비교 뒤 지운다. 모자라면 §12.10 2 항의 압축 대체만 쓴다.
- **스크립트**(이 선언과 함께 커밋, Python 은 `python3 -I`): `run.sh`, `plan_one.sh`, `compare.sh`, `cdiff.py`, `sections_cmp.py`, `twin.py`, `rules/*.rules`, `tool_copy/copy.sh`; 목록 `relink_expected.sha256`, `not_relink.sha256`, `ctest_first_expected_fail.txt`, `data_inputs.sha256`; 빌드 전 점검 `pre_mc.txt`, `ledger_check_pre.txt`, `tools_check_pre.txt`, `ctest_nonstatic_prebuild.log`. `run.sh`·`compare.sh`·`cdiff.py`·`sections_cmp.py`·`plan_one.sh` 는 앞 에이전트가 2026-11-18 에 커밋하지 않고 남긴 초안(무시되는 디렉터리 `../records/review-fixes-2-20261118/build/`)에서 옮겨 고쳤다(`README.md` 에 고친 곳).

#### 12.0 (나-2) 덧붙임 (2026-11-19, 쌍둥이 비교가 선언 밖의 차이에서 멈춘 뒤, 다시 돌리기 전에 커밋함) [D]

`compare.sh twin` 이 선언 밖의 차이 47 파일에서 멈췄다(`compare_twin.txt`, `compare/twin.txt`: 350 파일 가운데 같음 303, 다름 47, 건너뜀 10). 그 앞의 비교 열하나는 모두 OK 이다(`compare_all.txt`). 까닭 둘을 밝히고 선언한다.

1. **`status.txt` 의 `repo` 줄(2 파일: `rf2_final_extract` 의 c5 두 빌드, 셋째 줄).** 실행기는 실행 때의 저장소 HEAD 를 적는다(`env/redesign/one_build.sh`). scratch 실행은 2026-11-18 에 다른 단계가 커밋하는 동안 돌아 HEAD 셋을 적었다(`8249ba6` 여덟, `a414a58` 하나, `e809ba1` 하나). 선언의 바꿈은 한 쪽에 커밋 하나를 가정했다. 이 실행의 열 파일은 모두 `090e280`(이 실행 중 다른 단계의 커밋)이다. `8249ba6` 과 `a414a58`·`e809ba1` 사이에 `kairos/` 의 문서 밖 파일은 바뀌지 않았고, `090e280` 까지는 새 이름의 자료 파일 다섯(`kairos/semantics/framework_namespaces2_*.txt`)만 더해졌다. 실행은 자료 파일을 하나씩 이름 대고 `data_inputs.sha256` 과 대조하므로(`run.sh`) 그 파일들을 읽지 않는다. **덧붙임**: 두 쪽의 `status.txt` 에서 `repo` 줄을 가린다.
2. **계획 sidecar 의 sha256(45 파일: `rf2_plans_final`·`rf2_plans_v4`·`rf2_plans_v4_all` 의 빌드마다 `plan.kzs` 둘째 줄, `plan.mod`·`plan.exe` 첫 줄).** sidecar 는 입력 facts 와 basis 의 경로를 적는다(`input facts sha256=… path=…`). 그 경로는 선언된 바꿈(출력 뿌리)이므로 sidecar 자신은 바꿈 뒤 같지만 바이트로 다르고, 그 sha256 을 적는 세 파일이 다르다. 입력이 두 쪽에서 같은 경로인 `rf2_plans_t8cf`·`rf2_plans_v4_keptfacts` 에서는 이 차이가 없다. **덧붙임**: 바꿈 뒤 같지만 바이트로 다른 파일 쌍의 sha256(scratch → 이 실행)을 바꿈 목록에 더하고 모든 파일을 다시 견준다(두 번째 걸음).
- `twin_2.py`(`twin.py` 에 위 둘만 더함), `compare.sh twin_2`, `compare_rest.sh`(twin_2 와 declared 를 차례로; 앞 보고는 건드리지 않음). 선언: 350 파일 모두 같음, 다름 0, 한 쪽에만 있음 0, 건너뜀 10(`space_guard.log`). 그 뒤 `declared` 가 모든 보고 275 의 줄이 `declared/` 와 같아야 한다.

#### 12.0 (다) 이 묶음을 묶는 lead 의 판단 (2026-11-18)

(a) 코드 고침은 kz-static 빌드 하나에 모두 싣는다; 선언을 빌드 전에 커밋하고, scratch 점검 빌드를 감시 아래 먼저 하며 그 감시 기록을 기록 디렉터리에 옮긴다; 다시 빌드하기 전에 지금 도구로 `builds-05` 가 찾은 기본 경로 회귀를 돌린다; 빌드 뒤 다섯 빌드의 옵션 없는 경로와 마지막 옵션 회차, relations·order, 대조 선택이 든 개정 4 계획을 선언과 견준다. (b) `rules-01`: 층 예산 `tier:D07` 은 글 label 을 그대로 쓰고, 생성기는 sidecar 행 줄에 두 label 을 쓴다; 사실 label 로 계획을 다시 만들지 않고 새 선택 규칙도 만들지 않는다. (c) `rules-02`·`rules-06`: 결과 문서의 정정과 날짜 붙은 입력 표, `static_facts.md` 의 적음만; 판정 규칙 파일은 고치지 않는다. (d) `rules-03`: 문서만; 범위 규칙과 자료는 고치지 않는다. (e) `rules-04`·`rules-05`·`rules-07`: 코드로. 값 영역의 종류만으로 효과를 이름 대지 않고, success·failure 행은 return kind 가 status 인 항목만, outcome 등급의 정의는 SEMANTICS 와 RESULT_EFFECT 에 하나. (f) `semantics-1`, `semantics-3`–`semantics-10`: 코드와 검토가 쓴 꼴마다의 fixture; `semantics-2`: 만들기와 지우기를 저장 값이나 이름 공간 자료로 가르고 엔진에 함수 이름을 넣지 않는다; `semantics-11`: 부재·실패 값의 효과 호출과 성공 값의 open 효과를 따로. (g) `builds-01`: 남긴 커밋 뒤의 비교는 그대로; `builds-02`–`builds-10`: 스크립트는 새 이름의 사본으로 고쳐 새 출력에 다시 돌린다. (h) 공간: 지우는 것은 스스로 만든 scratch 뿐. (i) 둘째 에이전트가 읽기만 하며 병행한다.

### 12.1 바뀐 것 [F]

- **커밋 차례**: `fb1d9ff`(선언 §12.0 (가)와 그 스크립트; 돌리기 전), `8bb5a81`(회귀 출력, 덧붙임 §12.0 (가-1), 고친 스크립트 셋; 돌리기 전), 그리고 이 절의 커밋(다시 돌린 출력, 소스의 고침, 문서, 장부).
- **kairos 소스(MC 바이너리보다 앞섬; 빌드 전의 스냅숏)**: 새 `lib/llvm/outcome.hpp`(outcome class rule revision 1); `semantics.cpp`(return kind 행, status 항목만 success·failure, severity 상수의 행, 값을 돌려주지 않는 본문의 문서 반환은 unverified); `serialize.cpp`(RESULT_EFFECT 의 outcome 을 규칙으로, `exit-app` 은 terminal 표지가 있을 때만, lock handle rules revision 2, BASIS `extract.outcome_classes`); `ast_fn.cpp`(decay 의 조건·쉼표·안쪽 배열, switch 추적, LOOP_DEFS 의 switch 거절과 행별 깃발, guard 의 `@addr-taken`·switch 항·not-computed; 옛 `rf_class` 삭제); `finalize.cpp`(가변 인자의 사유); `relations.cpp`(revision 12: key-clearing call; revision 13: outcome 규칙); `order.cpp`(revision 5·6·7: 기다리는 스레드마다의 writer, awaited 필드, not-computed guard); `plan_gen.{hpp,cpp}`(plan rules revision 4: `fact-label=`, key-clearing 호출의 object-delete site, `event-send-other-outcome`); `result_flow.cpp`(개정 문자열).
- **fixture 와 시험**: `tests/static/fixtures/{objaccess,resultflow,looplock,relcallback}.c` 의 끝에 검토가 쓴 꼴(조건·쉼표·배열의 배열·가변 인자; 별칭 저장·주소를 받는 호출·switch·같은 callee 두 호출; switch 아래의 flag·인자만 읽는 재시도·스스로 기다리는 writer 스레드·두 곳에서 unlock 하는 함수를 거치는 release·run-status 포인터 점검·void 함수의 문서 반환; 이름으로 지우는 delete), 손으로 쓴 시험 자료 `data/looplock_fixture/framework_semantics_fixture-lock.txt`(status space 와 항목 둘), `static_golden.cpp`(대상 넷), `test_static_cli.cpp`(확인 1,151 → 1,184 [I]), `test_static_plan.cpp`(275 → 277 [I]). **golden 은 고치지 않았다**(HEAD 그대로; scratch 도구가 낸 것은 `records/review-fixes-2-20261118/golden_scratch/` 에 [I] 로 둠).
- **문서**: `kairos/docs/static_facts.md` §5w(새)와 §5t·§5u·§5v 의 날짜 줄, `plan_manifest.md`(§8 의 문장 `rules-08`, §9 새), `semantics_coverage.md` §7 의 날짜 붙은 제외 목록. 이 결과 문서 §7–§11 의 날짜 붙은 정정 19 곳. 장부의 네 행 비고(`builds-02`).
- **바꾸지 않은 것**: `include/`(스키마 개정 12 그대로), `semantics/` 의 자료와 범위 규칙, 판정 규칙 파일, 설명 스크립트, `one_build.sh`, 런타임·계측, MC 의 모든 바이너리.

### 12.2 빌드 [M]

- **MC 빌드: 하지 않았다.** kz-static `1a47abd05b8289bce90e6da8906d3b84d9bec48befc99634295d5350cd261086` 그대로(`prebuild/prebuild_regress.txt` 첫 줄; 끝 점검 `tools_check_end.txt`). 도구 사본도 만들지 않았다.
- **2026-11-19 덧붙임: 선언 §12.0 (나-1)의 빌드를 했다 [M].** MC kz-static 은 이제 `bf2ec2eedc756d72107a16532fa74f784cc7a1fd047d48edce62af20302fd460`(`--identity` 의 `tool_sha256` `73c3491c…`, `tool_source` `2b8834da…`, 선언대로). 앞 도구 `1a47abd0` 은 xz 사본으로 보존. 자세한 것은 §12.11.
- **scratch 점검 빌드**(scratchpad, LLVM Release 구성, 모든 cmake·make·ctest·실행을 감시 아래; 기록 `scratch_guard/` 의 `0NN_*.log`): 첫 make 는 새 헤더의 include 잘못으로 실패(rc 2), 고친 뒤 rc 0. 가장 큰 dip 은 `scratch_guard/` 의 `dip=` 줄에 있다(첫 kz-static 빌드 144,699,392 B). 빌드 트리는 지웠다. scratch 출력은 근거가 아니다.

### 12.3 시험 [M]/[I]

- **MC 의 전체 ctest: 돌리지 않았다**(빌드가 없음). MC 의 지금 `kz_lint`(다시 빌드하지 않은 `3d4d5a8f…`)로 바뀐 소스 트리의 lint 만 돌렸다 [M]: lint 시험 여덟 통과(`mc_lint_present_tool.log`: selftest 114 경우 실패 0, 규칙 일곱 clean). 주의: fixture 넷과 시험 자료가 바뀌었으므로, 지금 MC 의 (앞판) 시험 프로그램으로 ctest 를 돌리면 golden 시험이 통과하지 않는다 [I: 돌려 보지 않음]. 선언된 빌드에서 첫 ctest 의 실패를 이름으로 적고 golden 을 갱신하는 차례다.
- **2026-11-19 덧붙임 [M]**: 선언된 빌드의 첫 ctest 는 268 가운데 선언한 16 개만 실패, golden 17 은 scratch 의 것과 같은 바이트, golden 뒤 전체 ctest 268 실패 0·건너뜀 1, `test_static_cli` 1,184·`test_static_reasons` 763·`test_static_plan` 277·`test_core_schema` 34,017, lint 114 경우 실패 0·규칙 일곱 clean, 단위 점검 17/17·7/7(§12.11 (나)).
- **scratch 의 정적·lint 시험** [I, 근거 아님; `scratch_ctest_static.log`, 다시 만든 빌드의 `scratch_ctest_static_2.log`]: scratch golden 으로 `test_static_cli` 1,184, `test_static_reasons` 763, `test_static_plan` 277 확인 통과, `lint selftest` 114 경우 실패 0, lint 규칙 일곱 clean. scratch 에서 만들지 않은 시험 프로그램 아홉(`test_static_{expr,flow,recipe,gitrev,unroll,wrappers,link,elf32}`, `test_trace_static_closure`)은 돌지 않았다.
- scratch golden 이 HEAD 와 다른 것 17(`golden_scratch.sha256`): `relations`, `relations_np`, `relpaths_np`, `relsound_np`, `relincomplete_np`, `relincomplete_cut_np`, `relcallback_np`, `order_np`, `objaccess`, `objaccess_off`, `semantics_np`, `resultflow_np`, `resultflow_order_np`, `looplock_np`, `looplock_order_np`, `plangen_np.plan.golden`, `planv4_np.plan.golden`. 선언된 빌드의 첫 ctest 에서 실패할 시험은 이 golden 들의 `static_golden_*` 열다섯과 `test_static_plan`·`test_static_reasons`(golden 을 읽음) [I].
- 저장소의 단위 점검 [M]: `env/tools/tests/run_unit.sh` 17/17(`unit_tools.txt`), `env/redesign/step2a_tests_unit.sh` 7/7(`unit_step2a.txt`).

### 12.4 바이트 동일성 [M]

| 비교 | 결과 |
|---|---|
| 지금 도구(`1a47abd0`)의 기본 경로 회귀 (`prebuild/prebuild_regress.txt`) | 71 OK: 도구 확인; 다섯 빌드의 T8CF `relations.txt`·`keys.txt` = `ll_relcheck`, `order.txt`·`edges.tsv` = `ll_order`; 마지막 옵션 회차의 `order.txt`·`edges.tsv`·`cdr.txt`·`relations.txt`·`keys.txt` = `ll_final_order`(relations 는 다시 뽑기의 것과도 같음); 로그 빈 파일 |
| 중간 회차의 섹션, count 도 (`recheck/recheck_rounds.txt`) | 15 OK: 글과 count 가 같은 섹션 39/40/42(회차 objacc/sem/rf; c5 의 rf 회차 43), 다른 것 STRINGS·BASIS·FACT_EXPRS 뿐 |
| 계획 짝 열하나 × 다섯 빌드, rules 줄 허용 없이 (`recheck/recheck_plans.txt`) | 55 “only declared differences” |
| manifest 단계의 측정 다시 (`recheck/recheck_measure.txt`) | 다섯 빌드 rc 0, 남긴 `compare/measure_<b>.txt` 와 같은 바이트 |

- 누적 목록과 등록부(끝 점검): `ledger_check_end.txt` 135 행 실패 0, `tools_check_end.txt` 49 행 실패 0(deep). 등록부(`TOOLS.tsv`)는 바꾸지 않았다(도구가 바뀌지 않음). 새 장부 행 `OBS_rf2_pre_relcheck`, `OBS_rf2_pre_order`, `OBS_rf2_pre_final_order`.
- 빌드 뒤의 비교(다섯 빌드의 옵션 없는 경로, 마지막 옵션 회차, relations, order, 개정 4 계획)는 **하지 않았다**(빌드가 없음).
- **2026-11-19 덧붙임 [M]**: 빌드 뒤의 비교를 했다. 열한 단계 모두 OK, 쌍둥이 비교는 덧붙임 §12.0 (나-2) 뒤 350/350 같음, 보고 275 개 모두 선언과 같음(§12.11 (다)).

### 12.5 측정 [M]/[I]

- [M] 지금 도구의 출력은 앞 단계의 것과 같다(위 표): 그 도구가 낸 relations·order 의 수는 §10.5·§11.5 의 수 그대로다(정정은 그 수의 **읽기**에 대한 것이다).
- [I, 근거 아님] scratch 점검 빌드의 kz-static 으로 **남긴 facts**(T8CF 와 앞판 도구의 마지막 옵션 회차)를 읽은 것(`scratch_measure.txt`; 다시 뽑지 않았으므로 추출 쪽 고침의 효과는 없음):
  - c5_641·c5_642: 경로 A 대조 집합 (나) 15 → 2(A20–A32 가 빠짐; `key clearing:` 줄 26), 그 13 행의 order.static `unordered` → `unknown`(짝이 없어짐). c1 두 빌드와 r2_core: 행 수·대조 집합 (가) 1/1/0·(나) 5/5/4 그대로, order.static 이 바뀌는 행 0.
  - HB-await 간선 줄(마지막 회차 facts): c1 두 빌드 120 → 284, r2_core 64 → 116, c5 0. T8CF facts 에서는 0 → 0.
  - c1 의 `OS_common_record_t.name_entry` namespace: creators 53 → 47, key-clearing calls 6(조회 행과 같은 partition 이 아니어서 행의 줄은 0).
- **재지 못한 것**(다시 뽑기가 필요): 고친 outcome 규칙 아래의 handling label 과 그 움직임(lead 의 판단 (e)), exit-app·terminal·event 의 수, SEMANTICS 의 검증된 행 수, LOCK_HANDLES 의 핸들별 행(722 의 나뉨이 고쳐지는지), decay 의 상태 움직임, 개정 4 계획의 site·need 수.
- **2026-11-19 덧붙임 [M]**: 위의 것을 고친 도구의 출력에서 쟀다(§12.11 (라)). [예비] 수 가운데 c5 대조 집합 15 → 2, HB-await 120 → 284·64 → 116 은 그대로 확인되었다. handling label 은 다섯 빌드 어디에서도 한 행도 움직이지 않았다.

### 12.6 공간 [M]

- **2026-11-19 덧붙임 [M]**: 선언된 빌드의 공간은 §12.11 (마). 하한을 넘지 않았다(이 실행의 감시 기록 38 의 가장 낮은 여유 974,893,056 B). 남긴 새 출력 11 디렉터리 176,307,815 B, 도구 사본 47,373,676 B. 지운 것은 scratch 뿐(앞 에이전트의 scratch 566 MB 전부; 감시 기록은 기록 디렉터리로 옮김).

| 시점 (UTC, 2026-11-18) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 시작 | 1,523,216,384 | (둘째 에이전트의 scratch 빌드가 병행: 여유가 이 묶음과 무관하게 오르내림) |
| 09:24:42 | 1,354,162,176 | 회귀 스크립트를 쓴 뒤 |
| 09:26:12 | 1,322,921,984 | 기본 경로 회귀 끝(dip 30,969,856; 출력 30,641,951 B 를 남김) |
| scratch 점검 빌드 중 | 가장 낮은 값은 `scratch_guard/*.log` 의 `lowest=` | 첫 kz-static 빌드 dip 144,699,392 |
| scratch 빌드를 지운 뒤 | 1,466,961,920 | |

- 하한(314,572,800)을 넘지 않았다. 감시 기록: `df_prebuild_regress.log`, `df_recheck_{rounds,plans,measure}.log`, `scratch_guard/`(모두 `verdict=completed`).
- 남긴 출력(장부 행): `rf2_pre_relcheck` 7,630,029, `rf2_pre_order` 7,058,482, `rf2_pre_final_order` 15,953,440 B. 지운 것: 스스로 만든 scratch(점검 빌드 둘, scratch 실행의 출력)뿐.
- **2026-11-18 덧붙임: 바이트가 같은 점검 디렉터리 열둘의 회수 (§12.10 2 항 *공간*) [M].** 기록: `../records/reclaim-20261118/`. 빌드·추출·부팅은 없다.
  - *대상*: `/home/user/work/redesign/obs1/` 아래 relations 점검 디렉터리 일곱(`orderrev1_relcheck`, `orderrev1b_relcheck`, `schema12_relcheck`, `objacc_relcheck`, `sem_relcheck`, `rf_relcheck`, `ll_relcheck`)과 order 점검 디렉터리 다섯(`schema12_order`, `objacc_order`, `sem_order`, `rf_order`, `ll_order`). 견준 기준(남긴 디렉터리): 앞의 셋 relations 는 `relrev7_check`, `schema12_order` 는 `order_rev1b`, 나머지 relations 넷은 `revfix_relcheck`, 나머지 order 넷은 `revfix_order`(`pairs.tsv`).
  - *같음의 확인*: `identity_listing.py` 가 후보와 기준의 모든 파일의 sha256 을 새로 계산했다. 열두 쌍 모두 후보의 파일 15 가 기준의 같은 상대 경로 파일과 크기·sha256 이 같고, 다른 파일 0, 후보에만 있는 항목 0, 기준에만 있는 파일 0, 정규 파일이 아닌 항목 0, 기준과 inode 를 나눠 가진 파일 0 이다(`identity_summary_before.txt`: 180 파일, 89,207,176 B, 할당 89,423,872 B; 파일별 줄은 `listing_before_<디렉터리>.txt`). §12.10 의 [예비] 89,207,176 B 와 같다. 스크립트 자체는 꾸민 트리로 점검했다(`selftest.txt`: 같은 것 하나만 대상, 다른 바이트·남는 파일·심볼릭 링크·빈 디렉터리·하드 링크의 다섯은 대상 아님).
  - *장부*: 지운 디렉터리의 행과 그 sha256 목록은 그대로 두고, 행의 root 를 기준 디렉터리로 바꾸었다. `ledger_check.sh` 는 root 가 없으면 그 행을 실패로 세고(`env/tools/ledger_check.sh:22`) 행의 목록을 root 안에서 `sha256sum -c` 로 읽으므로(`:24`), 이렇게 하면 지운 디렉터리의 목록이 이름 댄 모든 바이트를 점검이 계속 읽는다. local 칸은 `removed 2026-11-18; bytes in <기준 행>`, note 는 지운 디렉터리·파일 수·바이트·기준 행·목록 파일과 앞의 note 다. 이 규약을 `records/LEDGER.tsv` 의 머리 주석에 적었다(`ledger_rewrite.py`, `ledger_rewrite.txt`: 행 12 를 바꿈, 주석 4 줄을 더함, 나머지 130 줄은 그대로; 바꾸기 전에 행의 목록과 새 목록의 파일·sha256 이 같음을 확인). `ledger_check.sh`: 시작 때 135 행 0 실패(`ledger_check_start.txt`), 고친 장부를 지우기 전에 135 행 0 실패(`ledger_check_proposed_before_removal.txt`), 지운 뒤 135 행 0 실패(`ledger_check_after_removal.txt`; 행마다의 결과가 시작 때와 같음). 목록·스크립트·고친 장부는 지우기 전에 커밋했다(`4f82752`).
  - *지우기*: `reclaim.sh` 가 디렉터리마다 지우기 바로 앞에 기준과 `diff -rq` 로 다시 견준 뒤 지웠다(`reclaim.txt`: 대상 12, 지움 12, 실패 0).
  - *여유(df -B1 /)*: 시작 1,463,078,912(`free_start.txt`), 지우기 직전 1,462,460,416, 직후 1,552,175,104(+89,714,688; `reclaim.txt`), 끝 1,552,138,240(`free_end.txt`).
  - *건드리지 않은 것*: 추출 회차는 하나도 건드리지 않았다(옵션 없는 회차와 중간 옵션 회차는 바이트 동일이 아니다, §12.10 2 항). `rf2_pre_*` 셋과 그 밖의 남긴 디렉터리도 그대로다. 도구 등록부는 바뀌지 않았다(`tools_check_after_removal.txt`: 49 행 0 실패, 스트림 점검 없이).
  - *잃은 것과 뒤 단계가 알 것*: 지운 파일의 수정 시각과 mode 는 디스크에 없다(목록의 끝 두 칸에 적어 둠). 앞 단계의 비교 스크립트 가운데 지운 디렉터리를 경로로 읽는 것(2026-11-16 단계들의 `compare.sh`·`rerun.sh`·`order.sh`, 이 묶음의 `prebuild/prebuild_regress.sh`)은 그대로는 다시 돌지 않는다. 남긴 출력은 바뀌지 않았다. 뒤 단계가 `ll_relcheck`·`ll_order` 의 바이트와 견줄 때는 `revfix_relcheck`·`revfix_order`(장부 행 `OBS_revfix_relcheck`·`OBS_revfix_order`)나 `rf2_pre_relcheck`·`rf2_pre_order`(같은 바이트임은 `../records/review-fixes-2-20261118/prebuild/prebuild_regress.txt` 의 71 OK)를 쓴다.

### 12.7 지적마다: 무엇이 틀렸고, 무엇을 바꿨고, 무엇을 다시 돌렸는가

“소스” = kairos 소스에 고쳤고 scratch 로 시험했으나 **MC 도구에는 아직 없음**(§12.0 (나)). 

| id | 무엇이 틀렸나 | 무엇을 바꿨나 | 다시 돌린 것과 증거 |
|---|---|---|---|
| `rules-01` | 계획 생성기가 한 실행에서 A21 에 label 둘(sidecar R, 값 요청 T?)을 주고, 층 차례가 글 label 에 기대는데 §11.5 가 적지 않았다 | 문서: §9.5·§11.5·§11.8 의 정정. 소스: sidecar 행 줄에 `label=` 과 `fact-label=`(plan rules revision 4); 층 차례는 그대로(lead (b)) | `mv_plans_v4/c1_pre_tx/plan.side:73`, `plan.req:5` 대조; scratch fixture golden 의 `A1 … label=R fact-label=T?`(`golden_scratch/planv4_np.plan.golden`) [I]. 다섯 빌드의 계획은 다시 만들지 않음 |
| `rules-02` | A21 의 기초 4 가 여전히 서지 않는데(개정 9 의 두 줄이 남음, result flow 줄을 읽는 쪽 없음) §7–§11 이 적지 않았다 | 문서: §9.8 의 정정, `static_facts.md` §5u 의 날짜 줄(개정 9 의 줄을 두는 까닭). 판정 규칙 파일은 고치지 않음(lead (c)) | `ll_final_extract/c1_pre_tx/c1_pre_tx/NP/relations.txt:635-636`, `:647-648`; `env/obs1/explain.py:336-354` |
| `rules-03` | 이름 공간 자료가 층이 다루는 이름 있는 종류의 전부가 아니고(종류 macro 에 기댐), §8.5 의 11 은 항목 수이며, §11.8 이 계획되지 않은 대조 행의 까닭을 틀리게 적었다 | 문서: §8.5·§11.8 의 정정, `semantics_coverage.md` §7 의 제외 목록. 규칙·자료는 고치지 않음(lead (d)) | `framework_namespaces_osal-f34759d.txt:100-102`; `env/tools/semantics_scan.py:585,638,640`; `mv_plans_v4/c1_pre_tx/plan.side:1105-1106` |
| `rules-04` | 엔진이 자료의 종류 `run-status` 를 효과 `exit-app` 으로 바꿨고, return kind none 인 `CFE_ES_RunLoop` 에 success·failure 행을 썼다; §9.5 의 c5 0/0 에 설명이 없었다 | 소스: `exit-app` 은 terminal 표지가 있는 호출에만; success·failure 행은 status 항목만(그 밖은 `none` 칸). 문서: §8.5·§9.5 의 정정, §5w | scratch fixture [I]: `is_running`(포인터 점검, return=none)의 행이 `framework none … returned constant (…no outcome class)`, application exit 0(`test_static_cli` 의 새 확인). c1 의 `CFE_ES_ExitApp` 호출 14 는 모두 terminal-op(마지막 회차 facts 의 calls 절)이라 c1 의 exit-app 은 남을 것 [I]. 다섯 빌드는 재지 않음 |
| `rules-05` | RESULT_EFFECT 의 outcome 등급이 엔진의 수 범위였고(근거 없는 부분 포함) SEMANTICS 의 failure 칸과 다른 정의였다 | 소스: outcome class rule revision 1(`outcome.hpp` 하나; status-space 행의 성공 상수와 severity mask; 없으면 이름 붙은 대체; BASIS 에 규칙 이름); relations rules revision 13. 문서: §5w | scratch fixture [I]: `extract.outcome_classes = outcome class rule revision 1 … status spaces of these facts 1: […success 0, severity mask 3221225472]`. **handling label 의 재측정은 하지 못했다**(다시 뽑기 필요, §12.9) |
| `rules-06` | 판정 규칙 8.2 의 입력 셋((다)의 종류별 writer, (라)의 생성 쪽 종류 값, (나)의 이름 공간 한도)이 아직 없는데 §7–§11 이 열린 것으로 적지 않았다 | 문서: §12.9 의 날짜 붙은 입력 표(8.7 을 줄마다), §7.8·§11.8 의 정정. 판정 규칙 파일은 고치지 않음 | `relations.txt:650`; `mv_plans_v4/c1_pre_tx/plan.side:1059-1062,1104`; `plan.kzs:845,932`; `framework_namespaces_osal-f34759d.txt:59` |
| `rules-07` | severity 상수가 모두 failure 칸에 있었고, void 함수의 문서 반환이 collated 로 세어졌다 | 소스: severity 상수는 `status-space none … [severity constant …]`; 본문이 값을 돌려주지 않는 함수의 문서 반환은 `unverified` + `doc-declaration mismatch`. 훑기 스크립트는 고치지 않음(범위 규칙의 개정이 뒤 단계) | scratch fixture [I]: `exit_like` 의 행(`documented-return … unverified … (doc-declaration mismatch: …)`), `!has("status-space failure")` |
| `rules-08` | “No API or kind name is in the generator” 가 글자 그대로는 틀렸다(`memchr`·`strnlen`·`kz_rt_posix_create`) | `plan_manifest.md` §8 의 문장을 좁히고 날짜 붙은 괄호 | `plan_gen.cpp` 의 그 세 이름(`grep`); framework API 이름은 없음 |
| `builds-01` | 결과 흐름 단계가 선언 밖의 차이에서 멈추지 않았고(측정이 덧붙임 전에 돎), 고친 스크립트를 커밋 전에 돌렸다; §9.0a 는 멈췄다고 적었다 | §9.7 에 어긋남 둘, §9.0a 의 글에 날짜 붙은 정정. 남긴 커밋 뒤의 비교는 그대로(lead (g)) | `df_compare_extract_on.log` 의 끝 21:07:01.937Z, `df_compare_measure.log` 의 시작 21:07:01.947Z, 커밋 `b2110c6` 21:08:53Z |
| `builds-02` | §9.4·§10.4 와 장부 비고가 STRINGS 를 “같은 레코드 글”로 셌다(count 가 다름); 선언 §10.0 의 중간 회차 비교가 STRINGS 를 빠뜨렸다 | 덧붙임 §12.0 (가-1); `recheck/rounds_cmp_2.py`(count 도 견줌); §9.4·§10.4 의 정정; 장부 네 행의 비고 | `recheck/recheck_rounds.txt` 15 OK |
| `builds-03` | 결과 흐름 단계의 scratch 점검 빌드에 감시 기록이 없다 | §9.7 에 적음. 이 묶음은 scratch 의 모든 cmake·make·ctest·실행을 감시 아래 두고 기록을 남김 | `records/result-flow-20261116/` 에 `df_scratch_*` 없음; 이 묶음 `scratch_guard/` |
| `builds-04` | 세 단계의 scratch 추출과 facts 보기에 감시 기록이 없다 | §7.7·§8.7·§10.7 의 한 줄씩 | 그 단계들의 `df_scratch_*` 의 `cmd=`(cmake·make 뿐) |
| `builds-05` | manifest 단계가 kz-static 을 바꾸고 relations·order 의 기본 경로 회귀를 돌리지 않았다 | 선언 §12.0 (가)와 `prebuild/prebuild_regress.sh`; §11.7 의 정정 | `prebuild/prebuild_regress.txt` 71 OK; 장부 행 셋 |
| `builds-07` | 선언들의 “lint 규칙 여덟”은 시험 여덟(규칙 일곱과 selftest)이다 | §9.3 의 한 줄 | `records/loop-lock-20261116/ctest_counts.log` 의 `lint <rule>: clean` 일곱 줄 |
| `builds-08` | `compare_plans.py` 의 rules 줄 허용이 선언보다 넓었다(쓰인 적 없음) | `recheck/compare_plans_2.py`(두 무늬를 뺌) | `recheck/recheck_plans.txt` 55 “only declared differences” |
| `builds-09` | manifest 단계 `compare.sh measure` 의 빌드별 `rc=$?` 가 늘 0 이다 | `recheck/recheck.sh measure`(상태를 먼저 잡음); §11.8 의 정정 | `recheck/recheck_measure.txt`: 다섯 빌드 rc 0, 같은 바이트 |
| `builds-10` | 객체 접근 단계의 “110 OK” 에 빌드 직후의 남긴 출력이 없다 | §7.2 의 정정(`resume_check.txt` 와 그 시각을 가리킴) | `records/object-access-20261116/resume_check.txt` |
| `semantics-1` | HB-await 간선의 writer 스레드가 spawn entry 로 가는 간선에서 틀렸다(스스로 기다리는 writer 스레드를 모두에게서 뺌) | 소스: order rules revision 5(기다리는 스레드마다의 writer, 짝마다 간선); fixture `parent_entry`. 문서: §10.5 의 정정, §5w | scratch [I]: fixture 의 `awaited:` 줄 `writers 4 (… parent_entry (spawn-entry) 2, task_entry_point (spawn-entry) 4)`; 남긴 마지막 회차 facts 에서 HB-await 줄 120 → 284(c1), 64 → 116(r2)(`scratch_measure.txt`). 간선별 대조는 하지 않음 |
| `semantics-2` | c5 의 대조 13 행이 지우는 호출을 creator 로 가져 (나)에 들었다 | 소스: relations rules revision 12(WRITERS 의 저장 값이 상수뿐인 쓰기에만 닿는 사용은 key-clearing call; 함수 이름을 보지 않음), 생성기가 그 호출을 object-delete site 로; fixture `sem_delete`. 문서: §11.5·§11.8 의 정정 | scratch [I]: c5 두 빌드 (나) 15 → 2, A20–A32 가 빠짐(`scratch_measure.txt`); fixture golden 의 `key clearing:` 줄. **개정 4 계획의 대조 집합은 다시 만들지 않았다** |
| `semantics-3` | 본문이 있는 가변 인자 함수의 인자 탈출에 “본문 없음” 사유가 붙었고 §7.5·§7.8 이 그렇게 분류했다 | 소스: `finalize.cpp` 의 사유를 가름. 문서: §7.5·§7.8 의 정정 | scratch fixture [I]: `Ent.n_var … passed to log_text: an argument beyond the named parameters of a callee defined in the closed program …`. 이 사유는 옵션 없는 경로에도 쓰이므로 기본 경로의 facts 가 달라질 수 있다 [I: 재지 않음] |
| `semantics-4` | 조건·쉼표 연산자 아래의 decay 와 배열의 배열의 한 줄이 탈출 행을 얻지 못했다 | 소스: `note_array_decay`·`decayed_target_arg`; fixture `decay_cases` | scratch fixture [I]: `Ent.n_cond`·`Ent.n_comma` `per-field=DISABLED … escape 1`, `Ent.n_grid` 는 out-pointer 호출(`count.array_decay = out-pointer calls 8, address-escape hits 4, …`). 다섯 빌드의 상태 움직임은 재지 않음 |
| `semantics-5` | guard 행이 주소 잡힌 지역의 정의를 하나뿐인 도달 정의로 적었다 | 소스: 그런 지역은 `x@addr-taken`(값을 넣지 않음), 행에 `addr-taken-local`; fixture `guard_alias` | scratch fixture [I]: `guard [report] shared cond (eq local:rc@addr-taken 0) {addr-taken-local}` 둘 |
| `semantics-6` | switch 아래의 flag 정의가 술어 `1` 의 recognised 를 냈고, switch 의 case 아래 호출에 guard 가 없었다 | 소스: loop flag predicate rules revision 2(거절), guard 의 `top` 항과 switch 사용 행; fixture `wait_switch`, `guard_switch` | scratch fixture [I]: `in wait_switch refused-unresolved {}`; `guard [report] unique cond (top a case of the switch at 254:11 on …)` 셋 |
| `semantics-7` | CF 재시도 대기의 awaited 필드가 조회의 out 인자와 열쇠였다 | 소스: order rules revision 5(호출 인자와 `addr` 아래는 awaited 아님); fixture `retry_fields` | scratch fixture [I]: `awaited: not named: the predicate reads no field of its own (… Named.id, Named.name)`. ES 루프의 awaited 는 `EntryFunc` 만 남고 `AppId`(호출 인자)는 빠진다 [I: 간선 수의 변화에 들어 있음] |
| `semantics-8` | LOCK_HANDLES 가 다른 family 의 행과 잠금 연산이 아닌 호출의 인자에 핸들을 적었다; 722 는 383 + 337 + 2 다 | 소스: lock handle rules revision 2(사슬의 호출이 모두 찾아질 때만 핸들); fixture `finish_with`. 문서: §10.5 의 정정 | scratch fixture [I]: `finish_with … not-static {} lock #0`(핸들 없음), `the calls of the chain are not all located (1 of 4; …)`. **c1 의 722 의 나뉨은 다시 재지 못했다**(다시 뽑기 필요) |
| `semantics-9` | LOOP_DEFS 정의 행의 깃발이 지역마다 찍혔다 | 소스: 행별 깃발 | scratch fixture [I]: `rc@172:7 before-loop … recognised flag {} value -5`, 몸 안 행만 `{under-lock}` |
| `semantics-10` | guard 가 호출 결과를 callee 이름만으로 구분했다 | 소스: 같은 callee 의 두 호출을 담은 guard 는 `not-computed`, order 는 unknown; fixture `guard_twice`. **마디에 site 를 싣는 것은 하지 않았다**(§12.8) | scratch fixture [I]: `guard [report] not-computed cond (and (not (ne (call-result obj_impl (seq 0)) 0)) (ne (call-result obj_impl (seq 1)) 0))` |
| `semantics-11` | 관계 1 의 `event-send` planned 3 가운데 둘은 성공 값의 open 효과다 | 소스: 실패·부재 outcome 의 효과만 `event-send`, 그 밖은 `event-send-other-outcome`. 문서: §11.5 의 정정 | `mv_plans_v4/c1_pre_tx/plan.kzs:916,921`. 다섯 빌드의 계획은 다시 만들지 않음 |


**2026-11-19 덧붙임: 빌드 뒤 다시 돌린 증거 [M].** 위 표의 “소스” 고침은 이제 MC 도구(`bf2ec2ee`)에 있다. 모든 수는 `../records/review-fixes-2-mc-20261119/` 의 남긴 출력에서 왔다(`measure/measure_<b>.txt`, `compare/`). 빌드 차례는 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642.

| id | 다시 돌린 것과 증거 |
|---|---|
| `rules-01` | `rf2_plans_v4/<b>/plan.side` 의 행 줄에 `label=` 과 `fact-label=`(예: c5_641 `row bbcaec5d278f30b8 A1 … label=T? fact-label=T?`); 층 차례와 선택은 바뀌지 않음(c1·r2 의 대조 예산 줄 5/5/4 그대로, `measure_<b>.txt`) |
| `rules-04` | application exit 행 305/305/86/0/0, terminal 159/159/158/169/165, event send 1073/1067/307/589/589 로 앞 도구와 같음(`measure_<b>.txt` 의 `result effect rows`) |
| `rules-05` | handling label 이 움직인 행 0/0/0/0/0(행 233/233/180/133/131 모두 짝지어짐); 머리 줄(예: c1_pre_tx `H 14 J 14 R 9 T 7 T? 47 none 142`)이 앞 도구와 같음 |
| `rules-07` | SEMANTICS 행 1773 → 1931(c1 둘), 1476 → 1613, 795 → 910, 786 → 899; framework 의 collated/행 1578/1690 → 1736/1848(c1), 1303/1395 → 1440/1532(r2), 639/716 → 752/831, 632/707 → 743/820; P0 과 status-space 는 그대로. 보기의 semantics 줄 32→190 등은 `compare/extract_final_view_<b>.txt` |
| `semantics-1` | HB-await 간선(마지막 회차) 120 → 284(c1 둘), 64 → 116(r2), 0(c5); writer 행위자마다: c1 `main (process)` 120 → 60, `BP_AppMain`·`CFE_ES_BackgroundTask`·`CFE_ES_TaskMain`·`CFE_TIME_TaskMain` 0 → 56 씩; r2 `main` 64 → 32, `CFE_ES_BackgroundTask`·`CFE_ES_TaskMain`·`CFE_TIME_TaskMain` 0 → 28 씩. T8CF facts 는 0 → 0 |
| `semantics-2` | c5 두 빌드의 대조 집합 15 → 2(`rows=2 planned=2 complete=yes`), `key clearing:` 줄 26(호출 하나: `cfe_sb_priv.c:123:7` `CFE_SB_CleanUpApp` → `CFE_SB_DeletePipeWithAppId`). c1·r2 는 행에 걸리는 key-clearing 호출 0, 이름 공간 `OS_common_record_t.name_entry` 에 6·5(§12.11 (라)의 소스 대조) |
| `semantics-3` | 옵션 없는 회차에서 c5 두 빌드의 FACT_ATYPE 5 레코드(보기 5 줄)만 다름; c1·r2 의 옵션 없는 facts 는 BASIS 1 레코드 밖에 같음(`compare/extract_off_facts_sections_<b>.txt`) |
| `semantics-4` | FACT_ATYPE 레코드 수는 그대로(36635 등), 마지막 회차의 atype 줄 65/65/22/68/69 이 글로 바뀜; extract.txt 의 location 상태 줄은 한 줄도 바뀌지 않음(상태 움직임 0) |
| `semantics-5`·`-6`·`-10` | RESULT_FLOW 행 12916 → 13033(c1_pre_tx; guard 행 3924 → 4041), 9622 → 9645(r2), 5568 → 5701(c5_641); 보기의 조건·깃발 칸 853 쌍(c1_pre_tx) |
| `semantics-7` | c1·r2 의 `awaited:` 줄 둘씩이 바뀜(`compare/final_order_order_<b>.txt` 의 `row:awaited`) |
| `semantics-8` | LOCK_HANDLES 행 수 1573/1573/967 그대로, 핸들을 찾지 못한 행 115 → 187(c1 둘), 83 → 97(r2); 보기의 lockhandles 줄 243/243/97 이 바뀜 |
| `semantics-9` | LOOP_DEFS 레코드 4/7/4/1/1 이 바뀜(행 수 그대로) |
| `semantics-11` | 개정 4 계획의 need: c1_pre_tx `event-send` planned 3 → 2, not-planned 2 → 3, `event-send-other-outcome` planned 0 → 3; c1_post_tx planned 3 → 1, not-planned 2 → 4, other-outcome 0 → 3 |
| `builds-05` 등 비교 지적 | 빌드 뒤의 비교는 선언의 스크립트로(§12.11 (다)); 덮어쓴 보고 없음 |

### 12.8 계획과 다르게 한 것

- **선언된 kz-static 빌드와 그 뒤의 다섯 빌드를 하지 않았다.** lead 의 판단 (a)는 이 묶음에서 빌드 하나와 그 뒤의 비교까지를 요구한다. 이 세션에서 한 것은 빌드 전의 점검, 문서 고침, 소스의 고침과 fixture, scratch 점검까지이고, 선언(허용되는 차이의 기록 종류·칸별 목록, 다시 링크되는 바이너리, 첫 ctest 의 실패 이름, 공간 필요량)과 그것에 맞춘 비교 스크립트를 쓰기 전에 멈췄다. 까닭: 고침 열다섯이 기본 경로의 출력도 바꾸어(relations·order 의 개정 줄과 행, 가변 인자 사유 글) 다섯 빌드의 허용 차이를 출력 종류마다 새로 선언하고 견줄 스크립트를 써야 하는데, 그 선언의 [I] 를 댈 scratch 의 다섯 빌드 다시 뽑기까지 이 세션의 작업 한도 안에서 끝낼 수 없었다. 반쯤 선언된 채 MC 를 덮지 않으려고 빌드 앞에서 멈췄다. MC 도구와 앞의 출력은 하나도 바뀌지 않았다.
- **소스가 MC 바이너리보다 앞선 채 커밋되었다**(선언 전의 스냅숏; 루프·잠금 단계의 `08760d6` 과 같은 꼴). fixture 가 바뀌었으므로 지금 MC 의 시험 프로그램으로 돌리는 ctest 는 통과하지 않을 것이다 [I].
- **scratch golden 을 한 번 잃고 다시 만들었다.** scratch 도구로 쓴 golden 을 기록 디렉터리로 옮기는 명령이 경로를 잘못 써 복사에 실패했는데, 이어진 명령이 작업 트리의 golden 을 HEAD 로 되돌리고 scratch 빌드를 지웠다. scratch 빌드를 다시 만들어(감시 아래, `scratch_guard/021-024`) golden 을 다시 내어 `golden_scratch/` 에 두었다. 잃은 것은 근거가 아닌 scratch 출력이다.
- **`semantics-10` 은 약한 쪽으로 고쳤다**: call-result 마디에 호출 site 를 싣지 않고, 같은 callee 의 두 호출을 담은 guard 를 결정되지 않음으로 둔다(검토가 “빌드 전까지”로 든 방법). site 를 싣는 것은 식의 이름을 바꾸어 RESULT_FLOW 의 모든 조건 글과 그것을 읽는 쪽(order, 계획 생성기)을 함께 바꿔야 한다.
- **`semantics-5`·`semantics-6` 의 guard 는 통째로 “결정 안 됨”이 아니라 모르는 항으로 두었다.** 처음에는 주소 잡힌 지역을 읽는 guard 를 not-computed 로 썼는데, 그러면 `if (f(&x) == OK && x != 0)` 꼴(첫 경로의 `CFE_ES_TaskEntryPoint` 가 이 꼴이다)의 호출자 쪽 사용이 모두 unknown 이 되어 HB-await 간선이 사라졌다(scratch fixture 에서 18 → 0). 예산 값이 다른 항을 거짓으로 만들면 guard 는 거짓이므로, 그 지역과 switch 의 case 를 모르는 값의 항으로 두고 셋값 논리로 읽게 했다.
- **`semantics-8` 은 대체 규칙을 없애지 않고 조건을 달았다**(사슬의 호출이 모두 찾아질 때만). 검토의 두 꼴은 모두 잘린 사슬에서 나왔다(`serialize.cpp` 의 걸음 찾기가 한 함수가 다음 함수를 여러 곳에서 부르면 멈춤). 모두 찾아진 사슬에서도 그 값이 그 family 의 잠금을 가리킨다는 증명은 아니다(§5w 의 남은 한계).
- **`rules-07` 의 훑기 쪽**(문서 반환이 void 선언 함수에 있으면 자료에 mismatch 로 적기)은 하지 않았다: 범위 규칙과 자료를 고치지 않는다는 판단 (d). 엔진이 facts 로 같은 것을 적는다.
- **개정 번호**: relations 는 바탕 줄 9 → 12, result flow 줄 10 → 13, 핸들 줄 11 그대로; order 는 바탕 줄 2 → 5, result flow 줄 3 → 6, 루프·핸들 줄 4 → 7. 바탕 줄의 번호가 옵션 줄의 옛 번호보다 커지도록 하나의 차례로 올렸다.
- 기본 경로 회귀에 다시 뽑기를 넣지 않았다(선언 §12.0 (가)대로).

### 12.9 열린 것

- **이 묶음의 남은 차례**(다음 실행이 이어서 할 것): (1) scratch 점검 빌드로 다섯 빌드를 옵션 없이·마지막 옵션 회차로 다시 뽑아 허용 차이의 [I] 를 잰다(기본 경로: relations·order 의 개정 줄, `uses:`·`summary A`·`key clearing:`·`dependency:` 줄, `awaited:` 줄, c5 의 13 행의 값, 가변 인자 사유 글이 든 A-TYPE 행; 옵션 경로: SEMANTICS·RESULT_FLOW·RESULT_EFFECT·LOOP_DEFS·LOCK_HANDLES·BASIS 의 행). (2) 선언(§12.0 (나))과 비교 스크립트를 커밋한다. (3) kz-static `1a47abd0` 을 사본하고 MC 에서 빌드, 첫 ctest 의 실패(golden 열일곱의 시험과 `test_static_plan`·`test_static_reasons` [I])를 이름으로 확인, golden 을 갱신해 `golden_scratch/` 와 `cmp`. (4) 다섯 빌드의 옵션 없는 경로와 마지막 옵션 회차, relations, order, 개정 4 계획과 대조 선택을 선언과 견준다. (5) handling label 의 재측정과 움직인 행의 목록, c5 두 빌드의 대조 집합, HB-await 의 간선별 대조, LOCK_HANDLES 의 핸들별 행, exit-app·terminal 의 수. 그때까지 §12.7 의 “소스” 고침은 도구에 없다.
- **판정 규칙 8.7 의 입력 표, 2026-11-18 의 상태**(8.7 을 줄마다; “준 것”은 그 출력 줄):

| 8.7 의 입력 | 쓰는 곳 | 2026-11-18 의 상태 | 지금 맡은 단계 |
|---|---|---|---|
| 이름 공간 항목(종류, 표, 키 필드, 종류 선택 값, 비교 함수, 한도와 설정 줄, 부재 상수와 경로, 조회·생성·삭제·회수 API, 판, 원문 줄) | (가), (나), 기초 3, (마) | **일부만.** 자료 `framework_namespaces_*.txt` 다섯이 있음(종류·표·키 필드·비교·조회·생성·삭제, 판, 원문 줄). 없는 것: 한도의 값과 설정 줄(이름만: `extent=OS_MAX_API_NAME`, `osal-f34759d.txt:59`), 부재 상수 항목(`chain-status` 목록뿐), 종류 macro 가 없는 종류와 TIMEBASE 의 조회·생성(`rules-03`), 층의 clear·rollback | 범위 규칙의 다음 개정(자료보다 먼저 커밋) |
| P0 `strcmp` 의 소스 대조 | (나) | 주지 않음(“source unverified”; 새 외부 자원이 필요) | PI 의 선택(새 외부 자원) |
| 종류별로 나뉜 키 필드 writer 와 완전성, 표 선택 상수만 받는 partition, 콜백으로 정하는 소속 | (다), (라) | **일부만.** partition 과 소속은 relations 개정 7 이 줌. 종류별 writer 나눔은 주지 않음: A21 의 `writers: OS_common_record_t.name_entry: 18 (assign 17, memset 1)` 한 줄(`relations.txt:650`). (다)는 `link_unlinked(name binding: a name mapping into the table is not excluded)` | OBJECT_ACCESS 를 읽는 relations 의 뒤 개정(8.12 (ㅈ)) |
| 생성자 callee 의 성공 상수를 행에 | 기초 4 | **서지 않음.** 개정 9 의 `returns not followed`·`set: not established` 줄이 A21 에 남음; result flow 의 줄(`:648`)은 규칙도 스크립트도 읽지 않음. 기초 4 는 `record_insufficient(no success constant of the creator: the callee's returns are not all followed)` | 판정 규칙의 덧붙임(완전성 조건)과 설명 스크립트 개정 4 |
| 삭제·회수 site 의 계획, 삭제 사건이 이름이나 out id 를 실음 | (마) | **일부만.** c1 의 관계 1·A20 과 r2 의 셋에 `object-delete` planned(§11.5). (라)의 실행 반쪽이 요구하는 생성 쪽 종류 값(`OS_ObjectIdAllocateNew`, `osapi-countsem.c:94`)의 site 나 not-planned need 는 계획에 없음: (라)는 `link_unlinked(name binding: the object kind of the lookup and the create is not shown the same)`. 조회 안의 키 글은 `string=validated bound=64`(런타임 상수; `plan.kzs:845`)이고 사용 호출의 bound=20 은 호출자 배열의 크기(`:932`): (나)는 `link_unlinked(name binding: key text not validated against the namespace bound)`. 판 4 를 읽는 런타임·플러그인이 없음 | 계획 생성기의 뒤 개정, 런타임·플러그인 단계 |
| 이름의 수명, 삭제 사건의 기록 분석 | (마) | 주지 않음 | 기록 분석 단계 |
| 이름 결합을 구현한 설명 스크립트 판 | 전부 | 주지 않음(개정 3 은 기초 4 의 생성자 쪽만) | 위 입력 뒤 |

  **사전 등록된 부팅은 위 셋((다), (라)의 실행 반쪽, (나))이 선다는 가정 위에 계획하지 않는다.** 그 셋이 없는 동안 연결 결론은 `unlinked` 를 넘지 않는다(8.7 의 “그때까지의 값”).
- `rules-01`: handling label 을 읽지 않는 선택 규칙(뒤 단계). `rules-02`: 판정 규칙의 덧붙임. `rules-03`: 범위 규칙의 개정과 자료의 다시 만들기, 다섯 계획. `semantics-10`: call-result 마디의 site.
- relations rules revision 12 의 한계 둘(값 없는 쓰기는 살아 있는 쓰기로 둠; 상수 열쇠의 생성)과 order rules revision 5 의 한계(같은 entry 의 스레드 인스턴스)는 `static_facts.md` §5w 에 있다 [미확인: 다섯 빌드에 그런 경우가 있는지].
- 앞 절에서 넘어온 열린 것은 그대로다.

### 12.10 lead 의 판단 (2026-11-18, 둘째 묶음의 검토와 §12 를 읽고)

모두 기술 판단이다. 목표·범위·공개·고유 증거의 삭제·새 외부 자원에 닿는 것은 5 항에 따로 적고 PI 결정을 기다린다. 아래에서 [예비] 라고 붙인 수는 사실 보기(`kz-static show`)와 남긴 relations·order·계획 출력을 스크립트로 읽어 얻은 것이고 기록 디렉터리에 남긴 출력이 아니다. 증거로 쓰지 않으며, 맡은 단계가 고친 도구의 사실에서 다시 재어 기록에 남긴다.

1. **이 묶음의 상태.** 문서 정정과 다시 돌린 비교(§12.4, §12.7)는 끝났다. 코드 고침은 소스와 fixture 까지만 커밋되어 있고 kz-static 의 선언 빌드는 아직이다(§12.8). MC 의 kz-static 은 `1a47abd0…` 그대로이므로 소스가 바이너리보다 앞서 있다. 다음 단계가 §12.9 의 차례대로 선언 12.0 (b)·사본·빌드·다섯 빌드·비교·처리 표시 다시 재기를 끝낸다. 그때까지 고친 규칙의 수(c5 대조 15 → 2, HB-await 120 → 284 등)는 [예비] 다.

2. **미뤄 두었던 판단.**
   - *층 차례의 라벨*(`rules-01`): `tier:D07` 은 글 라벨을 그대로 쓴다. 옛 예산을 다시 만들어 맞춰 보는 용도로 남긴다. sidecar 의 행 줄에 두 라벨을 함께 적는다. 처리 표시를 읽지 않는 선택 규칙은 C5 주기의 계획 전에 따로 글로 커밋하고, 그 순서 군은 같은 사실의 order 출력의 행 값에서 가져온다(세 경로에 한 어휘).
   - *묶음 루트 밖 정의의 키*(§7.8, A20 의 키가 c1_pre_tx 와 c1_post_tx 에서 다름): 형 이름에 이미 쓰는 소스 트리 토큰 이름을 함수·객체 키 글에도 쓴다. [예비] c1 두 빌드에서 키 셋씩이 움직이고 나머지 세 빌드는 0, 두 c1 빌드 사이에 같은 키 글이 364/367 에서 367/367 이 되며 충돌은 없다. 빌드 사이의 행 정체성은 옵션을 켠 사실에서만 쓴다.
   - *문서의 반환과 본문에서 얻은 반환*: 본문의 반환 집합이 완전할 때만 본문 값이 근거다. 문서의 값은 이름을 붙인 전제이고, 둘이 어긋나면 그 줄을 낸다.
   - *c5 의 것*(child without success 의 "possible" 증거, cfe 6.4.1·6.4.2 항목이 한 빌드에 함께 걸리는 것, 경로 B 의 대조 선택, 생성과 삭제를 가른 뒤의 대조 집합): C5 주기의 계획과 행별 기대를 커밋하기 전에 정한다.
   - *공간*: 다른 남긴 디렉터리와 바이트가 모두 같은 relations·order 점검 디렉터리 열둘([예비] 89,207,176 B)은 같음을 다시 확인하고 원장에 줄을 남긴 뒤 회수한다(고유 증거가 아니다). 옵션 없는 추출 회차 넷과 중간 옵션 회차 셋은 `facts.kz` 의 도구 정체성 바이트가 달라 바이트 동일이 아니다. 지우지 않는다. 공간이 모자라면 왕복을 확인한 압축 사본으로 바꾸고(바이트 보존) 원장에 사본의 sha256 과 구성 파일 목록을 적는다.

3. **말의 정정: "사전 시험"이 아니라 "대조 실행".** §6.10 2 항은 대조 행을 규칙으로 고르고 그 부팅을 이름 결합 규칙의 사전 시험으로 불렀다. 그러나 C1 의 관계 1, 고친 빌드의 관계 1, BP 의 탐침 행 A20 은 모두 규칙을 쓸 때 그 행의 글과 2026-11-16 의 기록을 읽은 행이다. 이 세 행의 실행은 **이미 읽은 행에 대한 대조 실행**이고 시험이 아니다. C1 안에는 이름 결합 규칙을 독립으로 시험하는 행이 없다(시간 기준 행은 생성자가 프로세스 행위자라 조회가 늘 찾고, 파일 이름 행은 행위자 뿌리와 부재 값이 없다 [예비]). 계획의 `--control-budget-reason` 글도 다음 계획부터 이 말로 쓴다. 옛 글은 남긴 계획에만 있다.

4. **C1 의 자동 주기 하나를 닫는 차례와, 닫았을 때의 기대.**
   - *차례*: (0) 위 2 항의 회수 → (1) 이 묶음의 선언 빌드 → (2) 규칙 글(판정 규칙의 덧붙임, 범위 규칙 개정 2, 계획 규칙의 다음 개정, 평가 절차, 설명 스크립트 개정 4 의 명세, 덤프의 build-id 레코드와 같은 빌드 줄의 글 형식)을 그것을 읽는 코드보다 먼저 커밋 → (3) 이름 공간 자료를 범위 규칙 개정 2 로 **새 파일 이름**에 만든다(지금 파일은 남긴 계획이 경로와 sha256 으로 붙잡고 있으므로 건드리지 않는다) → (4) kz-static 한 빌드(아래) → (5) 런타임·덤프·변환·플러그인 한 빌드(build-id 와 같은 빌드 판정, 플러그인의 `kzs-sites 4` 읽기; 덤프 형식의 부번호는 생성 토큰 레코드 종류까지 한 번에 정한다) → (6) 설명 스크립트 개정 4 를 (5)의 끝 점검 뒤에 동결(읽는 부분과 판정 부분의 sha256 을 따로 부팅 선언에) → (7) 두 C1 빌드의 계획·파생 빌드·결합, 미리 정한 사용 목록, 기대. 부팅은 선언을 커밋한 뒤 따로 한다.
   - *kz-static 한 빌드에 넣는 것*: 소스 트리 토큰 키; 생성자의 성공 경로 줄(그 값을 싣는 모든 반환 경로가 out id 저장과 이름 저장을 지나고, 저장되는 값의 정의가 그 경로에 묶임을 보일 때만 성립; 조건은 글 비교가 아니라 조건 나무의 함의로); 부재 상수의 기원 줄; 저장된 이름 버퍼의 writer 줄과 그 주소가 간 곳; id 를 받는 필드의 **모든** writer 와 값 종류, 뒤의 reader 가 지운 값을 가르는지; 호출 문맥별 대안 줄; order 의 완전한 쌍 파일(사실과 도구의 sha256 을 머리에, 줄 수 한도 없음; 영역은 관계 행이 이름 붙인 모든 생성자·writer 이며 같은 스레드의 것은 `same_thread`); relations 머리의 도구 sha256 줄; 계획의 이름 공간 항목 줄·생성 쪽 종류 값 site·문맥 틀 site·수명 끝의 site 형식. 선택 규칙은 넣지 않는다.
   - *실행 루프를 닫는 규칙*(필드에 저장된 값을, 그 필드의 주소를 받는 callee 의 반환 조건과 잇는 것): 형식은 함수·필드·역할 이름을 쓰지 않는다. 그러나 [예비] 다섯 빌드에서 이 규칙이 풀 열린 효과는 한 호출 자리(CF 의 실행 루프 호출)뿐이다. 그러므로 덧붙임과 결과에 "적용 자리 하나, 그 행을 읽고 쓴 규칙"이라고 적고, 선언에 빌드별 적용 자리 수를 넣으며, 설계 행 밖의 진술에는 쓰지 않는다. "사이에 writer 와 호출이 없다"는 줄 번호가 아니라 제어 흐름의 경로 조건으로 본다.
   - *기대: 지금 규칙으로는 어느 설계 행도 `use_cond = derived` 에 닿지 않는다.* 그러므로 이 주기의 어느 기록에도 `violation = observed` 를 기대하지 않는다. 결함 빌드의 관계 1 을 막는 것 [직접 확인한 줄과 예비 측정]:
     - (c2): 행의 `flip: static-undetermined`(조회의 키가 표 포인터로 와서 정적으로 풀리지 않는다, `ll_final_order/c1_pre_tx/relations.txt:661`).
     - (c3)(i): id 를 받는 필드 `CF_Channel_t.sem_id` 에 다른 정의가 있다. 엔진을 지우는 `memset`(`cf_cfdp.c:965`)이 그것이고, 뒤의 reader 는 그 지운 값을 가른 뒤에만 쓴다(`cf_cfdp_sbintf.c:91-93` 의 `OS_ObjectIdDefined`). writer 집합도 완전하지 않다(채널 주소가 형 없는 인자로 넘어가는 `cf_cmd.c:734`).
     - (c3)(iii): 초기화 문맥에서는 대안이 없음을 보일 수 있다 [예비]. 판정 규칙 6 절 1 항에 따라 (iii) 단독은 `partial` 이다.
     - (c4): 키가 풀리지 않아 등록 지점 집합이 전제에 머문다.
     - 연결: 이름 공간의 비교·복사·길이 호출이 원문 미대조라 (나)의 의미와 (다)가 빠진다.
   - *그래도 주기를 짓고 부팅한다.* 얻는 것: 쌍마다의 성립·누락 항목 목록(틀릴 수 있는 기대), 사용 사건의 호출 문맥 배정, 세 행을 요소 수준에서 가르는 줄(결함: 대안 없음 / 고친 빌드: 재시도 / 탐침: 같은 함수의 생성), `same_build` 의 판정, 개정 4 계획의 결합과 사건 양. 이 도구들은 C5 주기에도 그대로 든다.
   - *규칙을 그 행에 맞춰 풀지 않는다.* 덧붙임은 3.3 과 4.2 를 그대로 두고 입력만 댄다. 전제로 항목을 세우지 않는다(기초 4 는 성공 경로 줄이 설 때만, 아니면 누락으로 센다). 4.2 를 달리 읽는 것은 5 항의 PI 결정이다. 다만 답을 나중에 고를 수 있도록, **평가 갈래**를 부팅 전에 모두 적어 기대와 함께 커밋한다: 자료 상태 1(지금) / 자료 상태 2(규칙으로 만든 미대조 항목 목록 전부를 인정) / 그 부분 인정 / 4.2 의 읽기 갈래. 같은 동결 스크립트와 같은 기록으로 평가하며, PI 의 답은 어느 갈래를 보고할지를 고를 뿐 목록을 부팅 뒤에 고치지 않는다. 어느 갈래든 그 행을 본 뒤에 정한 것이므로 결과는 아는 행에 대한 확인이다.
   - *자료 상태의 규칙은 연결 항목만이 아니라 모든 성립 항목과 사용 조건 요소에 건다.* 자료 상태 1 에서는 행의 `P0-unverified(…)`·`sem-unverified(…)` 목록의 항목이나 적어 둔 전제에 기대는 것은 성립이 아니다(종료 호출 항목도 같다).
   - *보고의 틀*: 판정됨은 `observed`·`not_observed_in_runs`·`refuted_link` 이고 미확정은 같은 줄에 종류별로 둔다. 비율마다 단위 하나. 부팅별 분포로 내고 하나로 합치지 않는다. 행위자 쌍은 app-app, core-app, process-app(시작 때 프로세스 행위자가 만든 것), same-app-other, same_actor 로 가르고 앞의 것만 앱 사이로 센다. 정적 `unordered` 는 "이 개정이 간선을 찾지 못함"이라고 범례에 적는다. 도구 출력의 필드는 중립 이름(`pair_kind`, `order_ground`, `other_order_seen_in_boots`)만 쓴다. 틀릴 수 없는 기대(`same_build` 를 문에 두고 기초 1 을 기대하는 것, 판정 수준의 `expect_safe`)는 틀릴 수 있는 자리로 옮긴다: `same_build` 는 문이 아니라 기초 1 로 평가한다.
   - *부팅 수*: 부팅 전에 선언에 조건만의 함수로 적는다. 빌드의 첫 부팅이 문을 지나면 그 빌드를 정확히 네 번 더 부팅한다(첫 부팅이 무엇을 보였든). 문을 지나지 못하면 원인을 덧붙임에 적은 뒤 한 번만 다시 한다. 더 한 부팅에서 읽는 것은 부팅별 분포와 다른 순서가 보였는지뿐이다. 시작 스크립트가 소비자를 생성자보다 먼저 적으므로 모든 부팅에서 순서가 같으리라 본다 [I]. 그것은 다른 순서에 대해 아무것도 보이지 않는다.
   - *판독지*: 미리 정한 사용마다의 정적 부분(부팅 전)과 덤프 글에서 채우는 부팅별 부분으로 나누고, 읽는 사람에게 주는 파일 목록에서 기대와 사례 카드를 뺀다. 읽을 사람이 정해지지 않으면 "채우지 않음: 읽는 사람 없음"으로 적고 부팅은 진행하며, 정밀도와 계획 안의 재현율은 정의되지 않음으로 보고한다.

5. **PI 결정이 필요한 것**(결정 전의 값을 함께 적는다).
   - *C 라이브러리 항목의 근거*(비교·복사·길이·채움·종료 호출): 연결된 판의 원문은 이 환경에 없고 들여오는 것은 새 외부 자원이다. 결정 전: 미대조 그대로(자료 상태 1 을 보고).
   - *4.2 의 읽기*: (c3) 에서 (iii) 단독을 받을지; 지운 값의 정의를 "다른 정의"로 셀지; (c2) 를 flip 줄 대신 정적 부재 줄에서 읽을지; (c4) 를 키가 풀리지 않은 채 partition 의 생성자 집합과 전제로 받을지. 결정 전: 판정 규칙 그대로. 이 실제 행에서 드러난 것: 결함의 기전은 (iii)(조회 실패 → 앱 종료, 대안 없음)이고 (i) 은 글자대로는 서지 않는다. 6 절 1 항이 "실제 행에서 시험한 뒤 정한다"고 미뤄 둔 물음의 재료가 이것이다.
   - *생성된 프레임워크 층 자료의 지위*(§8.8), *독립 평가 자료*, *판독지를 읽을 사람*, *주장의 말과 단위*.

6. **실천의 좁힘.** 비교가 rc≠0 이면 다음 명령을 잇지 않는다(명령마다 rc 를 본 뒤 다음으로; `builds-01`). 시험 삼아 돌린 출력도 새 이름으로 남긴다. scratch 의 추출과 큰 보기도 감시 아래 둔다(`builds-04`). 선언 빌드 하나를 한 실행에 끝내지 못할 크기면 선언 전에 멈추고 소스만 커밋한다(이번 §12.8 과 같이; 반쯤 선언된 채 도구를 덮어쓰지 않는다). 두 단계가 같은 저장소에서 나란히 일할 때는 경로를 적은 커밋만 쓴다.

### 12.11 선언된 kz-static 빌드 (2026-11-19)

§12.9 의 남은 차례 (1)–(5)를 했다. 선언 §12.0 (나-1)(커밋 `44a45c7`)과 덧붙임 §12.0 (나-2)(커밋 `5096d4a`, 그것을 쓰는 비교보다 먼저)를 따랐다. 기록: `../records/review-fixes-2-mc-20261119/`(`README.md`). 출력: `/home/user/work/redesign/obs1/rf2_*`(새 디렉터리 11). cFS 를 부팅하지 않았고 push 하지 않았다.

#### 12.11 (가) 선언과 바뀐 것 [F]

- 앞 에이전트(2026-11-18, 보고 없이 멈춤)가 남긴 scratch(점검 빌드, 다섯 빌드의 scratch 출력, 감시 기록 16)를 근거가 아닌 [I] 의 출처로 썼다. 그 점검 빌드는 지금의 `kairos/` 코드와 같은 소스(HEAD `8249ba6`; 그 뒤 `kairos/` 에서 문서 넷과 새 자료 파일 다섯만 더해짐)에서 나왔다. 그 출력을 출력 종류마다 분류해 허용 차이(보고 275, `declared/`)와 쌍둥이 비교를 선언했다.
- 저장소에서 바뀐 것: 기록 디렉터리, golden 17(`kairos/tests/static/data/`), `records/TOOLS.tsv`(`1a47abd0` 행 copy, 새 행 `bf2ec2ee`), `records/LEDGER.tsv`(행 12: `TOOLS_20261119` 과 `OBS_rf2_*` 11), 이 문서(§12.0 (나-1)·(나-2), §12.2–12.7 의 날짜 붙은 줄, 이 절). `kairos/` 의 소스는 바꾸지 않았다.

#### 12.11 (나) 빌드와 시험 [M]

- 사본: `/home/user/work/preserve/tools-20261119/kz-static.1a47abd0.xz`(47,373,676 B, sha256 `b00b8545…`), `xz -t` OK, 푼 스트림의 sha256 과 `cmp` 가 제자리 파일과 같음(`tool_copy/ROUNDTRIP.txt`).
- `make kz-static`, `make all`(감시 아래, rc 0; `make_kz-static.log`, `make_all.log`). 새 kz-static `bf2ec2eedc756d72107a16532fa74f784cc7a1fd047d48edce62af20302fd460`, `--identity` 는 선언대로 `tool_sha256=73c3491c…`, `tool_source=2b8834da…`, `core_source`·스키마 그대로(`identity_kz-static.txt`). 다시 링크된 것 14 모두 바뀜, 다시 링크되지 않아야 할 110 모두 같음, `CMakeCache.txt` 같음, 다른 도구 여섯의 파일 sha256 같음(`relink_result.txt`).
- 첫 ctest(268, `-V`, `ctest_first.log`): 실패 16, 이름이 선언과 같음(`ctest_first_check.txt`). golden 갱신(`golden_update.log`, 16 시험 통과): 바뀐 golden 17 이 `../records/review-fixes-2-20261118/golden_scratch/` 와 모두 같은 바이트(`golden_cmp.txt`), `kairos/` 의 그 밖은 바뀌지 않음(`golden_git_status.txt`).
- 전체 ctest(`ctest_full.log`): 268 가운데 실패 0, 건너뜀 1(`test_probe_anchor_reached`). 확인 수(`ctest_counts.txt`): `test_static_cli` 1,184, `test_static_reasons` 763, `test_static_plan` 277, `test_core_schema` 34,017, `lint selftest` 114 경우 실패 0, lint 규칙 일곱 clean — 모두 선언의 [I] 와 같음. 단위 점검 17/17(`unit_tools.txt`), 7/7(`unit_step2a.txt`).
- 누적 점검: 빌드 뒤 `ledger_check` 136 행 실패 0(`ledger_check_after_build.txt`), `tools_check --deep` 50 행 실패 0(`tools_check_after_build.txt`); 끝 `ledger_check` 147 행 실패 0(`ledger_check_end.txt`), `tools_check --deep` 50 행 실패 0(`tools_check_end.txt`).

#### 12.11 (다) 바이트 동일성과 비교 [M]

| 비교 (`compare.sh`) | 결과 |
|---|---|
| 실행 단계 열하나 (`run_all.txt`) | 모두 rc 0 |
| relations, order, 다시 뽑기 둘, 그 order 둘, 계획 다섯 (`compare_all.txt`) | 15/25/50/50/40/50/30/30/35/35/35 OK, BAD 0. 모든 다른 줄이 선언된 class 에 듦(`other=0`) |
| 쌍둥이 (`compare/twin.txt`) | 350 파일 가운데 다름 47 — 선언 밖, 멈춤(§12.0 (나-2)) |
| 쌍둥이, 덧붙임 뒤 (`compare/twin_2.txt`, `compare_rest.txt`) | 350/350 같음(건너뜀 10 = `space_guard.log`); facts.kz·basis.kz 는 scratch 점검 빌드의 것과 같은 바이트 |
| 보고의 줄 (`compare_declared.txt`) | 275/275 가 `declared/` 와 같음 |

따라서 앞 도구 `1a47abd0` 의 출력과 다른 줄은 모두 선언의 class 와 수 그대로다(종류별 요약 `declared_summary.txt`). 옵션 없는 경로의 facts 는 c1·r2 에서 BASIS 1 레코드(도구 정체성에서 오는 basis 접두)만, c5 두 빌드에서 그것과 FACT_ATYPE 5 레코드(가변 인자 탈출의 사유 글)만 다르다.

#### 12.11 (라) 측정 [M]

수는 `measure/measure_<b>.txt`(`measure.py`, 남긴 출력에서만)와 `compare/` 의 보고에서 왔다. 차례 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642. 판정이 아니다.

- **handling label(하나의 outcome 규칙 아래)**: label 이 움직인 행 0/0/0/0/0(짝지은 행 233/233/180/133/131, 한 쪽에만 있는 행 0). 머리 줄이 앞 도구와 같다(c1_pre_tx `H 14 J 14 R 9 T 7 T? 47 none 142`).
- **exit-app·terminal·event**: RESULT_EFFECT 행 15930/15933/11198/6864/6867, 그 가운데 event send 1073/1067/307/589/589, application exit 305/305/86/0/0, terminal 159/159/158/169/165 — 앞 도구와 같다.
- **SEMANTICS**: 행 1773/1773/1476/795/786 → 1931/1931/1613/910/899. framework 의 collated/행 1578/1690·1578/1690·1303/1395·639/716·632/707 → 1736/1848·1736/1848·1440/1532·752/831·743/820. P0 과 status-space 는 그대로.
- **LOCK_HANDLES**: 행 1573/1573/967/0/0 그대로, 그 가운데 핸들을 찾지 못한 행 115/115/83 → 187/187/97(사슬의 호출이 모두 찾아질 때만 핸들, lock handle rules revision 2). 핸들 있는 행은 1458/1458/884 → 1386/1386/870.
- **decay**: FACT_ATYPE 레코드 수 그대로, 마지막 회차의 atype 줄 65/65/22/68/69 의 글이 바뀜(`compare/extract_final_view_<b>.txt`), extract.txt 의 location 상태 줄은 바뀐 것이 없다(상태 움직임 0).
- **HB-await 간선, writer 행위자마다**(마지막 회차): 120/120/64/0/0 → 284/284/116/0/0. c1: `main (process)` 120 → 60, `BP_AppMain`·`CFE_ES_BackgroundTask`·`CFE_ES_TaskMain`·`CFE_TIME_TaskMain` 0 → 56 씩. r2: `main (process)` 64 → 32, `CFE_ES_BackgroundTask`·`CFE_ES_TaskMain`·`CFE_TIME_TaskMain` 0 → 28 씩. T8CF facts 에서는 다섯 빌드 모두 0 → 0.
  - **(2026-11-19 덧붙임, `semantics-05`)** 이 수는 도구가 낸 간선의 수이고 소스가 허락하는 순서의 수가 아니다. §21 의 선언된 kz-static 이 낸 c1_pre_tx 마지막 회차(`obs1/cz_final_order/c1_pre_tx/edges.tsv`; 이 절의 284 와 같은 수)의 HB-await 284 간선은 모두 `cfe_es_apps.c:620:9` 의 `CFE_ES_TaskRecord_t.EntryFunc` 쓰기에서 나오고, writer × 대기 task 의 짝 71 에 사용 넷씩이다(writer 는 `main` 60, 나머지 넷 56 씩; `../records/closure-notes-20261119/check.txt` H). 소스에서 대기 루프(`cfe/modules/es/fsw/src/cfe_es_apps.c:508-529`)는 `CFE_ES_GetTaskRecordByContext()`(`:513`)로 자기 레코드를 읽고, 그 함수는 레코드를 `OS_TaskGetId()` 로 찾는다(`cfe_es_resource.c:285-286`): task 는 자기 레코드만 기다리고, 그것을 쓰는 것은 그 task 를 만든 쪽의 `CFE_ES_StartAppTask` 다. 그래서 성립할 수 있는 것은 생성자 → 만든 task 의 짝 15 의 60 간선이다(`main` → 코어 task 다섯과 시작 스크립트의 앱 다섯(`cfe_es_start.c:186`, `:199`), `CFE_ES_TaskMain` → `CFE_ES_BackgroundTask`, `CFE_TIME_TaskMain` → 1Hz task 둘, `BP_AppMain` → `BP_CLA` task 둘; 앱을 다시 시작하는 경로(`cfe_es_apps.c:1165`)의 생성자는 세지 않음 [I]). 나머지 224 는 소스가 부정하는 전제를 진다. 그 간선에 기대는 쌍은 없어(`obs1/cz_final_order/c1_pre_tx/pairs.tsv` 끝의 요약 줄 `conditional_before 0`) 어느 판정도 움직이지 않았다. r2 의 116 은 같은 방법으로 다시 세지 않았다 [미확인]. 도구의 한계이고 다음 kz-static 빌드가 다룬다(§26.0 (하)). [§26.1]
- **대조 집합(생성과 삭제를 가른 뒤)**: 5/5/4/15/15 → 5/5/4/2/2(모두 `complete=yes`).
- **key-clearing call 이 된 호출과 소스 대조**:
  - c5 두 빌드: 호출 하나, `cfe/fsw/cfe-core/src/sb/cfe_sb_priv.c:123:7` 의 `CFE_SB_CleanUpApp` → `CFE_SB_DeletePipeWithAppId`(행 줄 26). 소스에서 그 경로의 `CFE_SB_DeletePipeFull` 은 `CFE_SB.PipeTbl[PipeTblIdx].PipeId = CFE_SB_INVALID_PIPE`(`cfe_sb_api.c:431`)와 통계의 `PipeId = 0`(`:436`)을 쓴다: 삭제라는 읽기와 맞다. 같은 빌드의 이름 공간 둘에 key-clearing 함수 `CFE_ES_CleanUpApp#0`·`CFE_ES_ProcessControlRequest#0`·`CFE_TBL_CleanUpApp#0`·`CFE_TIME_CleanUpApp#0`, 호출 2 씩(행에 걸리지 않음; 소스 대조는 하지 않음 [미확인]).
  - c1 두 빌드와 r2: 이름 공간 `OS_common_record_t.name_entry` 에서 creators 53 → 47(r2 38 → 33), key-clearing calls 6(r2 5); 그 key-parameter 함수는 `OS_ObjectIdAllocateNew#1`(relations 의 `key-clearing (they reach constant stores of the key only)` 줄). 소스: 그 함수가 이르는 키의 쓰기는 `OS_ObjectIdFindNextFree` 의 `obj->name_entry = NULL`(`osal/src/os/shared/src/osapi-idmap.c:640`, 주석 “Ensure any data in the record has been cleared”) 하나이고, 이름은 호출자가 그 뒤에 넣는다. 즉 이 호출들은 **삭제가 아니라 자리를 비우며 하는 할당**이다. relations rules revision 12 의 이름(key-clearing)은 이 경우 맞지 않는다(§12.11 (사)). c1_pre_tx facts 의 `OS_ObjectIdAllocateNew` 호출 16 가운데 key-parameter 함수 밖의 것 5: `osapi-filesys.c:274:19` `OS_FileSysAddFixedMap`, `osapi-printf.c:95:19` `OS_ConsoleAPI_Init`, `osapi-sockets.c:129:19` `OS_SocketOpen`, `osapi-sockets.c:252:27` `OS_SocketAccept`, `osapi-time.c:134:19` `OS_DoTimerAdd`(`measure/c1_pre_tx_allocatenew_uses.txt`). 도구가 센 여섯째는 찾지 못했다 [미확인: 직접 호출이 아닌 사용일 수 있음].
  - 같은 c1 의 `OS_filesys_internal_record_t.virtual_mountpt` 이름 공간: key-clearing 함수 `OS_FileSys_Initialize#1`·`OS_initfs#1`·`OS_mkfs#1`(이르는 쓰기는 `osapi-filesys.c:159:9` 의 `memset … := 0`), 그 사용은 모두 다른 callback 으로 빠지는 쪽(left out creators 6 → 2, lookups 55 → 59)이다.
- **개정 4 계획의 site·need**: site 291/291/65/201/193 → 291/291/65/165/157(c5: creator-call 6 → 4, effect-call 18 → 10, use-call 140/134 → 114/108), gap 1/2/0/5/5 그대로. need 49/49/29/99/96 → 52/52/29/36/33. c1: `event-send` planned 3 → 2/1, not-planned 2 → 3/4, `event-send-other-outcome` planned 0 → 3. c5: `object-delete` not-planned 15 → 2, `use-result` planned 23/22 → 10/9, `lock-result`·`wait-result` not-planned 23/22·22/21 → 10/9·9/8, `event-send` planned 8 → 2, not-planned 7 → 0, `event-send-other-outcome` planned 0 → 2.

#### 12.11 (마) 공간 [M]

| 시점 (UTC, 2026-11-19) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 시작(01:15) | 950,894,592 | 앞 에이전트의 scratch 566 MB 가 남아 있음 |
| 선언 직전 | 1,050,628,096 | scratch 보기 88 MB 를 지운 뒤 |
| 빌드 전 | 1,349,107,712 | scratch 빌드 트리(약 296 MB)를 지운 뒤 |
| 빌드 중 가장 낮은 값 | 1,131,233,280 | `make kz-static` dip 170,074,112; `make all` dip 40,079,360 |
| 실행·비교 중 가장 낮은 값 | 974,893,056 | 마지막 회차 비교의 facts 보기(dip 88,408,064) |
| scratch 를 모두 지운 뒤 | 1,253,584,896 | 남긴 새 출력 176,307,815 B, 사본 47,373,676 B |

- 하한(314,572,800)을 넘지 않았다. 이 실행의 감시 기록 38(`df_*.log`)과 scratch 의 것 13(`scratch_guard/017-029`); 앞 에이전트의 감시 기록 16 은 `scratch_guard/001-016` 으로 옮겼다(그 scratch make 는 하한 아래 305,602,560 B 를 적었다, §12.0 (나-1)). 압축 대체는 쓰지 않았다.
- **(2026-11-19 덧붙임, `builds-06`)** 선언 §12.0 (나-1)의 근거가 된 앞 에이전트의 scratch 점검 빌드는 선언한 필요량을 넘었다: `scratch_guard/002_scratch-make.log` 는 필요량 500,000,000 B 에 dip 1,235,963,904 B(약 2.5 배)이고 끝 여유 305,602,560 B 로 하한 아래인데 판정은 `verdict=completed` 다. 감시(`env/tools/space_guard.sh:51-77`)는 명령이 도는 동안의 표본에서만 멈추고, 끝난 뒤 잰 값이 하한 아래여도 판정을 바꾸지 않는다(고침은 §26.0 (차)). 그래서 '모두 `completed`' 는 하한을 지켰다는 근거가 아니고, 근거는 가장 낮은 값이다: 이 실행의 감시 기록 38 은 974,893,056 B, scratch 의 017–029 열셋은 948,453,376 B, 앞 에이전트의 001–016 열여섯은 305,602,560 B(위의 한 건, 끝 값이 하한 아래인 유일한 기록)(`../records/closure-notes-20261119/check.txt` A). [§26.1]

#### 12.11 (바) 계획과 다르게 한 것

- 쌍둥이 비교가 선언 밖의 두 종류(47 파일)에서 멈췄고, 까닭을 밝힌 덧붙임 §12.0 (나-2)을 커밋한 뒤 `twin_2.py` 로 다시 돌렸다. 첫 보고(`compare/twin.txt`)는 남겼다.
- 앞 에이전트가 커밋하지 않고 남긴 초안 스크립트(무시되는 `build/` 디렉터리)를 옮겨 고쳐 썼다. 초안의 `status.txt` 점검 무늬(`^OK`)는 실행기가 쓰지 않는 것이라 scratch 비교에서 10 줄이 BAD 였다; 선언 전에 `result PASS` 로 고치고 그 열 줄을 따로 다시 점검했다(`scratch/decl_status_recheck.txt`). `declared/` 의 보고는 이 점검에 기대지 않는다.
- 측정 계획(`plans_v4_all`)의 예산 이유 글을 앞 계획의 글(개정 번호가 든 글)과 다르게 썼다(선언에 적음). 대조 예산의 이유 글은 §12.10 3 항의 말로 썼다.
- facts 보기의 허용 차이는 칸이 아니라 절 단위(네 칸의 가림 + 절 일곱)로 선언했다; 칸 단위 내용은 쌍둥이 비교(바이트)가 묶는다. 보기의 semantics 줄 차이(c1 32/190)는 SEMANTICS 레코드 차이(1696/1854)보다 훨씬 작다: 보기가 레코드의 모든 칸을 보이지 않기 때문으로 보인다 [미확인].
- c1 의 key-clearing 호출 여섯 가운데 하나는 위치를 찾지 못했다.
- **(2026-11-19 덧붙임, `builds-02`)** 측정은 선언 §12.0 (나-1)('비교가 모두 OK 인 뒤')과 달리 비교 도중과 쌍둥이 비교가 멈춘 뒤에 돌았다. `measure/measure_<b>.txt` 다섯은 02:03:56Z 에 쓰였고, 그때 끝난 비교는 relations·order·extract_off(02:03:26–02:03:40Z) 셋뿐이었다: extract_final 은 02:04:00Z, 나머지 일곱은 02:04:14Z 까지 끝났고 쌍둥이는 02:04:16Z 에 rc 1 로 멈췄다(`compare_all.txt`). `measure/c1_pre_tx_calls_of_allocatenew.txt` 는 02:04:10Z(그 facts 보기 `df_view_allocatenew.log` 02:04:09–02:04:11Z 는 `plans_v4`·`plans_v4_all` 의 비교와 겹침), `measure/c1_pre_tx_allocatenew_uses.txt` 는 02:04:28Z 로 쌍둥이 멈춤 뒤, 덧붙임 `5096d4a`(02:06:30Z) 전이다. `measure.py` 는 그 뒤 다시 돌리지 않았다. 측정이 읽은 `rf2_*` 는 실행 단계가 02:03:14Z 에 모두 끝낸 출력이고(`run_all.txt`), 뒤의 `twin_2`(350/350, 02:06:34Z)와 `declared`(275/275, 02:06:38Z)가 그 출력을 확인했다(`compare_rest.txt`). 시각은 `../records/closure-notes-20261119/check.txt` C·B. [§26.1]
- **(2026-11-19 덧붙임, `builds-03`)** 한 단계가 앞 단계의 rc 를 보기 전에 시작한 곳이 있다(§12.10 6 항 '명령마다 rc 를 본 뒤 다음으로' 와 어긋남): 빌드 뒤 `tools_check --deep`(`df_tools_check_after_build.log` 01:56:25.023–01:58:21.400Z)이 도는 동안 `run.sh` 의 relcheck(01:56:35Z)·order(01:56:37Z)·extract_off(01:57:21Z)가 시작했다. 측정의 facts 보기도 비교와 겹쳤다(위 `builds-02`). 모든 rc 는 뒤에 0 이었다. 겹침의 목록은 `../records/closure-notes-20261119/check.txt` B. 뒤 빌드는 감시 아래의 단계를 첫 rc≠0 에서 멈추는 차례 실행기 하나로 돌린다. [§26.1]
- **(2026-11-19 덧붙임, `builds-05`)** 첫 ctest 의 기대가 빌드 전 옛 바이너리의 실행에만 기댄 시험이 하나 있다: 다시 링크된 kz-static 을 부르는 `tool_identity_schema_revision`(§12.0 (나-1)의 덧붙임). 통과했다. 뒤 선언은 다시 링크되는 대상의 `TARGET_FILE` 인자에서 시험 집합을 낸다. [§26.1]
- **(2026-11-19 덧붙임, `builds-07`)** 선언된 소스는 선언 커밋 `44a45c7`(01:39:29Z)의 `kairos/` 다. 빌드는 01:42:35Z 에 시작했고(`build_start.txt`), 그 사이 01:42:28Z 에 다른 단계의 `090e280` 이 `kairos/semantics/framework_namespaces2_*.txt` 다섯을 더했다. ctest 의 lint 시험 `lint_case-ids-data`·`lint_semantics-names` 는 `kairos/semantics/` 를 읽으므로(`kairos/tests/lint/CMakeLists.txt:8-9`, `kairos/tests/lint/kz_lint.cpp:403`, `:415`) 첫 ctest 와 전체 ctest 의 lint 는 선언보다 자료 파일이 다섯 많은 트리에서 돌았고 깨끗했다. 도구 정체성(`tool_source=2b8834da…`, 선언의 값 그대로; §12.11 (나))은 그 파일들을 덮지 않는다. 덧붙임 §12.0 (나-2) 1 은 실행 단계가 그 파일을 읽지 않는다는 것만 말했다. [§26.1]

#### 12.11 (사) 열린 것

- **relations rules revision 12 의 이름**: c1·r2 의 `OS_ObjectIdAllocateNew` 처럼 자리를 비우고 할당하는 함수의 사용도 key-clearing call 로 센다(삭제와 할당 초기화를 가르지 못함). 그 때문에 그 이름 공간의 creators 가 53 → 47 로 줄었다. 행에 걸리는 것은 없지만(c1·r2 의 대조 집합과 order 는 그대로) 이름 공간 자료나 생성 쪽 site 를 쓰는 뒤 단계(§12.10 4 항의 kz-static 한 빌드)가 이를 다뤄야 한다. 다루는 방법은 lead 의 판단을 기다린다.
- c5 의 이름 공간 둘의 key-clearing 함수(`CFE_ES_CleanUpApp` 등)는 소스와 대조하지 않았다.
- handling label 은 하나의 outcome 규칙 아래에서도 다섯 빌드 모두 움직이지 않았다: `rules-05` 의 고침은 이 빌드들의 label 을 바꾸지 않는다 [M]. label 을 읽지 않는 선택 규칙(§12.10 2 항)은 그대로 뒤 단계다.
- §12.9 의 판정 규칙 8.7 입력 표와 그 밖의 열린 것은 그대로다. 이 빌드는 그 입력을 하나도 주지 않는다.

## 13. 규칙 글과 형식: 그것을 읽는 코드보다 먼저 커밋함 (2026-11-18–19)

이 절은 글 단계가 돌려준 결과 글을 kz-static 단계(§15)가 놓은 것이다. 받은 글이 (6) 항의 중간에서 끊겨 있었으므로 (6) 항과 그 뒤의 짧은 항목은 커밋 `8605a04`·`4e29d50`·`95ce80e` 의 메시지와 `../records/closure-texts-20261118/README.md` 에서 적었다(§15 (바)).

### 13.1 선언 [D]
글만 고쳤다. 도구 빌드는 없고 MC 바이너리도 돌리지 않았다(나란히 가는 kz-static 단계가 제자리에서 다시 빌드했다, §12.11). 그래서 정체성이 바뀐 도구가 없고, 빌드 선언도 바이트 동일성 목록도 없다.

### 13.2 바뀐 것 [F] (§12.10 4 항의 차례대로, 하나에 커밋 하나)
(1) 판정 규칙 8.13 (`e809ba1`): 3.3 과 4.2 는 그대로 두고 입력만 댄다. 전제로 항목을 세우지 않는다. 자료 상태의 규칙은 모든 성립 항목과 사용 조건 요소에 걸고, 미대조 항목의 후보 목록은 규칙으로 만든다. 평가 갈래는 `D1`/`D2`/`D2[종류]` × `R0`/`Ra`–`Rf` 이고, PI 의 답은 보고할 갈래를 고를 뿐이다. 기초 1 은 문이 아니라 항목이다. 기초 4 는 성공 경로 줄이 저장 값의 묶임까지 보일 때만 선다. (다)는 저장된 이름 버퍼의 writer 와 그 주소가 간 곳을 요구한다. (c3)(i)은 지우는 것을 포함한 모든 writer 를 값 종류와 함께, 그리고 reader 의 가름을 읽는다. 문맥별 대안 줄을 둔다. 필드 저장값 규칙은 다섯 빌드에서 적용 자리 하나이고, 그 행을 읽은 뒤 썼으며, 경로 조건으로 보고, 설계 행 밖에는 쓰지 않는다. 수명 끝의 꼴은 셋, 인스턴스 연결은 열린 세대에만 한다. 쌍 종류 다섯, 쌍 파일의 영역, 서식 검사 13–17 을 둔다.
(2) 범위 규칙 개정 2 (`5c460eb`, `kairos/docs/semantics_coverage.md` 8절): 종류는 표를 고르는 매개변수에 묶인 인자에서 오고, 구성 요소마다 `layer-fn` 의 정의 줄과 위치를 적는다. 실패한 생성의 되돌림을 적는다. 층의 지우기와 마무리는 최상위 `layer-op` 문으로 둔다(지금 읽는 쪽은 그 파일을 통째로 물리고 연산으로 잘못 읽지 않는다; 문장 점검 19 줄 가운데 9 줄을 물림, `records/closure-texts-20261118/out/ns_statement_check_2.txt`). 자료는 새 파일 이름 `framework_namespaces2_*` 에 쓴다.
(3) 계획 규칙 개정 5 글 (`0eaf478`, `kairos/docs/plan_manifest.md` 10절): `namespace-entry` 줄과 `namespace-layer` 줄(extent 값과 상수), 생성 쪽 종류 값 site, 문맥 틀 site, 수명 끝 넷의 site 꼴과 사건이 id 를 이름 대는 방식(되돌림의 `reentry` 조건 포함)을 둔다. 행 줄에는 라벨 둘을 적고, 층 라벨과 선택은 그대로다. 대조 예산의 사유는 "대조 실행"이라는 말로 쓴다.
(4) 형식 (`2e4827a`): `kairos/docs/static_facts.md` 5x 에 relations rules revision 14 의 줄 여섯, relations·order 머리의 도구 줄, 소스 트리 토큰 키(옵션을 켠 사실에서만 빌드 사이 행 정체성), order rules revision 8 의 완전한 쌍 파일(facts·tool sha256, domain, `same_thread`)을 둔다. `kairos/docs/event_semantics.md` 13절에 dump 1.5 의 `buildid` 줄, 예약한 `tok-*` 줄 셋, 연결 점검의 `same_build_object`/`same_build` 줄을 둔다. `include/`·`cmake/` 는 바꾸지 않는다.
(5) 평가 절차 (`269bcbb`, 2026-11-19 고침 `eeeb77f`, `plan/c1_closure_evaluation_protocol_20261118.md`): 부팅 수는 조건만의 함수로 정한다. 문에는 `same_build` 를 넣지 않는다. 부팅 전에 써 두는 평가 갈래는 16 이다(처음 판의 "14"는 잘못 센 것; 2 × (1+6+1)). 쌍·사용·행마다의 기대는 `[채움]` 자리로 둔다. 틀릴 수 있는 점검은 F1–F10 이고, `expect_safe`(s1–s4)와 `expect_discriminate` 는 틀릴 수 있는 자리에 둔다. 분모를 정하고, 판독지는 정적 부분과 부팅별 부분으로 나눈다. 사례 카드는 읽는 사람의 파일 목록에서 빼고, 읽는 사람이 없을 때의 대체를 적는다. 요약과 기대 대조 스크립트도 동결한다. PI 에게 묻는 라이브러리 근거에는 종료 호출 항목과 프레임워크 `agreement` 항목도 든다.
(6) 설명 스크립트 개정 4 명세 (`plan/c1_closure_explain_rev4_spec_20261118.md`): `8605a04` 는 사용 한도로 멈춘 때의 판(점검 전)이다. `4e29d50` 이 판정 규칙 8.13, 형식, 평가 절차와 대조해 마쳤다: 후보 목록과 인정 파일의 형식(여덟 칸, 머리, 꼬리, 부분 집합 점검, 어긋나면 rc 2), 되돌림을 이름에 잇는 재진입 조건, 필드 저장값 규칙에 기대는 갈래의 표시와 그 따로 센 수, 요약과 기대 대조 스크립트의 동결, 각각의 fixture [F: 커밋 메시지].
(7) 탐침 기록 (`a414a58`, 읽기 주석 `95ce80e`): `../records/closure-texts-20261118/` 의 탐침 스크립트와 앞 도구 `1a47abd0…` 의 마지막 옵션 회차 사실에서 다시 돌린 출력(29 명령 모두 rc 0). 수는 [예비] 이고, 고친 도구의 사실에서 다시 돌린 것은 §15 (나).

### 13.3 시험·바이트 동일성·공간
빌드가 없어 ctest 와 바이트 동일성 목록이 없다. 이름 점검은 `a414a58` 부터 그 단계의 마지막 커밋까지 깨끗했다고 그 단계가 보고했다 [그 단계의 보고; 이 단계가 다시 돌린 점검은 §15 (마)]. 공간은 글 몇 KB 와 탐침 기록이다.

### 13.4 열린 것
판정 규칙 8.13 의 입력(성공 경로 줄, 쌍 파일 등)은 이 글을 구현하는 kz-static 빌드(§15)와 런타임 빌드가 낸다. §12.9·§12.10 5 항의 PI 결정은 그대로 기다린다.

## 14. 이름 공간 자료: 범위 규칙 개정 2 의 새 파일 다섯 (2026-11-18–19)

이 절도 kz-static 단계(§15)가 놓았다. 받은 결과 글에 이 단계의 글이 들어 있지 않아(§15 (바)) 커밋 `5d74967`·`090e280` 과 `../records/nsdata-r2-20261118/`(`README.md`, `declaration.md`, `commands.txt`, `out/`)에서 적었다.

### 14.1 선언 [D]
`records/nsdata-r2-20261118/declaration.md`(`5d74967`, 훑기 고침과 함께, 어느 개정 2 자료 파일보다 먼저 커밋함): 범위 규칙 개정 2(§13.2 (2))에 따라 `env/tools/semantics_scan.py` 를 고치고, 새 파일 이름 `kairos/semantics/framework_namespaces2_<component>.txt` 다섯을 만든다. 개정 1 의 다섯과 나머지 자료 여덟은 건드리지 않는다(남긴 개정 4 계획이 경로와 sha256 으로 붙잡고 있음). 도구 빌드도 MC 바이너리 실행도 없다.

### 14.2 바뀐 것 [F]
- 훑기: 종류는 층 함수가 표를 고르는 매개변수에 묶인 인자에서 온다(그 매개변수는 선언된 형으로 찾음). 실패한 생성의 되돌림(`rollback`), 층 함수(`layer-fn`), 층 연산(`layer-op`), `not-covered`·`kind-unresolved` 줄.
- 자료(`090e280`): 새 파일 다섯, sha256 `../records/nsdata-r2-20261118/out/data2_sha256.txt`. 세대 N 의 두 구성 요소(osal-f34759d, osal-7.0.1): 종류 15/16 가운데 연산이 있는 것 13/14, 개정 1 에 없던 종류 셋(`CONDVAR`, `CONSOLE`, `TIMECB`), `layer-fn` 10, `layer-op` 4(`clear-table`, `clear-slot`, `rollback`, `delete-finalize`), `rollback` 16/17, `kind-unresolved` 7, `not-covered` 7(`out/report/kinds_per_component.txt`). 세대 T(osal-4.1.1, cfe-6.4.1, cfe-6.4.2)는 개정 1 의 항목 줄 그대로다.

### 14.3 점검 [M]
- `semantics_scan.py check` rc 0: 자료 18 파일(개정 1 열셋, 개정 2 다섯), 실패 0(`out/scan_check.txt`); fixture 점검 rc 0(`out/fixture_check.txt`); 옛 구성 요소 목록으로는 개정 2 의 세대 N 두 파일에서 rc 1(선언의 점검 6 대로, `out/scan_check_old_list.txt`).
- 개정 1 의 열세 파일을 다시 만든 것이 저장소와 같은 바이트이고, `kairos/semantics` 의 git diff 가 비어 있음(`out/regenerated_rev1_vs_repo.txt`, `out/data_sha256_compare.txt`).
- 남긴 개정 4 계획 다섯의 `input namespace-data` 붙잡기 25/25 같음(`out/plan_pins_check.txt`).
- lint `case-ids-data`·`semantics-names` 가 새 파일이 있을 때와 없을 때 모두 rc 0, selftest 114 경우 실패 0, 심어 둔 이름은 찾음(rc 1)(`out/lint_*`).

### 14.4 읽는 데 주의할 것과 열린 것
- **지금의 계획 생성기는 새 파일을 읽지 못한다**(`lib/llvm/plan_gen.cpp` `read_namespaces` 가 `layer-fn`·`layer-op`·`kind-unresolved` 와 들여 쓴 `rollback`·`not-covered` 를 물림). 읽는 쪽은 계획 규칙 개정 5 의 kz-static 빌드(§15)가 들인다.
- 세대 T 의 표에서 어느 항목 줄도 이름 대지 않는 이름 경로 쓰기(osal-4.1.1 의 `"\0"` 복사로 쓴 지우기 다섯, cfe 의 시작 객체 이름 복사와 끝 표지 저장 둘)는 기록만 했다(`out/report/writers.txt`). 개정 2 는 세대 T 를 바꾸지 않는다.
- 층의 get-by-name 함수를 거치는 이름 조회(파일 시스템 종류, 구성 요소마다 2 호출)는 `lookup` 줄이 아니고 따로 보고했다(`out/report/get_by_name.txt`).

## 15. C1 닫기의 kz-static 한 빌드: 소스 진행분과 scratch 점검 (2026-11-19, 선언 전에 멈춤)

§12.10 4 항의 "kz-static 한 빌드에 넣는 것" 가운데 넷을 소스로 커밋하고 scratch 빌드에서 점검했다. **선언, 도구 사본, MC 빌드, 다섯 빌드의 선언된 다시 뽑기는 하지 않았다**: 넣을 것이 다 들어가지 않았으므로 §12.10 6 항대로 선언 전에 멈췄다. MC 의 kz-static 은 `bf2ec2ee…`(§12.11) 그대로이고, 소스가 그 바이너리보다 앞서 있다. 기록: `../records/closure-kzstatic-20261119/`(`README.md`). cFS 를 부팅하지 않았고 push 하지 않았다. 아래의 수는 모두 기록 디렉터리의 출력 파일에서 왔다. scratch 도구의 수는 선언된 도구의 측정이 아니므로 [I] 다.

### 15.1 고친 도구의 사실에서 다시 돌린 탐침 [M]
- `../records/closure-texts-20261118/scripts` 의 스크립트를 입력 디렉터리 이름만 바꿔(`rf2_final_order`, `rf2_plans_v4`, `rf2_final_extract`) `probes/scripts/` 에 복사했다. 고친 도구 `bf2ec2ee…` 로 마지막 옵션 회차 사실의 보기 둘을 감시 아래에서 만들었고(`probes/views.sha256`, 도구 sha256 앞뒤 같음), 29 명령이 모두 rc 0 이다(`probes/out/commands_rc.txt`). 첫 시도는 저장소 뿌리를 한 단계 잘못 잡아 15 번째 명령에서 멈췄다. 그 출력은 `probes/out_trial1_wrong_root/` 에 남겼다.
- 앞 도구의 출력과 비교했다(`probes/normcmp_3.txt`). 줄 번호, 행 번호, 디렉터리 이름을 가린 뒤 C1 의 출력 26 이 같다: P1 대안, P3 버퍼 writer, P4 holder, P6 키, P7 항목, 열린 사유 모집단(1/1/0/0/0 호출 자리). P2 의 한 줄은 `view line(s)` 꼴의 번호만 다르다.
- 바뀐 것은 P5(`probes/out/p5_pairs.txt`)의 c5 두 빌드뿐이다. c5_641 의 쌍은 4,406 → 4,393, 적힌 쌍은 631 → 618, 계획된 행은 23 → 10 이다. c5_642 는 4,395 → 4,382, 620 → 607, 22 → 9 다. §12.11 (라)의 대조 집합 15 → 2 를 따르는 움직임이다. C1 의 쌍 24,796/1,988 과 계획된 행 13 은 그대로다.
- 그러므로 C1 설계 행에 대한 §12.10 4 항의 [예비] 읽기(필드 저장값 규칙의 적용 자리가 하나라는 것, (c3)(i)의 writer 둘과 reader 의 가름, 성공 경로 줄을 원형이 다섯 callee 가운데 하나에서만 세운다는 것)는 고친 도구에서도 같다.

### 15.2 바뀐 것 [F]
글을 먼저, 그것을 쓰는 코드를 뒤에 커밋했다.
- 형식 덧붙임 `static_facts.md` 5x (`b4cfda0`, `2f51dbe`, `49d8467`, `f381cd9`):
  - order 글의 도구 줄 자리, 쌍 파일의 `creator-not-a-pair(same actor only)`, 한 스레드의 쌍은 늘 `same_thread`, 꼬리 수.
  - 쌍 파일은 글 뒤에 계산한다. `edges.tsv` 에 없는 간선은 `edge` 줄로 적는다.
  - `source_tree[i]` 항목을 찾는 규칙과 키 글에 쓰는 법.
  - `absence origins:` 줄의 자세한 꼴.
- 소스 (`c0e17ae`, `5024826`, `4ce894a`; `lib/llvm/order.cpp`·`order.hpp`·`relations.cpp`·`relations.hpp`·`rel_facts.*`·`serialize.cpp`, `tools/kz-static/cmd_show.cpp`·`main.cpp`, `tests/static/static_golden.cpp`):
  1. relations·order 머리의 도구 줄(`tool kz-static tool_sha256=… tool_source=…`). 명령이 쓰고, 안에서 읽는 쪽은 쓰지 않는다. golden 시험은 이 줄을 뺀다.
  2. order rules revision 8 의 완전한 쌍 파일(`kz-static order … --pairs FILE`). 영역은 relations 행이 이름 대는 모든 생성자·writer 다(not-a-pair 셋 포함, 같은 스레드는 `same_thread`). 행마다 짝이 없으면 `no_pair`, `edges.tsv` 에 없는 간선은 `edge` 줄이고, 줄 수 한도가 없다. golden 시험이 쌍 파일을 읽고, `--pairs` 가 글을 바꾸지 않는지 본다.
  3. `--object-access` 아래의 BASIS `source_tree[i]` 와 relations 키 글의 소스 트리 토큰.
  4. relations rules revision 14 의 `absence origins:` 줄. RESULT_FLOW·RESULT_EFFECT·OBJECT_ACCESS 가 함께 있는 사실에서만 쓴다.
- 바꾸지 않은 것:
  - 규칙 개정 이름(`relations rules revision 12`, `order rules revision 5`)은 개정 14·8 이 다 들어간 뒤에 바꾼다.
  - 저장소의 golden 은 갱신하지 않았다. 선언된 빌드에서 갱신한다(§12.8 의 앞례). 그래서 지금 HEAD 를 빌드하면 order golden 셋이 실패한다 [I: scratch ctest].

### 15.3 scratch 빌드와 점검 [I]
scratch Release 빌드는 kz-static, static_golden, kz_lint 이고 모두 감시 아래에서 돌렸다(`scratch_guard/`, `scratch/logs/`). 같은 tree 에서 고친 뒤 다시 빌드하기를 다섯 번 했다(make1–5).

| 점검 | 결과 | 출력 |
|---|---|---|
| golden 32(core 제외) | 실패 3 = 쌍 절이 더해진 order golden 셋(`order_np`, `resultflow_order_np`, `looplock_order_np`), 나머지 29 같음; scratch golden 셋은 다시 돌려 `matches` | `scratch/logs/ctest_golden4.log`, `scratch/golden_scratch/` |
| lint 시험 8 | 모두 통과 | `scratch/logs/ctest_lint.log` |
| 쌍 점검(`scripts/pair_check.py`) fixture 셋 | 모두 PASS | `scratch/fixture_pairs/` |
| 쌍 점검 다섯 빌드(scratch 도구, rf2 마지막 회차 사실) | 모두 PASS: 글에 적힌 쌍 1,988/1,988/1,607/618/607 이 모두 같은 값의 줄을 가짐; 행마다 글의 `pairs:` 수 = 파일의 생성자·writer 줄; 간선 id 가 `edges.tsv` 나 `edge` 줄에서 모두 풀림; 계획된 행 13/13/7/10/9 가 모두 줄을 가짐(그 줄 1,125/1,125/47/1,841/1,835) | `scratch/pairs_check4/check_<b>.txt` |
| 같은 실행의 order 글과 `edges.tsv` | 도구 줄 말고 `rf2_final_order` 와 같은 바이트(다섯 빌드) | 같은 실행, 위 표의 줄 |
| 다섯 빌드 다시 뽑기(scratch 도구, `run.sh extract_final`) | rc 0; facts 차이는 머리의 정체성 칸, BASIS(c1 두 빌드: 항목 셋 더함, 그 뒤 항목의 번호가 밀림), STRINGS 크기뿐 | `scratch/facts_cmp/facts_<b>.txt` |
| `source_tree` 항목 | 3/3/0/0/0 (`<bp>`, `<bplib>`, `<cf>`) | `scratch/keys/basis_source_tree_<b>.txt` |
| 키가 움직인 행 | 3/3/0/0/0(A20, A27, A30); 공유 키 0; **두 C1 빌드의 키 글이 367 개 가운데 367 개 같음**(앞 도구는 364) | `scratch/keys/keys_compare.txt` |
| relations 글 | facts 줄과 도구 줄 말고 같음(다섯 빌드) | `scratch/keys/relations_text_diff.txt` |
| 개정 4 계획 | sidecar 차이는 정체성·입력 줄과, c1 에서 움직인 키 셋의 `row`·`map`·`site refs`·`need` 줄뿐(키를 바꿔 놓으면 `map` 줄 하나의 차례만 남음); r2·c5 는 정체성 줄만 | `scratch/plans_v4/sidecar_diff_kinds.txt` |
| `absence origins:` 줄 | 값이 정해진 `absence:` 줄 39/39/17/23/23 마다 하나, 모두 `[search no-match]` 기원을 가짐; 정해지지 않은 24/24/8/43/43 은 줄 없음; 기원 둘인 줄 7/7/4/0/0 | `scratch/absence/absence_counts.txt` |

### 15.4 쌍 파일이 보인 것 [I]
- 쌍 파일의 줄 수는 26,634/26,634/9,643/5,218/5,207 이다. 값 분포(c1_pre_tx)는 unordered 21,664, hard_before 1,525, fixed_early_proved 1,276, same_thread 1,920, unknown 187, no_pair 62 이다(`scratch/pairs_check4/check_c1_pre_tx.txt`).
- **남긴 쌍 수 24,609 와 `pairs:` 줄의 합 24,796 의 차이 187 은 이렇게 설명된다.**
  - 그 187 은 한쪽이 어느 뿌리에도 닿지 않는 쌍(`unknown(no root reaches …)`)이고, 쌍 파일의 unknown 187 과 같은 수다.
  - 글의 `pairs:` 줄은 이 쌍을 `unknown` 으로 센다. caller-dependent 측정(`order.cpp` 의 `pair_values_`)은 두 쪽에 뿌리가 있어 `pair()` 를 계산한 쌍만 기록한다. 그래서 24,796 − 187 = 24,609 다.
  - 글의 unknown 331 은 이 187 과 경로 B·C 의 한 스레드 쌍 144 를 더한 것이다. 쌍 파일은 그 144 를 `same_thread` 로 적는다.
- 설계 행에 대한 비판(쌍 파일의 영역)은 이제 이렇게 답한다. 탐침 행의 같은 함수 생성(`bp_flow.c:132:18`)은 `creator-not-a-pair(probe-create) … same_thread` 줄로 파일에 있다. 다른 응용의 생성은 `creator … unordered` 줄이다. 시작 때 프로세스 행위자가 만든 것은 `other_root` 가 `main (process)` 인 줄로 따로 읽힌다.

### 15.5 공간 [M]
- 시작 1,247,948,800 B(`free_start.txt`, 02:12Z).
- 이 실행의 가장 낮은 값은 940,474,368 B 이고 하한을 넘지 않았다(`scratch_guard/`). scratch 빌드 tree 는 약 122 MB, 다시 뽑은 다섯 빌드는 약 71 MB 였다. make 의 dip 은 처음 114,311,168 B 이고, 다시 빌드할 때는 수 MB 였다.
- scratch 를 모두 지운 뒤 1,228,222,464 B(`free_after_scratch_removed.txt`). 기록 디렉터리는 약 1.0 MB 다.
- 압축 대체는 쓰지 않았다.

### 15.6 계획과 다르게 한 것
- **받은 결과 글이 끊겨 있었다.** 글 단계의 결과 글은 (6) 항의 중간에서 끊겼고, 이름 공간 자료 단계의 글은 없었다. §13 의 (6)·(7) 항과 §14 는 그 단계들의 커밋과 기록에서 적었다(각 절의 머리에 밝힘).
- **비판 파일 경로가 없었다.** 지정된 비판 JSON 둘은 경로에 없었다. 둘째 비판은 같은 내용의 텍스트 사본(scratch)에서 읽었다. 첫 비판은 읽지 못했으나, 그 처리 상태는 둘째 비판의 첫 항목이 적고 있다.
- **형식에 정해지지 않은 것이 드러났다.** 코드를 쓰며 다섯 가지를 덧붙임으로 정하고 코드보다 먼저 커밋했다: 같은 행위자만의 not-a-pair 종류, 한 스레드의 값, `edges.tsv` 에 없는 간선, 쌍을 글 뒤에 계산하는 것, `source_tree` 를 찾는 규칙. 먼저 계산 순서를 고치지 않은 판은 `--pairs` 가 글의 `HB-seq steps` 수를 바꿨다(fixture 81 → 82). scratch 점검이 이것을 잡아 고쳤다.
- **scratch 점검에서 잡은 결함 둘.**
  - 첫 판은 `same actor only` 접두를 길이 34 로 비교해 그 생성자를 놓쳤다(`scratch/pairs_trial1`, `pairs_trial2`). 접두 비교로 고쳤다.
  - 점검 스크립트가 처음에는 `control-state=planned` 행을 세지 않았다(9 of 9 → 13 of 13, `scratch/pairs_check3`). 모든 시도 출력은 새 이름으로 남겼다.
- **fixture 가 없는 줄 둘.** `source_tree` 항목과 `absence origins:` 줄은 fixture 가 그 줄을 내지 않는다. 지금 fixture 는 모두 묶음 루트 안에 있고, 결과 흐름 fixture 는 `--object-access` 를 쓰지 않는다. 선언 전에 fixture 를 더해야 한다(15.7).

### 15.7 열린 것과 다음 차례
kz-static 한 빌드에 아직 넣지 않은 것은 다음과 같다. 차례는 형식이 이미 커밋된 5x 의 순서다.
1. 생성자의 성공 경로 줄(5x 1 항).
   - out-id 저장은 callee 의 PARAM_FLOW out-store 행에서 읽는다.
   - 저장되는 값의 정의를 성공 경로에 묶는다.
   - 조건 나무의 함의를 판정하고, 두 방향의 fixture 를 둔다.
   - 다섯 빌드의 모든 creator callee 에서 성립과 누락을 까닭별로 낸다.
2. holder 줄(3 항). 모든 writer 를 값 종류와 함께 적고, 뒤의 reader 가 지운 값을 가르는지 본다.
3. 문맥별 대안 줄과 필드 저장값 규칙(4 항).
   - 경로 조건으로 보고, 줄 차례와 경로 차례가 다른 fixture 를 둔다.
   - 빌드마다 열린 효과를 사유 종류별로, 규칙이 정한 호출 자리 수를 낸다.
   - 설계 행 밖의 진술에는 쓰지 않는다.
4. 저장된 이름 버퍼 줄과 수명 끝 줄(5 항).
   - FIELD_STATUS 의 스칼라 배열 상태가 원소 저장과 decay 를 모으면 완전해질 수 있는지를 scratch 출력으로 정한다. 안 되면 깃발마다 까닭을 적는다.
   - 이름 포인터 필드의 writer 를 종류별로 나눈다.
5. 계획 규칙 개정 5. `framework_namespaces2_*` 의 새 문장을 읽는 쪽, `namespace-entry`·`namespace-layer` 줄, 생성 쪽 종류 값 site, 문맥 틀 site, 수명 끝 site, 라벨 둘을 넣는다. 선택 규칙은 그대로이고 층 라벨도 그대로다.
6. 규칙 개정 이름을 바꾼다(relations 14, order 8, plan 5).
7. 위 모든 줄과 `source_tree`·`absence origins` 의 fixture 를 성립 꼴과 누락 꼴로 만든다.
8. 그 뒤의 차례:
   - scratch 점검 빌드에서 [I] 와 첫 ctest 실패를 이름으로 얻는다.
   - 선언(허용 차이는 출력 종류별, relink 는 dry run 에서, 공간 포함)을 커밋한다.
   - 사본을 만들고 MC 를 빌드한다.
   - 누적 점검과 다섯 빌드 다시 뽑기를 한 번 한다(relations, 쌍 파일을 낸 order, 새 자료 파일 위의 개정 4·5 계획).
   - 끝 점검(키 글 같음, 쌍 파일 완전, 탐침을 도구 줄이 재현, 새 줄마다 다섯 빌드에서 성립하는 곳과 빠지는 곳)을 하고 결과 절을 쓴다.
- §12.11 (사)의 key-clearing 이름 문제(할당 때의 자리 비우기를 지우기로 셈)는 위 4·5(수명 끝 꼴의 `clear-slot`, 범위 규칙 개정 2 의 `layer-op clear-slot`)와 함께 다뤄야 한다. 다루는 방법은 lead 의 판단을 기다린다.
- §12.10 5 항의 PI 결정은 그대로 기다린다(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)).

## 16. C1 닫기의 kz-static 한 빌드: 소스 진행분 둘째 (2026-11-19, 선언 전에 멈춤)

§15.7 의 다음 차례 1·2(생성자의 성공 경로 줄, holder 줄)를 형식 덧붙임 → 소스 → scratch 점검의 차례로 넣었다. **선언, 도구 사본, MC 빌드, 다섯 빌드의 선언된 다시 뽑기는 하지 않았다**: 넣을 것이 아직 다 들어가지 않았으므로 §12.10 6 항대로 선언 전에 멈췄다. MC 의 kz-static 은 `bf2ec2ee…` 그대로이고(이 실행 끝에 sha256 을 다시 보았다), 소스가 그 바이너리보다 더 앞서 있다. 기록: `../records/closure-kzstatic-20261119/`(`scratch2/`, `scratch_guard2/`, `scripts/holder_counts.py`, README). cFS 를 부팅하지 않았고 push 하지 않았다. 아래 수는 모두 기록 디렉터리의 출력 파일에서 왔고, scratch 도구의 수라서 [I] 다.

### 16.1 바뀐 것 [F]
- 형식 덧붙임 다섯째(`6023304`, `kairos/docs/static_facts.md` 5x): 성공 경로 줄을 자세히 정한다. 성공 값은 outcome class rule 의 성공 상수다. 운반·제외 대안과 `its own branch` 의 조건을 정한다. 조건 나무는 원자 위의 식으로 읽고, **함의는 반례 배정이 없을 때만 선다**(반례가 있어도 "서지 않음"이 아니라 "보이지 않음"이다). callee 의 나무에 매개변수와 지역 변수를 묶는 법, 이름 저장, out-id 저장, out-store 행의 글로 본 저장 값, 까닭의 차례를 둔다. 이름 공간 블록에는 그 이름 공간의 모든 생성자 callee 의 줄을 둔다(설계 행에만 보이는 줄이 없게).
- 소스(`a60f77b`; `lib/llvm/relations.cpp`, `lib/llvm/outcome.hpp`, `tests/static/fixtures/closure.c`, `tests/static/CMakeLists.txt`, `tests/static/static_golden.cpp`): relations rules revision 14 의 1 항 줄을 넣었다(행마다 생성자 callee 의 줄, 이름 공간 블록의 줄, 머리의 `summary success path:`). 새 fixture `closure.c` 와 golden 경우 `closure_np`(`--result-flow --object-access`)를 더했다. fixture 에는 생성자 API 다섯이 있다: 성립 둘(상수 반환, 상태를 되돌려 주는 마무리 단계), 누락 셋(두 번째 기원, 지역 변수를 거친 id, 다른 갈래의 이름 저장).
- 형식 덧붙임 여섯째(`757ee59`): holder 줄을 자세히 정한다. 자리, 필드·지역·그 밖의 holder, WRITERS 행 모두와 종류, FIELD_STATUS 를 막은 깃발과 ATYPE 위치, reader 와 값의 시험을 둔다. 경로 B 행에는 조회의 `id holder:` 줄이 없으므로 이 줄도 없다.
- 소스(`48048d3`; `relations.cpp`, `rel_facts.hpp`·`rel_facts.cpp` 가 ATYPE 을 읽음, `closure.c`): relations rules revision 14 의 3 항 줄을 넣었다. fixture 에 필드 holder(지우는 writer 와 reader 둘), 전역 배열 holder, 지역 holder 를 더했다.
- 바꾸지 않은 것: 규칙 개정 이름(§15.2 와 같은 까닭). 저장소의 golden 도 바꾸지 않았다: 새 golden `closure_np` 는 `scratch2/golden_scratch/closure_np.golden_2` 에만 있고 선언된 빌드에서 커밋한다. 그래서 HEAD 를 빌드하면 order golden 셋과 `closure_np` 가 실패한다 [I].

### 16.2 scratch 점검 [I]
scratch Release 빌드는 kz-static, static_golden, kz_lint, test_static_reasons 이고, 모두 감시 아래에서 돌렸다(`scratch_guard2/t01`–`t20`). 다섯 빌드의 relations 는 남긴 마지막 옵션 회차 사실(`rf2_final_extract`)에서 돌렸다.

| 점검 | 결과 | 출력 |
|---|---|---|
| golden·fixture 컴파일·lint(새 golden 을 잠시 놓고) | 68 가운데 실패 3 = order golden 셋(§15.3 과 같음); `closure_np` 통과; lint 8 통과 | `scratch2/logs/ctest_golden_lint_2.log` |
| test_static_reasons(새 golden 을 놓고) | 통과 | `scratch2/logs/ctest_reasons.log` |
| fixture 의 성공 경로 줄 | callee 5: held 2(`ent_create`, `ent_create_fin`), missing 3(a second origin 1, not tied 1, not decided 1) | `scratch2/success_path/fixture_closure_relations.txt`, `golden_scratch/closure_np.golden_2` |
| 다섯 빌드의 `summary success path` | callee 14/14/12/15/15, held 0/0/0/0/0. 누락 까닭: no out-id store 3/3/2/5/5, not tied 4/4/3/4/4, not decided 7/7/7/5/5, returns not followed 0/0/0/1/1 | `scratch2/success_path/summary.txt`, `lines_<b>.txt` |
| 다섯 빌드의 holder 줄 | 줄 26/26/21/18/18 (field 8/8/2/0/0, local 17/17/18/18/18, 그 밖 1/1/1/0/0); 필드 holder 의 reader: 시험 뒤 18/18/5/0/0, 시험 없음 26/26/4/0/0, 정하지 않음 0 | `scratch2/holder/counts_<b>.txt`, `lines_<b>.txt` |
| relations 글의 다른 줄 | 다섯 빌드 모두 앞 도구의 `rf2_final_order` 와 비교해 더한 줄(도구 줄, `absence origins`, 성공 경로의 줄들)만 다름(holder 줄을 넣기 전의 판으로 셈) | 이 실행의 diff 출력은 scratch 에만 있었고 남기지 않았다 |

### 16.3 성공 경로 줄이 보인 것 [I]
- **C1 의 OSAL 생성자(`OS_BinSemCreate`, `OS_CondVarCreate`, `OS_CountSemCreate`, `OS_MutSemCreate`)는 이름 저장과 out-id 저장이 운반 조건으로 함의된다.** 그러나 저장되는 값이 묶이지 않는다. out-id 는 `OS_ObjectIdFinalizeNew` 의 지역 변수 `final_id` 다(`osapi-idmap.c:860`). 이 사실이 보이는 정의는 `token->obj_id`(`:841`, READS)뿐이다. 다른 정의 `OS_OBJECT_ID_UNDEFINED` 대입(`:845`)과 그 주소가 넘어가는 `OS_ObjectIdTransactionFinish`(`:849`, ARGFLOW)는 사실에 정의로 실리지 않는다. 그래서 줄은 `missing(the stored value is not tied to the success path)` 이고, 판정 규칙의 기초 4 는 누락으로 센다(§12.10 4 항의 "성공 경로 줄이 설 때만").
- 이 줄이 서려면 추출이 out-store 된 지역 변수의 도달 정의를 사실로 내야 한다(RETURNS 가 반환된 지역 변수에 하는 것과 같은 규칙). 지금 형식(`include/` 를 바꾸지 않음)에는 그 자리가 없다. 이것을 열린 것으로 둔다(16.6).
- 나머지 C1 생성자는 이름 저장이 callee 가 부르는 함수에만 있다(CFE_ES·CFE_SB 쪽). 이 개정은 그 사이 호출의 guard 를 읽지 않으므로 정하지 않는다. `OS_initfs`·`OS_mkfs`·`OS_mv` 에는 out-store 행이 없다.
- 탐침 P2 의 읽기(§15.1: 원형은 다섯 callee 가운데 하나에서만 세운다)는 저장 값의 묶임을 보지 않았다. 도구의 줄은 그것까지 보므로 C1 에서 하나도 세우지 않는다. **탐침과 도구 줄의 차이는 이 규칙 차이 하나다**(이름·out-id 저장의 성립은 같다: `OS_CountSemCreate` 의 두 저장).

### 16.4 holder 줄이 보인 것 [I]
- A21(`CF_Channel_t.sem_id`)의 holder 줄은 탐침 P4 의 내용을 도구 줄로 다시 낸다.
  - writer 둘: 엔진을 지우는 memset(`cf_cfdp.c:965`, 값 0)과 조회 자신의 out 저장.
  - FIELD_STATUS 는 두 규칙 모두 DISABLED 다. 막은 것은 깃발 다섯, 주소 탈출 둘(`cf_cfdp.c:1353`), cast 하나(`cf_cmd.c:734`)다.
  - 뒤의 reader `cf_cfdp_sbintf.c:93:49` 는 `OS_ObjectIdDefined` 의 시험 뒤에 쓴다. 91:35 의 읽기는 그 시험 자체다.
- A20(`BP_Throttle_t.Sem`)의 holder 줄: 다른 정의 하나(`bp_flow.c:132:18`, `OS_CountSemCreate` 를 거친 out-pointer)가 있다. reader 셋은 모두 시험 없이 쓴다. 탐침 P4 의 "다른 정의" 와 같다.

### 16.5 공간 [M]
- 시작 1,226,493,952 B(`free_start_attempt2.txt`, 02:46Z).
- 이 실행의 가장 낮은 값은 931,139,584 B 다(`scratch_guard2/`). scratch 빌드 tree 는 202 MB, 사실 보기 하나(c1_pre_tx)와 relations 출력은 77 MB 였다.
- scratch 를 모두 지운 뒤 1,222,668,288 B(`free_end_attempt2.txt`, 03:14Z). 압축 대체는 쓰지 않았다.

### 16.6 계획과 다르게 한 것, 열린 것
- **out-store 행의 값.** 이 사실은 out-store 행의 저장 값을 상수일 때만 나무로 싣고, 나머지는 쓴 글로만 싣는다(`lib/llvm/argflow.cpp:165-174`). 그래서 저장 값의 묶임은 글로 정한다(덧붙임 다섯째). 지역 변수로 저장된 id 는 어느 빌드에서도 묶이지 않는다. 추출 규칙을 더할지(형식과 `include/` 의 변경)는 lead 의 판단을 기다린다.
- **함의를 두 값으로만 낸다.** 반례 배정이 있어도 원자 사이의 관계를 사실이 주지 않으므로 "함의되지 않음"이라 쓰지 않는다. 이것은 형식 1 항의 까닭 글 여섯 가운데 `no name store`·`no out-id store` 를 "저장이 아예 없음"에만 쓰게 한다.
- **성공 값의 출처.** 성공 값은 `callee-check (result flow, success)` 가 보인 값이 아니라 outcome class rule 의 성공 상수다. 성공 상수를 바로 돌려주는 callee 는 RESULT_EFFECT 가 반환에 닿는 값을 보이지 않는데(fixture 에서 확인), 형식 1 항의 `[outcome class rule]` 도 이것을 가리킨다.
- holder 줄은 경로 A 행에만 있다(경로 B 행에는 조회가 없다).
- §15.7 의 다음 차례 가운데 남은 것은 이렇다.
  - (3) 문맥별 대안 줄과 필드 저장값 규칙
  - (4) 저장된 이름 버퍼 줄과 수명 끝 줄
  - (5) 계획 규칙 개정 5
  - (6) 개정 이름 바꾸기
  - (7) 나머지 줄의 fixture: `source_tree`, `absence origins`, 위 (3)–(5)
  - (8) scratch 점검 빌드 → 선언 → 사본 → MC 빌드 → 다시 뽑기 → 끝 점검 → 결과 절
- §15.7 의 key-clearing 이름 문제와 absence origins 의 이름 칸 문제, §12.10 5 항의 PI 결정(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함))은 그대로 남아 있다.

## 17. C1 닫기의 kz-static 한 빌드: 소스 진행분 셋째 (2026-11-19, 선언 전에 멈춤)

§16.6 의 다음 차례 (3)(문맥별 대안 줄과 필드 저장값 규칙)을 형식 덧붙임 → 소스 → scratch 점검의 차례로 넣었다. **선언, 도구 사본, MC 빌드, 다섯 빌드의 선언된 다시 뽑기는 하지 않았다**: 넣을 것이 아직 다 들어가지 않았으므로 §12.10 6 항대로 선언 전에 멈췄다. MC 의 kz-static 은 `bf2ec2ee…` 그대로이고 소스가 그 바이너리보다 더 앞서 있다. 기록: `../records/closure-kzstatic-20261119/`(`scratch3/`, `scratch_guard3/`, `scripts/alt_counts.py`, README). cFS 를 부팅하지 않았고 push 하지 않았다. 아래 수는 모두 기록 디렉터리의 출력 파일에서 왔고, scratch 도구의 수라서 [I] 다.

### 17.1 바뀐 것 [F]
- 형식 덧붙임 일곱째(`cc0f38b`, 고침 `a5b4c31`, `kairos/docs/static_facts.md` 5x): 4 항을 자세히 정한다. 규칙은 판정 규칙 8.13 의 17 번 그대로다. 이 사실에는 함수의 제어 흐름 그래프가 없다. 그래서 (5)의 경로 조건은 추출이 그 그래프에서 구해 exit·terminal 행의 `chain` 글에 싣는다. (0)–(6)을 사실의 어느 행에서 보는지, 정한 결과로 열린 효과를 닫는 조건, 호출자를 따라 올라가는 걷기(`return-const` 도 따라감), `rests on`·`premises`, 프로그램 전체를 세는 머리 줄을 정한다. 고침은 `toward` 를 "K 를 다시 지나지 않고 효과에 닿는 후속"으로 좁힌다. 그래야 K 위의 고리 몸이 효과로 가는 갈래로 읽히지 않는다.
- 소스(`6b48311`):
  - `lib/llvm/ast_fn.cpp`: 저장한 위치의 주소를 받는 호출 K 마다 `chain` 끝에 ` [path from the store at …]` 덩이를 붙인다(`rf_span`, `rf_path_chunk`). S 에서 K 까지 경로 위의 다른 호출 수와 그 위치의 저장 수, S 에서 효과까지의 분기와 그 `toward` 를 담는다. `reason`·`status` 와 다른 행은 바뀌지 않는다. `include/` 는 그대로다.
  - `lib/llvm/relations.cpp`: `alternatives [context k: …]:` 줄과 머리의 `rules with decided call results:` 줄을 쓴다(`sf_rule`, `sf_closes`, `alt_walk`, `alternatives_lines`, `sf_header`). layers 칸에는 아무것도 더하지 않는다.
  - `tests/static/fixtures/closure.c`, `tests/static/CMakeLists.txt`(설명): 다음 경우를 더했다.
    - 줄로는 저장과 호출 사이에 있으나 어느 경로에도 없는 저장(`goto`)이 있는 경우 — 규칙이 정한다.
    - 줄로는 exit 뒤에 있으나 경로에서는 호출 앞에 오는 저장이 있는 경우 — (5)로 정하지 않는다.
    - 실패를 다른 상수로 바꿔 돌려주는 틀, 둘째 호출자 문맥, 재시도.
    - 널 검사가 없는 같은 표의 조회 API. 처음 있던 조회는 앞의 널 검사 때문에 부재 값이 정해지지 않는다.
- 바꾸지 않은 것: 규칙 개정 이름. 저장소의 golden 도 바꾸지 않았다. 새 golden 은 `scratch3/golden_scratch/`(`closure_np.golden_3`, `resultflow_np.golden`)에만 있고 선언된 빌드에서 커밋한다.

### 17.2 scratch 점검 [I]
scratch Release 빌드는 kz-static, static_golden, kz_lint, test_static_reasons 이고, 모두 감시 아래에서 돌렸다(`scratch_guard3/u01`–`u08`). 다섯 빌드는 scratch 도구로 `run.sh extract_final` 한 번(`u05`, rc 0)으로 다시 뽑았다. facts sha256 은 `scratch3/facts_scratch_sha256.txt` 에 있다.

| 점검 | 결과 | 출력 |
|---|---|---|
| fixture 의 대안 줄 | runner: `none found (exploration complete)`, `decided calls: run_loop … result 0 by the stored-field rule (store …:229:5 := 3)`, `rests on: P0-unverified(exit)`. 둘째 문맥(cmd): 스레드 뿌리까지 돌아가 `none found`. runner2: (5)로 정하지 않음(`stores of field:RunState.mode 1 (first 238:3)`, 줄로는 exit 뒤). waiter: `found: retry`. 처음 조회 셋: `absence value is not evaluated`. 머리: 주소 넘김 3(호출 자리 2), 정함 2, 정한 호출 자리 1(행 1) | `scratch3/golden_scratch/closure_np.golden_3` |
| golden·fixture 컴파일·lint 68 | 실패 5: order golden 셋(§15.3), `resultflow_np`(facts 의 `chain` 덩이 하나만 다름, `resultflow_np.golden` 과 diff 한 줄), `closure_np`(저장소에 golden 없음). lint 8 통과 | `scratch3/logs/ctest_golden_lint_3.log` |
| relations 글(다섯 빌드, 다시 뽑은 사실) | `rf2_final_order` 와 비교해 없어진 줄은 `facts` 줄 하나뿐. 더한 줄 166/165/89/159/159(도구 줄, 개정 14 의 줄들) | `scratch3/relations_text_diff.txt` |
| 머리의 필드 저장값 규칙(c1_pre_tx/c1_post_tx/r2/c5_641/c5_642) | 주소 넘김 72/72/0/0/0(호출 자리 2/2/0/0/0), **정함 0**. 정하지 않은 까닭(C1 두 빌드 같음): (5)·(6)이 전제 66, 저장이 값이 정하지 않는 경로 위 2, 저장한 위치가 필드가 아님 4 | `scratch3/alternatives/header_<b>.txt` |
| 열린 효과의 까닭 종류 | branch not decided 6,812/6,817/4,176/4,150/4,157; outcome not enumerated 3,857/3,857/2,771/1,929/1,929; address passed 72/72/0/0/0; other 2,398/2,398/1,884/984/984 | 같은 파일 |
| 대안 줄 | 줄이 있는 행 63/63/25/66/66, 줄 65/64/26/66/66. `none found` 3/3/3/0/0. found(retry 또는 같은 함수의 생성; 모두 정해지지 않은 문맥과 함께) 5/6/2/0/0. not established: 부재 값 없음 24/24/8/43/43, 열린 효과 17/16/7/19/19, 효과가 돌아가는 함수에서 끝남 9/8/1/4/4, 그 지점에 그 값의 RESULT_EFFECT 행 없음 7/7/5/0/0. 전제가 적힌 줄 1/0/0/0/0, 정한 호출이 있는 줄 0 | `scratch3/alternatives/counts_<b>.txt`, `lines_<b>.txt` |

### 17.3 대안 줄이 보인 것 [I]
- **결함 빌드의 관계 1(A21), 초기화 문맥에서 규칙이 결과를 정하지 않는다.**
  - (5)는 경로로 선다. 저장 `cf_app.c:333` 에서 `CFE_ES_RunLoop`(`cf_app.c:338:12`)까지 다른 호출 0, 저장 0 이다.
  - (6)이 서지 않는다. 판정 규칙 17 번은 X 의 writer 집합이 **필드 단위와 형 단위 모두** 완전하기를 요구한다. 그런데 `CF_AppData_t.run_status` 는 per-field=writer-set-complete, type-level=DISABLED 다. 그래서 줄은 `not established (open: exit-app at cf_app.c:366:5 …)` 이고 `premises: (6) the writer set of CF_AppData_t.run_status is complete under both rules (…type-level=DISABLED)` 다.
  - §12.10 4 항의 [예비] 읽기("(c3)(iii): 초기화 문맥에서는 대안이 없음을 보일 수 있다")는 이 규칙 글로는 도구 줄이 되지 않는다. 탐침 P1 은 필드 단위 상태만 보았다(`p1_alternatives.py` 의 `pf[0] == "writer-set-complete"`). **도구 줄과 탐침의 차이는 이 규칙 차이 하나다.**
  - 규칙을 그 행에 맞춰 풀지 않았다. 형 단위 조건을 빼는 것은 판정 규칙의 글을 바꾸는 일이라, lead 또는 PI 의 판단을 기다린다(17.5).
- 같은 행의 둘째 문맥(`CF_CmdEnableEngine@cf_cmd.c:1119:13`)은 탐침과 같다: `not established (the value's effects end in CF_CmdEnableEngine and it returns to its caller: direct callers 0, called through a function pointer: yes)`.
- 탐침 행 A20(두 빌드): `found: creation in the same name space and partition by the same function (bp_flow.c:132:18 -> OS_CountSemCreate …); retry (… open)`. 탐침과 같은 두 항목이다. 다만 도구 줄은 그 틀의 열린 event-send 둘을 `[not established (…)]` 로 함께 적는다.
- 고친 빌드의 A21: `found: retry (cf_cfdp.c:1003:23 …)` 이고, 문맥은 하나다. 첫 틀에 열린 `return-result`(`cf_cfdp.c:1053`)가 있으면 도구는 거기서 그 문맥을 끝낸다(17.5). 탐침은 반환을 따라 문맥 둘을 냈다.
- 시간 기준 대조 행: 탐침이 따라가지 않았던 `return-const` 틀을 도구는 따라간다.
  - A22: `SCH_Lab_AppMain` 의 열린 exit-app 에서 `not established`.
  - A26: `CFE_TIME_TaskMain` 의 호출에 값 0 의 RESULT_EFFECT 행이 없어 `not established`.
- 다섯 빌드 어디에서도 규칙이 정한 호출 자리는 0 이다. 모집단은 C1 의 호출 자리 둘이고, 그 가운데 하나가 `CFE_ES_RunLoop` 의 자리다(전제 66 을 가진 덩이, 17.3). 다른 자리가 머리의 "저장한 위치가 필드가 아님 4"·"값이 정하지 않는 경로 2"의 덩이인지는 이 실행에서 따로 보지 않았다 [미확인]. 앞 탐침의 모집단(`probes/out/open_reason_population.txt`: 필드의 주소를 넘기는 자리 1/1/0/0/0)과 함께 읽으면, 규칙이 필드에 적용되는 자리는 하나다. 이 결과를 설계 행 밖의 진술에 쓰지 않는다.

### 17.4 공간 [M]
- 시작: 1,219,821,568 B(`free_start_attempt3.txt`, 03:31Z).
- 감시 기록의 가장 낮은 값: 924,598,272 B(`scratch_guard3/`).
- 크기: scratch 빌드 tree 203 MB, 다시 뽑은 다섯 빌드 70 MB, relations 출력 7.8 MB.
- scratch 를 모두 지운 뒤: 1,218,379,776 B(`free_end_attempt3.txt`, 03:41Z).
- 압축 대체는 쓰지 않았다.

### 17.5 계획과 다르게 한 것, 열린 것
- **(5)를 추출에서 구한다.** 이 사실에는 함수의 제어 흐름이 없다. 그래서 경로 조건은 추출이 CFG 에서 구해 RESULT_EFFECT 의 `chain` 글에 싣는다. 선언된 빌드에서 다섯 빌드의 facts 가 이 글만큼 바뀐다: RESULT_EFFECT 의 글과 STRINGS. fixture 에서는 `resultflow_np` 의 한 줄이다. 열린 행의 `chain` 은 어느 출력 줄도 인쇄하지 않으므로 relations·order·plan 의 있는 줄은 바뀌지 않는다(17.2).
- **(6)의 형 단위 조건이 설계 행을 막는다.** 위 17.3. 규칙 17 번의 글을 그대로 썼다. 읽는 방법은 lead 의 판단을 기다린다. 필드 단위만으로 읽는 것도 갈래로 둘 수 있다. 다만 그것은 판정 규칙의 덧붙임이고, 부팅 전에 커밋해야 한다.
- **열린 효과가 있는 틀에서 문맥을 끝낸다.** 그 틀의 반환을 따라 올라가지 않는다. 그 문맥의 결론은 어차피 `not established` 다. 그래서 고친 빌드의 A21 은 문맥이 하나다. 덧붙임 일곱째의 "닫히지 않은 열린 행이 있으면 `not established`"를 그대로 따른 것이다.
- 경로 B 행에는 이 줄이 없다(부재 값이 없음; 덧붙임에 적음).
- 열린 효과의 까닭 가운데 `other`(C1 에서 2,398)가 무엇인지는 보지 않았다 [미확인].
- test_static_reasons 는 이번 scratch 에서 돌리지 않았다(새 golden 을 저장소 자료 디렉터리에 놓지 않았기 때문).
- 선언의 첫 ctest 실패 목록에 `static_golden_resultflow_np` 가 더해진다. 지금까지는 order golden 셋과 `static_golden_closure_np` 였다.
- §16.6 의 다음 차례 가운데 남은 것:
  - (4) 저장된 이름 버퍼 줄과 수명 끝 줄. 탐침 P3 의 출력(`probes/out/p3_buffer_writers_c1_pre_tx.txt`)에서 이름 버퍼 열하나는 모두 per-field·type-level DISABLED 이고, 깃발은 `address-escape` 다. 그 출력이 적은 FIELD_STATUS 깃발마다 원소 저장·decay·형 없는 쓰기가 풀리는지 scratch 출력으로 정해야 한다. 그다음 줄을 쓴다.
  - (5) 계획 규칙 개정 5.
  - (6) 개정 이름 바꾸기.
  - (7) 나머지 줄의 fixture(`source_tree`, `absence origins`, (4)·(5)).
  - (8) scratch 점검 빌드 → 선언 → 사본 → MC 빌드 → 다시 뽑기 → 끝 점검 → 결과 절.
- §15.7 의 key-clearing 이름 문제, absence origins 의 이름 칸 문제, §12.10 5 항의 PI 결정(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함))은 그대로 남아 있다.

## 18. C1 닫기의 kz-static 한 빌드: 소스 진행분 넷째 (2026-11-19, 선언 전에 멈춤)

§17.5 의 다음 차례 (4)(저장된 이름 버퍼 줄과 수명 끝 줄)를 형식 덧붙임 → 소스 → scratch 점검의 차례로 넣었다. **선언, 도구 사본, MC 빌드, 다섯 빌드의 선언된 다시 뽑기는 하지 않았다**: 넣을 것이 아직 다 들어가지 않았으므로 §12.10 6 항대로 선언 전에 멈췄다. MC 의 kz-static 은 `bf2ec2ee…` 그대로이고(시작과 끝에 sha256 을 보았다, `scratch4/mc_tool_sha256_start.txt`·`_end.txt`), 소스가 그 바이너리보다 더 앞서 있다. 기록: `../records/closure-kzstatic-20261119/`(`scratch4/`, `scratch_guard4/`, `scripts/buffer_counts.py`, README). cFS 를 부팅하지 않았고 push 하지 않았다. 아래 수는 모두 기록 디렉터리의 출력 파일에서 왔고, scratch 도구의 수라서 [I] 다.

### 18.0 lead 의 판단 (2026-11-19, §17 의 열린 것에 대해)
lead 가 2026-11-19 에 정한 것을 그대로 적는다.
- (a) 판정 규칙 8.13 의 17 번 조건 (6)은 쓰인 대로 적용한다: writer 집합이 필드 단위와 형 단위 **모두**에서 완전해야 한다. 도구를 필드 단위로만 읽게 바꾸지 않는다. 필드 단위만으로 읽는 것은 설계 행을 본 뒤의 규칙 읽기다. 이미 커밋된 평가 갈래 Re 가 그것을 덮는지는 설명 스크립트 단계와 평가 파일의 일이다. Re 의 커밋된 글이 17 번 조건 (6)을 덮지 않으면 PI 의 열린 것으로 적고, 지금 어떤 글도 넓히지 않는다.
- (b) 성공 경로 줄에서 out 저장된 지역 변수의 추출 틈은 그대로 둔다: 이 빌드에서 `include/` 아래는 바꾸지 않는다(모든 바이너리가 다시 링크된다). 성공 경로 줄은 그 까닭으로 계속 missing 이고 열린 것이다.
- (c) 저장된 이름 버퍼: 스칼라 배열의 FIELD_STATUS 상태가 건전하게 complete 가 될 수 있는지를 scratch 출력으로 깃발마다 정한다. 건전하게 뺄 수 없는 깃발이 하나라도 있으면 줄은 not complete 와 그 깃발을 말하고, 설계 행을 맞추려고 아무것도 풀지 않는다.
- (d) 바뀌는 golden(order golden 셋, `resultflow_np`, `closure_np` 와 새로 생기는 것)은 선언된 빌드에서 커밋한다. 선언은 첫 ctest 실패를 **완전한 소스**의 scratch 점검 빌드에서 확인한 이름으로 적는다.
- (e) C1 빌드의 둘째 주소 넘김 호출 자리와 열린 까닭의 `other` 종류는 빌드 단계의 측정에 들어가면 그때 가리고 세고, 아니면 열린 것으로 둔다.

### 18.1 바뀐 것 [F]
- 형식 덧붙임 여덟째(`1859bb6`, `kairos/docs/static_facts.md` 5x)와 그 고침 둘(`16036b1`, `05c4de5`), 모두 코드보다 먼저 커밋했다. 5 항을 자세히 정한다: 쓰는 사실과 자리(경로 A 이름 공간 블록의 `success path …` 줄들 뒤, `uses:` 앞), 키 필드 writer 의 종류 묶음(그 함수의 partition, 아니면 그 함수에 직접 호출로 닿는 키 매개변수 함수들의 partition), 이름 저장과 버퍼 필드(같은 자리의 decay 탈출 행), 버퍼 줄의 꼴, 원인을 깃발마다 정하는 법(18.2), address uses 줄(이름 포인터의 READS 행마다; 본문은 PARAM_FLOW 깊이 0 으로 따라가고, 본문 없는 함수는 SEMANTICS 의 `out-behaviour` 항목으로), 수명 끝의 네 꼴(`clear-table`, `clear-slot` 과 빈 레코드 조건, `release`, `release through parameter`)과 각 꼴이 id 를 어떻게 이름 대는지, 머리 줄. 고침 둘은 scratch 판을 본 뒤에 했다: 매개변수를 거친 release 의 호출자는 creator 함수 자신이거나 creator 가 한 단계로 부를 때만 생성 쪽(`rollback`)이다(여러 단계로 닿으면 생성자가 실패를 치우려고 부른 삭제일 수 있다; c1 에서 삭제 쪽 함수가 creator 셋에서 여러 단계로 닿았다).
- 소스(`638e500`):
  - `lib/llvm/relations.cpp`: `buffer_lifetime_lines`, `lifetime_lines`, `param_use`, `library_use`, `bl_header`. 새 줄은 행의 layers 칸과 basis 편차에 아무것도 더하지 않는다(`NoNotes`).
  - `lib/llvm/rel_facts.hpp`·`rel_facts.cpp`: FIELD_TYPES 를 읽는다(버퍼의 extent, 키가 포인터 필드인지).
  - `tests/static/fixtures/closure.c`, `tests/static/CMakeLists.txt`(설명): 이름 포인터의 공통 표와 두 종류 표, 다른 writer(이름 바꾸기), 포인터를 시험하고·읽기만 하는 함수에 넘기고·그것을 통해 쓰는 함수에 넘기고·전역에 베끼는 쓰임, 빈 레코드 조건이 보이는 clear-slot 과 보이지 않는 clear-slot, creator·삭제·널 포인터로 불리는 마무리 단계, 상수 release, 표 채움.
- 바꾸지 않은 것: `include/`, 추출(다섯 빌드의 facts 는 셋째 실행의 scratch 판과 같은 sha256 이다, `scratch4/facts_same_as_scratch3.txt`), FIELD_STATUS, 규칙 개정 이름, 저장소의 golden. 새 golden 은 `scratch4/golden_scratch/closure_np.golden_4` 에만 있고 선언된 빌드에서 커밋한다.

### 18.2 lead 의 판단 (c): 깃발마다 정한 것 [I]
FIELD_STATUS 가 DISABLED 가 되는 원인은 `lib/llvm/lockset.cpp:589-597` 의 셋뿐이다: 그 필드의 적중(escape·cast·union·unknown), 형 단위에서 다른 필드의 적중과 형 없는 쓰기 자리가 함께 있는 것, 스칼라 배열 표시. `writer-function-not-once`·`several-instances`·`writer-in-loop` 는 `:636-643` 에서 `once` 판정에만 쓰이고 상태를 바꾸지 않는다.

| 깃발 | 판단 | 근거 | 다섯 빌드의 버퍼 줄(c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642) |
|---|---|---|---|
| `array-element-stores-not-collected` | `--object-access` 아래에서 **풀린다** | `ast_fn.cpp` `note_array_decay`(:641-788): decay 된 배열의 원소 저장은 writer 행, 호출 인자는 out-pointer 호출, 나머지 쓰임은 주소 탈출 적중. 탐침(`scratch4/probe/probe_view.txt`): 대상이 아닌 스칼라 배열 필드의 원소 저장이 WRITERS 행 없이도 FIELD_STATUS 의 writer 로 세어진다(`Rec.name writers=1`) | discharged 16/16/17/0/0 |
| 주소 탈출(`address-escape`) | **풀리지 않는다** | 탈출 행은 모두 이름 저장이지만, 그 주소(이름 포인터의 값)의 쓰임 20/20/21 가운데 14/14/15 가 풀리지 않는다: `strncpy`·`memcpy` 의 인자 1 에 "쓰지 않는다"는 항목이 없음 12/12/13, `OS_strnlen` 을 거쳐 `memchr` 1, 가변 인자 1. 풀린 것 6/6/6: 시험 4, `strcmp` 의 `out-behaviour doc-collated [declared pure …]` 1, 본문을 따라가 쓰기가 없음 1 | not discharged 16/16/17/0/0 |
| 형 단위(다른 필드의 적중 + 형 없는 쓰기) | **풀리지 않는다** | 이 사실은 형 없는 쓰기가 그 필드에 닿지 않음을 보이지 않는다 | not discharged 4/4/4/0/0; 다른 필드의 적중이 없어 `untyped-in-program` 이 원인이 아닌 것 12/12/13/0/0 |
| `writer-function-not-once`, `several-instances`, `writer-in-loop` | **원인이 아니다** | 위의 `once` 판정 | not a cause 16/16/16, 16/16/17, 3/3/3 |

- 그러므로 **어느 빌드의 어느 버퍼도 `complete` 가 아니다**(16/16/17/0/0 가운데 0). 줄은 풀리지 않은 깃발을 이름으로 적는다. 아무것도 풀지 않았다.
- FIELD_STATUS 자체는 바꾸지 않았다. 배열 표시를 `--object-access` 아래에서 빼는 것은 추출의 변경이고, FIELD_STATUS 를 읽는 다른 규칙(LOCK_IDS 의 정적 인스턴스, AWAIT_L1, 경로 C, 이미지 키, holder 줄, 필드 저장값 규칙의 (6))이 모두 움직인다. 그 크기: c1_pre_tx 에서 그 표시가 있는 행 185 가운데 그 필드의 적중이 없는 것(필드 단위로 움직일 것) 33, 그 가운데 형 단위의 원인도 없는 것 27(`scratch4/array_flag_only_c1_pre_tx.txt`). 버퍼 필드는 모두 주소 탈출이 남으므로 그 변경으로도 complete 가 되지 않는다. 할지는 lead 의 판단을 기다린다(18.8).
- 덧붙임대로, 원인을 모두 풀어도 이 개정은 FIELD_STATUS 를 넘어 `complete` 라고 쓰지 않는다. 그래서 문자 배열 버퍼는 이 개정에서 `complete` 가 될 수 없다(배열 표시가 늘 남는다). 줄이 서는 꼴은 원인과 address use 마다의 discharged 다.

### 18.3 scratch 점검 [I]
scratch Release 빌드는 kz-static, static_golden, kz_lint, test_static_reasons 이고, 모두 감시 아래에서 돌렸다(`scratch_guard4/v01`–`v19`, 31 기록, 모두 `verdict=completed`). 다섯 빌드는 scratch 도구로 `run.sh extract_final` 한 번(`v03`, rc 0)으로 다시 뽑았다.

| 점검 | 결과 | 출력 |
|---|---|---|
| 다시 뽑은 facts | 다섯 빌드 모두 셋째 실행의 scratch facts 와 같은 sha256 | `scratch4/facts_same_as_scratch3.txt` |
| relations 글(다섯 빌드) | `rf2_final_order` 와 비교해 없어진 줄은 `facts` 줄 하나뿐. 더한 줄 244/243/149/297/297 가운데 이 단계의 줄: writer 묶음 43/43/28/96/96, address uses 1/1/1/0/0, 버퍼 16/16/17/0/0, 수명 끝 17/17/13/41/41, 머리 1. 나머지 166/165/89/159/159 는 셋째 실행까지의 줄(그때의 수와 같다) | `scratch4/relations_text_diff.txt` |
| golden·fixture 컴파일·lint 69 | 실패 5: order golden 셋, `resultflow_np`, `closure_np`(저장소에 golden 없음). lint 8 통과. scratch golden 다섯은 scratch 자료 디렉터리에서 모두 `matches`; order 셋과 `resultflow_np` 는 앞 실행의 scratch golden 과 같은 바이트 | `scratch4/logs/ctest_golden_lint_4d.log`, `golden_scratch_check_4c.txt`, `scratch4/golden_scratch/same_as_earlier.txt` |
| test_static_reasons(새 golden 을 놓은 scratch 자료 디렉터리) | 793 확인 통과 | `scratch4/logs/reasons_4c.log` |
| 두 C1 빌드의 새 줄 | 빌드 경로를 가리면 같다(같은 수, 같은 글) | `scratch4/buffer/lines_c1_pre_tx.txt`, `lines_c1_post_tx.txt` |

### 18.4 새 줄이 서는 곳과 서지 않는 곳 [I]
수는 `scratch4/buffer/counts_all.txt`(차례 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642/fixture)다. 설계 행에만 보이는 줄은 없다: 줄은 모든 이름 공간의 모든 writer 와 이름 저장에 대해 나온다.

| 줄 | 서는 것 | 서지 않는 것 |
|---|---|---|
| 키 writer 의 묶음 | partition 하나 11/11/12/17/17/2, 여럿이 함께 씀 2/2/2/0/0/0 | 키 매개변수 함수가 닿지 않음 30/30/14/79/79/2 |
| 버퍼 | complete 0/0/0/0/0/0 | not complete 16/16/17/0/0/2: FIELD_STATUS(풀리지 않은 원인) 11/11/12/0/0/1, 다른 writer 5/5/5/0/0/1; 정하지 못함 0 |
| address uses | discharged 6/6/6/0/0/4(시험 4/4/4/0/0/2, 항목 있는 library 1/1/1/0/0/0, 본문 따라감 1/1/1/0/0/2) | not discharged 14/14/15/0/0/3(항목 없는 library 12/12/13/0/0/1, library 로 넘김 1/1/1/0/0/0, 가변·간접 1/1/1/0/0/0, 본문이 그것을 통해 씀 0/0/0/0/0/1, 베낌 0/0/0/0/0/1) |
| clear-slot 의 빈 레코드 조건 | shown 0/0/0/0/0/1 | not shown 10/10/6/18/18/1 |
| 그 밖의 수명 끝 | clear-table 5/5/6/0/0/1, release 1/1/0/23/23/1, 매개변수를 거친 release 1/1/1/0/0/1(호출자: 생성 쪽 1/1/1/0/0/2, 삭제 쪽 1/1/1/0/0/1, 널 포인터 1/1/1/0/0/1) | — |

- c5 두 빌드(cFE 6.4 의 OSAL)에는 이름 포인터 키가 없으므로 이름 저장·버퍼·address uses 줄이 없다. 그 빌드의 수명 끝은 상수 release 23 과 clear-slot 18 이다. release 의 일부는 생성 함수 안의 것이다(예: `OS_TaskCreate` 안의 `OS_task_record_t.free := 1`, `lines_c5_641.txt`). 이 개정은 상수 release 가 실패한 생성의 되돌림인지 삭제인지 가르지 않는다(18.7).

### 18.5 C1 에서 줄이 보인 것 [I]
- **설계 행의 종류(partition `{3}`, 셈 세마포어)의 버퍼 줄**(`lines_c1_pre_tx.txt`): `OS_count_sem_internal_record_t.obj_name extent 20`, 이름 저장 하나(`osapi-countsem.c:100:9`), writer 셋 = 표 초기화의 채움(`:73:5`), 생성의 채움과 생성의 복사(`:100:9`). 다른 writer 는 없다. 원인: 배열 표시는 풀림, 주소 탈출 1 은 이름 저장이지만 address uses 20 가운데 14 가 풀리지 않아 풀리지 않음, 나머지는 원인이 아님. 줄은 `not complete (FIELD_STATUS disabled; causes not discharged: address-escape)` 다. 탐침 P3 의 읽기("writer 는 생성의 복사와 지우기뿐, FIELD_STATUS 는 주소 탈출로 DISABLED")를 도구 줄이 다시 낸다. 다른 점은 도구가 배열 표시까지 원인으로 따로 적는다는 것이다(탐침은 주소 탈출만 적었다; 배열 표시만으로도 두 규칙이 DISABLED 다, `lockset.cpp:594-597`).
- **이름 공간 `OS_common_record_t.name_entry` 의 수명 끝**:
  - `clear-table osapi-idmap.c:120:5 in OS_ObjectIdInit`: id 를 이름 대지 않고 `OS_common_table` 의 모든 레코드를 끝낸다.
  - `clear-slot osapi-idmap.c:640:9 in OS_ObjectIdFindNextFree`: 빈 레코드 조건이 보이지 않는다. 그 저장의 guard `(eq local:return_code 0)` 는 활성 필드를 시험하지 않는다. 같은 함수가 같은 guard 로 `active_id := token->obj_id`(`:639:9`)를 쓰므로 그 레코드는 여기서 할당된다. 이것이 §12.11 (사)의 "자리를 비우며 하는 할당"을 도구 줄로 보인 것이다. 계획 규칙 10.4 에 따라 이 slot clear 는 계획에서 수명 끝 site 로 남는다.
  - `release through parameter 1 of OS_ObjectIdTransactionFinish at osapi-idmap.c:1079:9`: `active_id := *final_id`. 호출자 셋은 `OS_ObjectIdFinalizeNew`(`:849:5`, `&final_id`, 생성 쪽: creator 함수 열둘이 바로 부름), `OS_ObjectIdFinalizeDelete`(`:887:5`, `&final_id`, 삭제 쪽), `OS_ObjectIdRelease`(`:1115:5`, `NULL`: 저장의 guard 가 널이 아니기를 요구) 다. 넘긴 지역 변수의 값은 묶이지 않는다(lead 의 판단 (b)의 틈과 같다). 그래서 이 줄은 어느 id 가 끝나는지 말하지 않는다.
- 키 writer 의 묶음: `osapi-idmap.c:640:9` 의 지우기는 partition 열하나가 함께 쓰는 묶음, `OS_ModuleLoad` 의 이름 저장은 `{10} {11}` 둘이 함께 쓰는 묶음이다. 생성자 아홉의 이름 저장은 각각 partition 하나다.

### 18.6 공간 [M]
- 시작: 1,216,319,488 B(`free_start_attempt4.txt`, 03:49Z).
- 감시 기록의 가장 낮은 값: 873,267,200 B(`scratch_guard4/v19`, ctest). 하한 314,572,800 B 를 넘지 않았다.
- 크기: scratch 빌드 tree 207 MB, 다시 뽑은 다섯 빌드 70 MB, 보기·relations·fixture 49 MB.
- scratch 를 모두 지운 뒤: 1,212,977,152 B(`free_end_attempt4.txt`, 04:15Z). 기록 디렉터리에 더한 것은 약 0.9 MB 다. 압축 대체는 쓰지 않았다.

### 18.7 계획과 다르게 한 것
- **덧붙임 고침 둘.** 생성 쪽과 삭제 쪽의 가름을 scratch 판과 fixture 를 보고 두 번 좁혔다(18.1). 둘 다 코드 커밋 전에 커밋했다.
- **이름 저장은 포인터 키에서만.** 첫 판은 키가 배열 필드인 이름 공간(`OS_stream_internal_record_t.stream_name`, `OS_filesys_internal_record_t.virtual_mountpt`)에서도 같은 자리의 decay 를 이름 저장으로 셌다(버퍼 19/19/20). 5 항의 "이름 포인터 필드"대로 키가 FIELD_TYPES 의 포인터 필드이고 쓰기가 대입일 때만 센다. 그 판의 수는 `scratch4/buffer_trial_before_pointer_key/` 에 남겼다.
- **layers 칸.** 첫 판은 새 줄이 이름 대는 함수를 행의 layers 에 더해 c1·r2 의 `namespace layers:` 줄 둘이 바뀌었다(덧붙임은 더하지 않는다고 적음). `NoNotes` 로 고쳤다. 그 판의 diff 파일은 scratch 에서 덮어써 남지 않았다. 수는 위 글이 전부다. 세기 스크립트의 첫 판(깃발 판정의 정규식 결함)의 출력도 기록 디렉터리에서 덮어썼다. "시도 출력은 새 이름으로" 를 지키지 못한 곳이다.
- **fixture 가 보이지 못하는 꼴.** 문자 배열 버퍼는 배열 표시가 늘 남으므로 `complete` 와 `every cause discharged` 는 fixture 에도 다섯 빌드에도 없다. fixture 는 `--semantics` 없이 뽑으므로 검색의 비교 함수 인자도 풀리지 않는다.
- **쓰임으로 읽지 않는 것.** 이름 포인터가 레코드 통째 복사로 다른 곳에 가는 길은 READS 행이 없어 address uses 에 없다. `release` 는 상수 빈 값을 쓰는 것을 모두 같은 꼴로 적고, 생성 함수 안의 되돌림(c5 의 `OS_TaskCreate`)과 삭제를 가르지 않는다. 생성 쪽 호출(`rollback`)은 성공한 생성의 마무리이기도 하다(계획 규칙 10.4 의 "status 인자가 성공 값이 아닐 때 되돌림"과 같은 site).
- 열린 효과의 `other` 종류와 둘째 주소 넘김 호출 자리(lead 의 판단 (e))는 이 단계에서 보지 않았다.

### 18.8 열린 것과 다음 차례
- **lead 의 판단을 기다리는 것**: FIELD_STATUS 의 배열 표시를 `--object-access` 아래에서 뺄지(추출의 변경; c1_pre_tx 에서 33 행이 필드 단위로, 27 행이 형 단위로도 움직임 [I]). 빼더라도 버퍼 줄은 주소 탈출 때문에 complete 가 되지 않는다.
- **PI 결정이 필요한 것**: 버퍼의 주소 탈출을 풀려면 `strncpy`·`memcpy` 의 원천 인자를 쓰지 않는다는 C 라이브러리 항목이 필요하다. 지금 그 항목은 없거나 `unverified` 다. §12.10 5 항의 C 라이브러리 항목 결정에 든다(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)). 결정 전: 미대조 그대로.
- lead 의 판단 (a): 평가 갈래 Re 가 17 번 조건 (6)을 덮는지는 설명 스크립트 단계가 평가 파일에서 본다. 덮지 않으면 PI 의 열린 것이다.
- lead 의 판단 (b): out 저장된 지역 변수의 추출 틈은 그대로다. 매개변수를 거친 release 줄도 그 틈 때문에 끝나는 id 를 말하지 않는다.
- lead 의 판단 (e): 빌드 단계의 측정으로 미룬다.
- 첫 ctest 실패(지금 소스의 scratch): `static_golden_order_np`, `static_golden_resultflow_np`, `static_golden_resultflow_order_np`, `static_golden_looplock_order_np`, `static_golden_closure_np`. 커밋할 golden 은 `scratch/golden_scratch/` 의 order 셋, `scratch3/golden_scratch/resultflow_np.golden`, `scratch4/golden_scratch/closure_np.golden_4` 다. 남은 내용을 넣으면 다시 정한다(lead 의 판단 (d)).
- §17.5 의 다음 차례 가운데 남은 것:
  - (5) 계획 규칙 개정 5 생성기: `framework_namespaces2_*` 를 읽는 쪽, `namespace-entry`·`namespace-layer` 줄, 생성 쪽 종류 값 site, 문맥 틀 site, 수명 끝 site(이 줄의 `clear-slot … not shown` 을 10.4 가 읽음), 라벨 둘. 선택 규칙과 층 라벨은 그대로.
  - (6) 개정 이름 바꾸기(relations 14, order 8, plan 5).
  - (7) `source_tree`·`absence origins`·(5)의 fixture.
  - (8) 완전한 소스의 scratch 점검 빌드 → 선언 → 사본 → MC 빌드 → 다시 뽑기 → 끝 점검 → 결과 절.
- §15.7 의 key-clearing 이름 문제(할당 때의 자리 비우기를 지우기로 셈)는 위 clear-slot 줄이 그 꼴을 보이므로, 계획 규칙 개정 5 의 수명 끝 site 와 함께 다룰 수 있다. 다루는 방법은 lead 의 판단을 기다린다. absence origins 의 이름 칸 문제와 §12.10 5 항의 PI 결정은 그대로 남아 있다.

## 19. C1 닫기의 kz-static 한 빌드: 소스 진행분 다섯째, 계획 규칙 개정 5 (2026-11-19, 선언 전에 멈춤)

§18.8 의 다음 차례 (5)(계획 규칙 개정 5 생성기)를 글 → 코드 → scratch 점검의 차례로 넣었다. **선언, 도구 사본, MC 빌드, 다섯 빌드의 선언된 다시 뽑기는 하지 않았다**: 넣을 것이 아직 다 들어가지 않았으므로(§18.8 의 (6)–(8)) §12.10 6 항대로 선언 전에 멈췄다. MC 의 kz-static 은 `bf2ec2ee…` 그대로다(시작과 끝에 sha256 을 보았다, `../records/closure-kzstatic-20261119/scratch5/mc_tool_sha256_start.txt`·`_end.txt`). 소스가 그 바이너리보다 더 앞서 있다. 기록: `../records/closure-kzstatic-20261119/`(`scratch5/`, `scratch_guard5/`, `scripts/plan_one_ns.sh`·`plan_rev5_compare.py`·`plan_rev5_counts.py`, README). cFS 를 부팅하지 않았고 push 하지 않았다. 아래 수는 모두 기록 디렉터리의 출력 파일에서 왔고, scratch 도구(`scratch5/scratch_tool_sha256.txt`)의 수라서 [I] 다.

### 19.0 lead 의 판단 (2026-11-19, §17 의 열린 것에 대해)
lead 가 2026-11-19 에 정한 (a)–(e)는 §18.0 에 적은 그대로다. 이 단계에 닿는 것만 적는다.
- (a) 17 번 조건 (6)은 쓰인 대로(필드 단위와 형 단위 모두) 적용한다. 계획 생성기는 이 조건을 읽지 않는다. Re 가 그것을 덮는지는 설명 스크립트 단계의 일이다.
- (b) out 저장된 지역 변수의 추출 틈은 그대로 두고 `include/` 는 바꾸지 않는다. 이 단계도 `include/` 와 추출을 바꾸지 않았다. 되돌림 site 의 after-call 은 out 인자를 런타임에 기록하므로(10.4) 정적 틈을 정적으로 메우지 않는다.
- (c) 저장된 이름 버퍼는 §18.2 대로이고 이 단계에서 풀지 않았다.
- (d) 바뀌는 golden 은 선언된 빌드에서 커밋한다. 이 단계에서 둘이 더해진다(19.3): `planv4_np.plan.golden`(plan id 를 가림)과 새 `planv5_np.plan.golden`. 첫 ctest 실패 목록은 **완전한 소스**의 scratch 점검 빌드에서 다시 이름으로 확인한다.
- (e) 둘째 주소 넘김 호출 자리와 `other` 종류는 빌드 단계의 측정으로 미룬다.

### 19.1 바뀐 것 [F]
- **글(코드보다 먼저)**: `kairos/docs/plan_manifest.md` 10.8 (`3f82845`)과 그 다듬기 하나(`fcf34ab`). 생성기를 고치며 개정 2 자료와 relations 개정 14 의 줄을 읽고, 10.1–10.6 이 열어 둔 선택을 정했다: 새 줄을 쓰는 조건(모든 `--namespaces` 파일이 `semantics-data 2` 일 때; 두 개정을 섞으면 거절), 읽는 쪽이 받는 문장, 두 세대(`kind`/`table`) 항목의 칸, 사실에서 읽는 칸(FIELD_TYPES 의 저장 이름과 extent, 비교·복사·길이 호출; 길이 호출은 PARAM_FLOW 의 모든 깊이), `success-path` 칸, 구성 요소가 적용되는 조건, sidecar 의 자리, kind 값의 층 호출과 그 두 not-planned 사유, 되돌림·채움·slot clear 의 site, lifetime 줄의 `sites=`, `lifetime-complete` 가 yes 인 조건, 새 need, bound 줄. **고침 하나(10.3)**: 문맥 틀은 at-call 과 after-call 두 site 이므로 `frame=<at-call id>,<after-call id>` 로 쓰고, 문맥은 서로 다른 사슬마다 하나다. **다듬기(`fcf34ab`)**: 저장 이름이 둘이고 extent 가 다른 항목(osal-7.0.1 의 파일 시스템 종류: 64 와 32, `kairos/semantics/framework_namespaces2_osal-7.0.1.txt:152-153`)은 가장 **큰** extent 로 key text 를 묶는다(작은 쪽으로 묶으면 큰 필드에 맞는 이름을 자른다). scratch 계획을 본 뒤, 코드 커밋 전에 했다.
- **소스(`5a47dcc`)**: `lib/llvm/plan_gen.cpp`·`plan_gen.hpp`.
  - 읽는 쪽: `layer-fn`, `layer-op`, `kind … value= at=`, `rollback`, `stored-name`, `extent`, 두 `compare` 꼴을 읽고 `kind-unresolved`·`not-covered` 는 읽고 쓰지 않는다. 개정 1 파일의 이 문장은 전처럼 거절한다.
  - 계획된 경로 A 행(층 또는 대조)의 use 나 creator callee 가 적용되는 항목의 lookup·create 이면: `namespace-entry` 줄, `kind-value` site(lookup 과 create 의 층 호출에서 `kind-param` 인자), `object-rollback` site(create-finalize 호출의 at-call 의 status 인자와 after-call 의 out 인자, `reentry`), `object-clear` site(clear-table 의 채움 호출, relations 가 빈 레코드 조건을 보이지 않는 clear-slot 의 저장; 보이면 `not-a-lifetime-end(…)`), `sites=` 가 붙은 `lifetime` 줄, 종류마다 `lifetime-complete`, 저장 이름 extent 로 묶은 key text(10.5).
  - 적용되는 파일마다 `namespace-layer` 줄. 사슬이 둘 이상인 계획된 행의 문맥마다 틀 site 와 `context` 줄(10.3).
  - `plan_reasons()` 에 둘: `plan_missing(no call site tells the context apart)`, `plan_missing(no lifetime-end entry in the namespace data)`.
  - 바꾸지 않은 것: 선택(`observation-set`), 우선순위, 예산, 대조 선택, 층 라벨, 행 줄(라벨 둘은 개정 4 부터 있음), `rules` 줄과 plan id 의 글(규칙 개정 이름 바꾸기는 §18.8 의 (6)), `include/`, 추출, relations, order.
- **fixture 와 시험(`5a47dcc`)**: `tests/static/fixtures/planv5.c`(종류 매개변수가 있는 조회·할당, 실패한 생성을 되돌리는 마무리, 삭제 마무리, 표 채움, 빈 레코드 조건이 있는 slot clear 와 없는 slot clear, 삭제가 없는 종류, strncmp 로 찾는 표, 호출 문맥이 둘인 초기화 함수), 손으로 쓴 개정 2 자료 둘(`tests/static/data/planv5_fixture/`), `test_static_plan.cpp` 의 개정 5 블록, `tests/static/CMakeLists.txt`(fixture 컴파일 목록과 설명). 계획 golden 의 가림에 plan id 를 더했다(19.3).

### 19.2 scratch 점검 [I]
scratch Release 빌드(kz-static, static_golden, kz_lint, test_static_reasons, test_static_plan)는 모두 감시 아래에서 돌렸다(`scratch_guard5/w01`–`w25`, 모두 `verdict=completed`). 다섯 빌드는 이 실행의 scratch 도구로 `run.sh extract_final` 한 번(`w03`, rc 0)으로 다시 뽑았다(`scratch5/facts_scratch_sha256.txt`; sha256 은 넷째 실행의 것과 다르다. facts 는 뽑은 도구의 정체성을 싣는다).

| 점검 | 결과 | 출력 |
|---|---|---|
| 개정 1 자료, 남긴 rf2 사실, 다섯 빌드 | sidecar 는 `generator` 줄 말고, manifest 는 sidecar sha256 말고, 요청·모듈 표·실행 파일 목록은 첫 줄 말고 `rf2_plans_v4` 와 같다(5/5) | `scratch5/plans/rev1_keptfacts_vs_kept_v4.txt` |
| 선택·층·위치·라벨·대조(개정 2 자료) | 남긴 사실에서도 다시 뽑은 사실에서도 다섯 빌드 모두 같다: 층 계획 행 9/8/3/8/7, 대조 계획 행 5/5/4/2/2, 층 같음(`equal=yes` 30 개, `no` 0 개). 행 줄은 c1 두 빌드의 다시 뽑은 사실에서 키 셋(A20, A27, A30)만 움직인다. 소스 트리 토큰 키 때문이고(§15.3), 같은 사실의 개정 1 계획에서도 같다 | `scratch5/plans/rev2_keptfacts_vs_kept_v4_<b>.txt`, `rev2_scratchfacts_vs_kept_v4_<b>.txt`, `rev1_scratchfacts_vs_kept_v4_<b>.txt` |
| 같은 사실에서 개정 5 가 더한 것 | 없어진 site 0. 더한 site 14/11/14/0/0. 있던 site 가운데 규칙이 더해진 것 2/1/0/0/0(조회의 층 호출에 `kind-value`, 문맥 틀이 된 `CF_Init` 의 after-call), 값이 바뀐 것 1/1/0/0/0(조회 key text 의 bound 64 → 20). 새 need 14/12/12/0/0. 바뀐 need 2/2/1/0/0(시간 기준 종류의 `object-delete` 가 `no namespace data entry` 에서 planned 로: 개정 1 의 osal-f34759d·osal-7.0.1 파일에 그 종류가 없었다). 요청은 site 번호를 키로 바꾸면 같다. 나머지 sidecar 줄은 같다 | `scratch5/plans/rev2_vs_rev1_scratchfacts_<b>.txt` |
| golden·fixture 컴파일·lint·reasons·plan 72 | 실패 6: order golden 셋, `resultflow_np`, `closure_np`(§18.8 과 같음), `test_static_plan`(검사 389 가운데 2: planv4 의 plan id 를 가린 golden, 새 planv5 golden; 둘 다 저장소에 아직 없다). lint 통과, test_static_reasons 통과 | `scratch5/logs/ctest5d.log` |
| scratch golden 둘을 놓은 scratch 자료 디렉터리 | test_static_plan 389 확인 통과 | `scratch5/golden_scratch/test_static_plan_scratch_data.log` |

### 19.3 golden 의 plan id [I]
- `planv4_np.plan.golden` 은 `plan`·`plan-id` 의 16 hex 가 다르면 실패한다. plan id 는 facts 의 artifact hash(golden 에서 이미 가림)와 규칙 글의 sha256 이고, facts 는 뽑은 도구의 정체성을 싣는다. 그래서 도구 소스가 바뀔 때마다 이 golden 이 실패한다. 저장소 golden 의 `1362f7e3…` 이 이 실행의 scratch 빌드에서 `86afd55f…` 가 되었다(`scratch5/golden_scratch/planv4_np.plan.golden.diff_vs_repo.txt`; 이 실행의 앞 scratch 빌드에서도 다른 값이었으나 그 출력은 남지 않았다). 앞 실행들의 남은 scratch ctest 기록에는 test_static_plan 이 없다(`scratch*/logs/`). 그래서 그 실행들은 이것을 보지 못했다 [I].
- 시험의 가림에 plan id 를 더했다. 그래서 planv4 golden 은 두 줄(`<plan-id>`)만 바뀐다(`planv4_np.plan.golden_2.diff_vs_repo.txt`). 선언된 빌드는 `golden_2` 둘을 커밋한다. 그 뒤로는 도구가 바뀌어도 이 두 golden 이 plan id 때문에 실패하지 않는다.

### 19.4 다섯 빌드에서 새 줄이 서는 곳과 서지 않는 곳 [I]
수는 `scratch5/plans/counts_rev2_scratchfacts.txt`(다시 뽑은 사실, 차례 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642)다. 줄은 계획된 모든 경로 A 행과 적용되는 모든 자료 파일에서 나온다. 설계 행에만 보이는 줄은 없다.

| 줄·site | 서는 것 | 서지 않는 것 |
|---|---|---|
| `namespace-entry` | 계획된 경로 A 행 5/5/4/2/2 가운데 4/4/4/0/0(c1: 셈 세마포어 A20·A21, 시간 기준 A22·A26; r2: 파일 시스템 셋, 시간 기준 하나) | 1/1/0/2/2: c1 의 A63(lookup·creator `OS_FileOpenCheck`·`OS_mv` 가 이 빌드에 적용되는 항목에 없음), c5 의 A16·A17(lookup·creator `CFE_ES_DeleteCDS`·`CFE_ES_RegisterCDSEx` 가 cFE 6.4 자료의 항목에 없음; 이유는 sidecar 의 need 줄 글) |
| `namespace-layer` | 1/1/1/2/2 | — |
| `kind-value` | need planned 4/4/4/0/0 | — |
| `object-rollback` | planned 4/4/4/0/0; lifetime 줄 2/2/3/0/0, 모두 `reentry=none` | — |
| `object-clear` | planned 4/4/4/0/0; clear-table 과 clear-slot 각 2/2/2/0/0 줄 | slot clear 가 `not-a-lifetime-end` 인 것 0(다섯 빌드의 clear-slot 줄은 모두 `not shown`, §18.5) |
| `lifetime-complete` | yes 2/2/2/0/0(종류마다) | no 0 |
| `context` 와 틀 | 사슬이 둘인 계획된 행 1/0/0/0/0(c1_pre_tx 의 A21), 틀 2/0/0/0/0 | 틀 없는 문맥 0 |
| extent 로 묶은 key text | 1/1/0/0/0 | r2 의 파일 시스템 종류는 가장 큰 extent 가 64 라 `KZ_RT_STR_MAX` 보다 작지 않다 |

- c5 두 빌드(cFE 6.4.x·OSAL 4.1.1, 세대 T)는 `namespace-layer` 줄 둘(칸이 모두 `-`) 말고 개정 5 가 더한 것이 없다.
- fixture(`scratch5/fixture/planv5_plan.side`)는 다섯 빌드에 없는 꼴을 보인다: `lifetime-complete … no(no delete statement)`, 표 항목의 `no(no delete statement; no rollback statement; no layer-op statement)`, 빈 레코드 조건이 보이는 slot clear 의 `not-a-lifetime-end(…)`, 표 항목의 `kind-value … plan_missing(no rule of this revision)`. 틀 없는 문맥(`plan_missing(no call site tells the context apart)`)은 fixture 에도 다섯 빌드에도 없다.

### 19.5 C1 에서 줄이 보인 것 [I]
- **설계 행 A21**(`plans_new_lines/rev2_scratchfacts_c1_pre_tx.txt`): `kind=OS_OBJECT_TYPE_OS_COUNTSEM value=3`, `stored-name=OS_count_sem_internal_record_t.obj_name extent-value=20`, `compare-call=strcmp@…/osapi-idmap.c:271`, `copy-call=memset@…/osapi-countsem.c:100,strncpy@…:100`(이름 저장 매크로가 한 줄에서 펼쳐짐), `length-call=memchr@…/osapi-countsem.c:91,memchr@…/osapi-idmap.c:973`, `success-path=OS_CountSemCreate:missing`. 조회 key text 의 bound 는 64 에서 20 이 된다. 문맥 둘: 1 은 `CF_Init` 의 `CF_CFDP_InitEngine` 호출(`cf_app.c:252:18`), 2 는 `CF_CmdEnableEngine` 의 호출(`cf_cmd.c:1119:13`)이 틀이다. 되돌림 site 는 `OS_CountSemCreate` 의 `OS_ObjectIdFinalizeNew`(`osapi-countsem.c:106:23`, status arg0, out arg2), 수명 끝은 삭제(`OS_CountSemDelete`), 표 초기화(`osapi-idmap.c:120`의 memset), slot clear(`osapi-idmap.c:640`)다.
- §15.7 의 key-clearing 이름 문제: `OS_ObjectIdFindNextFree` 의 slot clear 는 relations 가 빈 레코드 조건을 보이지 않으므로(§18.5) 10.4 대로 `object-clear` site 로 계획된다. 10.4 는 그 사건이 id 를 이름 대지 않고 그것이 든 생성의 실행에 속한다고 읽는다. 다루는 다른 방법은 lead 의 판단을 기다린다.

### 19.6 공간 [M]
- 시작: 1,209,716,736 B(`free_start_attempt5.txt`, 04:24Z).
- 감시 기록의 가장 낮은 값: 810,037,248 B. 하한 314,572,800 B 를 넘지 않았다.
- 크기: scratch 빌드 tree 289 MB, 다시 뽑은 다섯 빌드 70 MB, 계획·fixture 21 MB.
- scratch 를 모두 지운 뒤: 1,205,841,920 B(`free_end_attempt5.txt`, 04:50Z). 기록 디렉터리에 더한 것은 약 0.7 MB 다. 압축 대체는 쓰지 않았다.

### 19.7 계획과 다르게 한 것
- **글 다듬기 하나.** extent 를 가장 큰 것으로 하는 다듬기는 다섯 빌드의 scratch 계획을 본 뒤에 했다(코드 커밋 전, 19.1). 그 전 도구의 출력은 `scratch5/plans_trial1_before_largest_extent/` 에 남겼다. 다른 것은 r2 의 줄뿐이다: 묶은 key text 2 → 0, 값이 바뀐 site 2 → 0, `bound` 줄 하나가 없어짐.
- **fixture 를 고쳤다.** 첫 판은 층 조회 함수 안에 길이 검사의 이른 반환이 있어 relations 가 부재 값을 정하지 않았고(`absence: unevaluated`), 그래서 문맥 줄이 없었다. 길이 검사를 API 쪽으로 옮겼다(세마포어는 도우미 함수 안: 길이 호출을 깊이 1 에서 찾음; 큐는 API 본문: 깊이 0). 그 시도의 출력은 scratch 에만 있었고 남지 않았다. `scratch5/fixture/planv5_relations_trial.txt` 는 고친 fixture 에 대한 앞 scratch 도구의 출력이다.
- **시도 계획.** 마지막 도구 앞의 두 도구로 만든 계획이 있었다: 첫 판(표 항목에 층 연산을 붙이지 않게 고치기 전)과 둘째 판(extent 다듬기 전). 첫 판의 출력은 scratch 에만 있었고 남지 않았다. 그래서 이 절은 그 판의 수를 쓰지 않는다. 둘째 판은 위 trial 디렉터리다.
- **kind 값의 역할 이름.** 층 호출의 kind 인자가 다른 행의 key 로 이미 기록된 site(fixture 의 kind 필드 이름 공간)에서는 값의 역할이 `key` 로 남는다. 같은 인자가 기록되므로 바꾸지 않았다.
- **golden 가림.** test_static_plan 의 가림에 plan id 를 더했다(19.3). 시험의 변경이고, planv4 golden 이 두 줄 바뀐다.
- `scratch_guard5/` 에 처음 함께 복사한 ctest 기록 넷은 `scratch5/logs/` 와 같은 바이트라서 README 커밋에서 지웠다.

### 19.8 열린 것과 다음 차례
- **자료 파일의 머리 주석.** `framework_namespaces2_*.txt` 다섯의 머리는 "No tool reads this file yet"라고 적는다. 이제 읽는 쪽이 소스에 있다. 이 파일은 훑기가 만들고 계획이 sha256 으로 붙잡으므로 이 단계에서 고치지 않았다. 고치려면 훑기(`env/tools/semantics_scan.py`)의 머리 글을 바꾸고 다시 만드는 자료 단계가 필요하다.
- 틀 없는 문맥의 꼴은 fixture 와 다섯 빌드 어디에도 없어 시험되지 않았다.
- 첫 ctest 실패(지금 소스의 scratch): `static_golden_order_np`, `static_golden_resultflow_np`, `static_golden_resultflow_order_np`, `static_golden_looplock_order_np`, `static_golden_closure_np`, `test_static_plan`. 커밋할 golden 은 §18.8 의 다섯에 `scratch5/golden_scratch/planv4_np.plan.golden_2`, `planv5_np.plan.golden_2` 를 더한 일곱이다. 남은 내용을 넣으면 다시 정한다(lead 의 판단 (d)).
- §18.8 의 다음 차례 가운데 남은 것:
  - (6) 규칙 개정 이름 바꾸기(relations 14, order 8, plan 5). 계획의 `rules` 줄과 plan id 의 글도 이때 바뀐다(10.8).
  - (7) `source_tree`·`absence origins` 의 fixture. 개정 5 의 fixture 는 이 단계에서 넣었다.
  - (8) 완전한 소스의 scratch 점검 빌드 → 선언 → 사본 → MC 빌드 → 다시 뽑기 → relations, 쌍 파일을 낸 order, 새 자료 파일 위의 개정 4·5 계획 → 끝 점검 → 결과 절.
- §18.8 의 lead 판단(FIELD_STATUS 의 배열 표시), PI 결정(C 라이브러리 항목; PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)), lead 의 판단 (a)·(b)·(e)의 열린 것, absence origins 의 이름 칸 문제는 그대로다.

## 20. C1 닫기의 kz-static 한 빌드: 개정 이름, 남은 fixture, 완전한 소스의 scratch 점검 빌드 (2026-11-19, 선언 전에 멈춤)

§19.8 의 다음 차례 (6)(규칙 개정 이름), (7)(`source_tree`·`absence origins` 의 fixture)과 (8)의 앞 절반(완전한 소스의 scratch 점검 빌드, 다섯 빌드의 scratch 다시 뽑기, 선언이 쓸 값)을 했다. **선언, 도구 사본, MC 빌드는 하지 않았다**: 이 단계의 끝 조건이 선언 전이다. MC 의 kz-static 은 `bf2ec2ee…` 그대로다(시작과 끝에 sha256 을 보았다, `../records/closure-kzstatic-20261119/scratch6/mc_tool_sha256_start.txt`·`_end.txt`). 기록: `../records/closure-kzstatic-20261119/`(`scratch6/`, `scratch_guard6/`, `scripts/scratch6_run.sh`·`scratch6_compare.sh`·`diffkinds.py`·`compare_table.py`·`open_other.py`, README). cFS 를 부팅하지 않았고 push 하지 않았다. 아래 수는 모두 기록 디렉터리의 출력 파일에서 왔다. scratch 도구의 수라서 [I] 다(공간의 수는 [M]).

### 20.0 lead 의 판단 (2026-11-19, §17 의 열린 것에 대해)
lead 가 2026-11-19 에 정한 (a)–(e)는 §18.0 에 적은 그대로다. 이 단계에서 어떻게 지켰는지만 적는다.
- (a) 판정 규칙 8.13 의 17 번 조건 (6)은 쓰인 대로 적용한다(필드 단위와 형 단위 **모두**). 도구는 바꾸지 않았다. 다섯 빌드의 머리 줄은 여전히 정함 0 이고, (5)·(6)이 전제인 덩이가 66 이다(`scratch6/heads/relations_heads_c1_pre_tx.txt`). 평가 갈래 Re 가 이것을 덮는지는 설명 스크립트 단계와 평가 파일의 일이다. 덮지 않으면 PI 의 열린 것으로 적는다. 어떤 글도 넓히지 않았다.
- (b) out 저장된 지역 변수의 추출 틈은 그대로다. `include/` 를 바꾸지 않았다. 성공 경로 줄은 계속 missing 이다(`summary success path: creator callees 14: held 0`, c1_pre_tx). 열린 것이다.
- (c) 저장된 이름 버퍼는 §18.2 에서 깃발마다 정한 대로다. 아무 깃발도 빼지 않았다. 버퍼 줄은 `not complete` 와 풀리지 않은 깃발을 적는다(머리 줄: buffers 16, complete 0).
- (d) 바뀌는 golden 일곱은 선언된 빌드에서 커밋한다. 첫 ctest 실패는 **완전한 소스**의 scratch 점검 빌드에서 이름으로 확인했다(20.2).
- (e) C1 의 둘째 주소 넘김 호출 자리와 열린 까닭의 `other` 종류를 scratch 사실에서 가리고 셌다(20.5). 선언된 빌드의 측정에서 다시 센다.

### 20.1 바뀐 것 [F]
- **글(코드보다 먼저, `680dc66`)**: `kairos/docs/static_facts.md` 5x 덧붙임 아홉째와 `kairos/docs/plan_manifest.md` 10.9. 개정 이름이 나오는 자리를 정한다.
  - relations 의 `rules` 줄은 그대로 `relations rules revision 12 (2026-11-18)` 다. 5x 의 "없는 사실은 앞판의 바이트에 도구 줄만 더함"을 지키려는 것이다. 개정 14 는 개정 11·13 처럼 자기 줄을 둔다: OBJECT_ACCESS 가 있는 사실에서만 `rules with object access: relations rules revision 14 (2026-11-19), …` 한 줄을 relations 머리와 키 목록(`--keys`)에 쓴다. 쌍 파일의 `relations` 줄은 이 줄을 저절로 담는다.
  - order 의 글은 그대로다. 5x 의 "글과 `edges.tsv` 는 도구 줄 말고 바뀌지 않는다"와 golden 시험의 "`--pairs` 가 글을 바꾸지 않는다"를 지키려는 것이다. order rules revision 8 은 쌍 파일의 개정이다. 그래서 쌍 파일의 `order` 줄 맨 앞에 이름을 둔다.
  - 계획: `--manifest 4` 의 `rules` 줄·manifest 머리·plan id 가 `plan rules revision 5 (2026-11-19)` 를 이름 댄다. 이름 공간 파일의 개정과 상관없다(10 절 머리: 개정 1 파일이면 생성기 정체성과 이 이름 말고 개정 4 의 출력). 기본 경로는 `plan rules revision 2` 그대로다.
- **소스(`aa8968f`)**:
  - `lib/llvm/relations.cpp`·`.hpp`: `relations_object_access_revision()`, 머리 줄과 키 목록의 줄.
  - `lib/llvm/order.cpp`·`.hpp`: `order_pairs_revision()`, 쌍 파일의 `order` 줄.
  - `lib/llvm/plan_gen.cpp`: 개정 5 의 이름과 글.
  - `tools/kz-static/main.cpp`·`cmd_plan.cpp`: 도움말과 주석.
  - `tests/static/test_static_plan.cpp`: 개정 5 를 읽는 확인 둘.
  - fixture 둘:
    - `tests/static/fixtures/closure.c` 에 `ent_look` 를 더했다. 널 out 포인터면 스스로 부재 상수를 돌려주고, 아니면 검색의 결과를 돌려주는 감싸개다. 명령 스레드가 쓴다. 이 행의 줄은 `absence origins: -17 [CL_ERR_NOT_FOUND]: 2 origin(s): ent_look at closure.c:225:17 when (eq param:out#0 0) [other]; ent_get at closure.c:218:3 when always [search no-match]` 이다. 기원이 둘인 꼴과 `[other]` 꼴이 fixture 에 처음 들어온다.
    - `tests/static/test_static_cli.cpp` 에 소스 트리 키 블록을 더했다. 루트 밖 머리 파일의 static 키 매개변수 함수를, `.git` 항목이 있는 같은 트리의 두 checkout 에 둔다. 옵션을 켜면 BASIS `source_tree[0] = <checkout>/app <app>` 이 나오고, 키 글 `…|t_find@<app>/src/kt.h#0|1` 은 두 checkout 에서 같다. 옵션이 없으면 절대 경로가 남아 키가 다르다. 개정 14 의 줄은 옵션을 켰을 때만 머리와 키 목록에 있다.
  - `CMakeLists.txt` 의 fixture 설명.
- 바꾸지 않은 것: `include/`, 추출, 선택 규칙, 층 라벨, 저장소의 golden(20.4). 개정 5 와 (3)–(4)의 fixture 는 앞 실행들이 넣었다(§17·§18·§19).

### 20.2 scratch 점검 빌드와 시험 [I]
- **시도 빌드**(커밋 전 소스, `scratch6/trials/`). 감싸개 꼴을 고르기 전에 한 번 시도했다. 검색 함수 안에 이른 부재 반환을 둔 첫 판은 부재 값을 정하지 않았다(`fixture/fx1_peek_in_search/rel.txt`: `absence: unevaluated`). 끝 반환의 조건에 그 앞 조건의 부정이 들어가기 때문으로 보인다 [미확인: 코드에서 그 항까지 따라가지 않음]. 개정 이름을 읽던 `test_static_plan` 의 두 확인도 이 빌드에서 잡았다(`logs/ctest_trial1.log`).
- **점검 빌드**는 커밋 `aa8968f` 의 `kairos/` 를 `git archive` 로 꺼낸 것(681 파일, 작업 트리의 추적 파일과 sha256 이 같음)을 Release 로 빌드했다. 모두 감시 아래에서 돌렸다(`scratch_guard6/c01`–`c18`). scratch kz-static 의 정체성은 `checkbuild/scratch_tool_identity.txt` 에 있다: 파일 `de7fcdd1…`, `tool_sha256=36e02995…`, `tool_source=3b89477d…`, 스키마 12.

| 시험 | 결과 | 출력 |
|---|---|---|
| 첫 ctest(`-V`), static·lint 82 | 실패 6: `static_golden_order_np`, `static_golden_resultflow_np`, `static_golden_resultflow_order_np`, `static_golden_looplock_order_np`, `static_golden_closure_np`, `test_static_plan`(389 가운데 2: planv4·planv5 의 plan golden). Not Run 9: 이 차례에 빌드하지 않은 실행 파일 | `checkbuild/logs/cb_ctest_first_V.log` |
| Not Run 이던 9(세 묶음으로 빌드, 돌리고 지움) | 모두 통과: `test_static_expr` 26, `flow` 36, `recipe` 71, `gitrev` 13, `unroll` 90, `wrappers` 512, `link` 68, `elf32` 233, `test_trace_static_closure` 114 확인 | `cb_ctest_g5.log`–`g7.log` |
| 첫 ctest 의 통과 | `test_static_cli` 1,217, `test_static_reasons` 771 확인; lint selftest 114 경우 실패 0, lint 규칙 일곱 clean | `cb_ctest_first_V.log` |
| 정체성 시험 | `tool_identity_schema_revision`, `source_identity_cmake` 통과 | `cb_ctest_identity.log` |
| scratch 자료 디렉터리의 golden 일곱 | golden·plan·reasons 명령 36 이 모두 rc 0 | `cb_golden_scratchdata.log`, `golden_scratch/` |
| golden 일곱의 바이트 | 시도 빌드의 것과 같다. 앞 scratch golden 과 다른 곳은 다음뿐이다. order 셋: 쌍 파일의 `order` 줄. planv4·planv5: 머리 주석, `obsrules`, `rules` 세 줄. `resultflow_np`: §17 의 것과 같은 바이트. `closure_np`: 새 조회의 줄들 | `trials/golden_scratch/`, `checkbuild/golden_scratch/goldens_sha256.txt` |

점검 빌드 tree 의 271 시험 가운데 위 84(static·lint 82, 그 가운데 Not Run 이던 9 는 빌드한 뒤 다시 돌림; 정체성 2)만 돌렸다. 나머지 187 은 다시 링크되지 않는 바이너리의 시험이다(20.4). 그 시험의 결과는 선언된 빌드의 전체 ctest 가 낸다.

### 20.3 다섯 빌드의 scratch 다시 뽑기와 고친 도구 출력과의 차이 [I]
- 실행: `scripts/scratch6_run.sh`, 단계 열(relcheck, order, extract_off, extract_final, off_order, final_order, plans_t8cf, plans_final, plans_v4, plans_v4_ns2). 모두 rc 0 이다(`scratch6/runs/run_*.txt`). order 단계는 `--pairs` 로 쌍 파일을 쓴다. 다시 뽑기는 `env/redesign/one_build.sh` 다(마지막 옵션 회차의 인자, 기본 경로).
- 비교: `scripts/scratch6_compare.sh` 로 출력 파일마다 고친 도구 `bf2ec2ee` 의 남긴 출력(`/home/user/work/redesign/obs1/rf2_*`)과 비교했다. 보고 485 는 `scratch6/compare_2/` 에, 표는 `compare_2_table.txt` 에 있다. 바이트 점검은 모두 OK 다(BAD 0, `runs/cmp2_*.txt`): 두 실행이 같은 바이트(order·edges·pairs·cdr), 로그가 빔, relations 글이 다시 뽑기의 것과 같음, status PASS, plan.log 가 남긴 것과 같음.
- 출력 종류마다 다른 줄(차례 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642). `facts` 줄과 도구 줄은 꼴로만 가려 셌다(diffkinds 의 가림).

| 출력 | 다른 것 |
|---|---|
| relcheck(T8CF 사실) relations.txt / keys.txt | 도구 줄 +1 / 같음 |
| order(T8CF), off_order: order.txt / edges.tsv / keys.txt | 도구 줄 +1 / 같음 / 같음 |
| final_order: order.txt / keys.txt / edges.tsv·cdr.txt | 도구 줄 +1, 행 키 줄 −3/+3(c1 두 빌드: A20, A27, A30) / 개정 14 줄 +1(다섯 빌드), 키 줄 −3/+3(c1 둘) / 같음 |
| 쌍 파일(새 파일) | 줄 수 27,123/27,123/10,158/5,145/5,134(order·off_order), 27,163/27,163/10,198/5,241/5,230(final_order). 꼬리(final): 쌍 26,572/26,572/9,622/5,160/5,149, no_pair 62/62/21/58/58 |
| extract_off: basis.kz / facts.kz | 머리의 `tool_sha256` / 머리 셋(`basis_sha256`, `parent[0]`, `tool_sha256`)과 BASIS 1 레코드 |
| extract_off: basis.txt / sha256.txt / 보기 / relations.txt / extract.txt·args.txt | 1 줄(basis 해시) / 3 줄(basis, facts, 도구) / `extract.units_from` 1 줄 / 도구 줄 +1 / 같음 |
| extract_final: facts.kz | 머리(c1 넷 `container` 포함, 나머지 셋). BASIS −49/+52(c1 둘: `source_tree` 셋을 더하고 뒤 번호가 밀림), 나머지 −1/+1. RESULT_EFFECT −72/+72(c1 둘: `chain` 의 경로 덩이), STRINGS 크기 |
| extract_final: 보기 | c1 −74/+77(`source_tree` +3, `chain` −72/+72, BASIS 수 줄, `extract.units_from`), 나머지 −1/+1 |
| extract_final: relations.txt | 없어진 줄 0. 더한 줄 244/243/149/297/297: 도구 줄, 개정 14 줄, 머리 셋(성공 경로, 정한 호출, 버퍼·수명), 성공 경로의 이름 공간 머리 13/13/8/31/31, 행의 `callee-check (success path)` 14/14/12/15/15, 그 블록 줄 5/5/1/2/2, `writers [..]` 43/43/28/96/96, `alternatives` 65/64/26/66/66, `lifetime end` 17/17/13/41/41, `holder` 26/26/21/18/18, `absence origins` 39/39/17/23/23, `stored-name address uses` 1/1/1/0/0, `stored-name buffer` 16/16/17/0/0 |
| plans_t8cf·plans_final | 생성기·입력·artifact 줄. final 의 c1 은 행·need 줄 −3/+3(움직인 키) |
| plans_v4(개정 1 이름 공간 자료) | 개정 이름의 셋(머리 주석, `plan` 줄, sidecar `rules`), plan id, 생성기·입력·요청 sha256. c1 둘: 행 3, site 11, need 8, 요청 16, map 1 이 −/+ 같은 수(움직인 키의 글) |
| plans_v4_ns2(개정 2 이름 공간 자료: 새 계획) | sidecar −82/+127, −82/+120, −39/+80, −18/+20, −18/+20. 더한 것: `namespace-entry` 4/4/4/0/0, `namespace-layer` 1/1/1/2/2, `lifetime-complete` 2/2/2/0/0, `context` 2/0/0/0/0, extent `bound` 1/1/0/0/0, site·need·lifetime 줄(표). 요청 줄 −26/+26, −20/+20(site 번호), r2·c5 는 머리만 |

- **키 글**: 두 C1 빌드의 키 글이 367 개 가운데 367 개 같다. 움직인 행은 c1 의 셋뿐이고 공유 키는 0 이다(`scratch6/keys/keys_compare.txt`).
- **쌍 파일**: `pair_check.py` 가 다섯 빌드의 마지막 옵션 사실(22 점검)과 기본 경로 사실(20 점검)에서 모두 PASS 다. 두 C1 계획(개정 1·2 자료)의 계획된 행 13 이 모두 쌍 파일에 줄을 가진다(그 줄 1,125)(`scratch6/pairs/`).
- **개정 5 계획의 수**(`scratch6/plans_v4_ns2_counts.txt`)는 다섯째 실행의 수와 같다. 다른 곳은 디렉터리 머리의 꼴뿐이다(`plans_v4_ns2_counts_vs_scratch5.txt`).

### 20.4 선언이 쓸 값 [I]
- **다시 링크되는 것**(MC tree 의 dry run `make -n all`, 감시 아래 `d01`; `scratch6/dryrun/`): link script 가 돌 것 여섯(`libkz-llvm.a`, `kz-static`, `static_golden`, `test_static_cli`, `test_static_plan`, `test_static_reasons`). 여기에 `libkz-llvm.a` 를 이름 대는 link 파일의 실행 파일을 더해 열넷이다(`relink_expected.txt`: `test_static_elf32`, `expr`, `flow`, `gitrev`, `link`, `recipe`, `unroll`, `wrappers` 더함). §12.11 의 열넷과 같은 집합이다. 정체성이 바뀌는 도구는 kz-static 하나다. 다시 컴파일되는 목적 파일은 열넷이다: `kz-llvm` 의 `ast_fn`·`order`·`plan_gen`·`rel_facts`·`relations`·`semantics`·`serialize`, `kz-static` 의 `cmd_plan`·`cmd_show`·`main`, 시험 넷.
- **다시 링크되지 않아야 할 것** 110: 지금 sha256 이 §12.11 의 빌드 전 목록과 같다(`not_relink_now.sha256`). 선언은 빌드 바로 전에 다시 잰다.
- **첫 ctest 의 실패, 이름으로**(완전한 소스, lead 의 판단 (d)): `static_golden_order_np`, `static_golden_resultflow_np`, `static_golden_resultflow_order_np`, `static_golden_looplock_order_np`, `static_golden_closure_np`, `test_static_plan`. 커밋할 golden 일곱은 `scratch6/checkbuild/golden_scratch/` 에 있다. `order_np`, `resultflow_np`, `resultflow_order_np`, `looplock_order_np` 은 바꾸고, `closure_np` 는 새것이다. `planv4_np.plan` 은 바꾸고, `planv5_np.plan` 은 새것이다. 앞 실행들의 scratch golden(`scratch/`, `scratch3/`, `scratch4/`, `scratch5/` 의 것)은 이 일곱이 대신한다.
- **허용 차이**: 출력 종류와 칸마다 20.3 의 표와 `compare_2_table.txt` 다. 선언은 이 보고를 `declared/` 로 삼아 그 줄과 수를 적는다. 쌍 파일은 비교할 짝이 없는 새 출력이다. 그래서 머리·꼬리·값의 수를 선언한다.
- **공간이 드는 곳**(선언된 빌드):

| 일 | 크기 | 근거 |
|---|---|---|
| kz-static 사본(xz) | 약 47 MB [I] | `1a47abd0` 사본 47,373,676 B(§12.11). 같은 크기의 바이너리 |
| MC 빌드의 dip | 약 170 MB, 잠깐 | §12.11 의 `make kz-static` dip 170,074,112 B, `make all` 40,079,360 B(같은 relink 집합) |
| 남길 새 출력 | 245.7 MB | `scratch6/output_sizes.txt`: final_extract 73.0, off_extract 47.0, final_order 42.3, off_order 39.6, order 31.9, relcheck 7.6, 계획 넷 4.3(MB). 쌍 파일이 order 출력의 대부분이다 |
| 두 번째 실행(`again/`) | 97.5 MB, 비교 뒤 지움 | 같은 파일 |
| 비교의 facts 보기 | 88.5 MB, 잠깐 | `scratch_guard6/k50_cmp2-extract_final.log` 의 dip 88,514,560 B |

그 합은 약 480 MB 다. 지금 여유는 1,193,545,728 B 다. 그러므로 가장 낮은 값은 약 0.71 GB 로 하한 314,572,800 B 위다. 압축 대체는 필요 없다 [I].

### 20.5 lead 의 판단 (e): 둘째 주소 넘김 호출 자리와 `other` 종류 [I]
- **호출 자리 둘**(`scratch6/address_passing_sites.txt`, 두 C1 빌드 같음):
  - `CFE_ES_RunLoop` at `cf_app.c:338:12`: 주소 넘김 덩이 68. 설계 행의 자리다(§17.3).
  - `CFE_ES_RunLoop` at `bp/fsw/src/bp_app.c:818:12`(`BP_AppMain`): 덩이 4. 저장한 위치는 지역 변수 `run_status` 다(`:814:9` 의 `CFE_ES_RunStatus_APP_RUN`, `:834:13` 의 실패 값). 머리의 "저장한 위치가 필드가 아님 4" 가 이 자리다.
  - 그러므로 필드 저장값 규칙이 필드에 적용되는 자리는 C1 에서 하나뿐이다(§17.3 의 [미확인]이 풀림).
- **`other` 종류 2,398 행**(c1_pre_tx·c1_post_tx 같음; `open_other_c1_*_2.txt`, 열린 행 8,549/8,554):
  - 2,065 행은 "returns <식>, neither a constant nor this result" 부분을 가진다. 함수가 상수도 아니고 그 호출의 결과도 아닌 값을 돌려주는 경우다. 그 가운데 2,006 은 이 종류만 가진다.
  - 392 행은 괄호 안의 `; ` 에서 잘린 꼬리(`and N more)`)를 가진다. 머리 줄의 분류(`relations.cpp` `sf_class`)가 까닭을 괄호를 보지 않고 `; ` 로 나누기 때문이다. 그 가운데 **333 행은 이 잘림 때문에만 `other` 로 세어진다**(앞 부분은 다른 종류, 대개 `outcome not enumerated`).
  - 그 밖의 `other` 부분은 없다.
- 이 세기의 결함은 머리 줄의 수에만 닿는다. 대안 줄과 정한 호출에는 닿지 않는다. 고치면 소스가 바뀌어 scratch 점검 빌드를 다시 해야 하므로 이 단계에서 고치지 않았다(20.8).

### 20.6 공간 [M]
- 시작: 1,204,490,240 B(`free_start_attempt6.txt`, 05:01Z).
- 감시 기록 58 이 모두 `verdict=completed` 다(`scratch_guard6/`). 가장 낮은 값은 606,969,856 B 다(`k50`: 마지막 옵션 회차 비교의 facts 보기를 만들 때. 다섯 빌드의 scratch 출력 343 MB 와 점검 빌드 tree 가 함께 있었다). 하한을 넘지 않았다.
- 크기: 시도 빌드 tree 283 MB, 점검 빌드 tree 283 MB(시험 실행 파일은 묶음마다 지움; 묶음 dip 242/244/170 MB), scratch 출력 343 MB.
- scratch 를 모두 지운 뒤: 1,193,545,728 B(`free_end_attempt6.txt`, 05:43Z). 기록 디렉터리에 더한 것은 약 6.5 MB 다. 압축 대체는 쓰지 않았다.

### 20.7 계획과 다르게 한 것
- **개정 이름의 자리.** §15.2 는 "`relations rules revision 12`·`order rules revision 5` 를 바꾼다"고 적었다. 그러나 5x 의 형식 두 문장("없는 사실은 앞판의 바이트에 도구 줄만", "글은 도구 줄 말고 바뀌지 않는다")과 golden 시험의 `--pairs` 점검이 그것을 막는다. 그래서 형식대로 개정 14 와 8 에 자기 줄과 자리를 주고, 덧붙임 아홉째로 먼저 커밋했다. 기본 `rules` 줄과 order 첫 줄은 그대로다.
- **부재 기원 fixture 의 꼴.** 첫 시도(검색 안의 이른 반환)는 부재 값을 정하지 않아 줄이 나오지 않았다. 감싸개 꼴로 바꿨다. 첫 시도의 출력은 `trials/fixture/` 에 남겼다.
- **"전체 ctest".** 점검 빌드 tree 에서 271 가운데 84 를 돌렸다. 다시 링크되는 열넷의 시험, kz-static 을 부르는 시험, 정체성 시험이다. 나머지 187 은 다시 링크되지 않는 110 바이너리의 시험이다. scratch 에 모두 빌드할 공간이 없다(MC tree 4.1 GB). 그 결과는 선언된 빌드의 전체 ctest 에서 나온다.
- **비교 보고의 첫 판.** `compare/` 는 절대 경로 줄을 경로마다 따로 묶었다. 경로 줄을 접는 판으로 `compare_2/` 를 새로 만들었고, 이 절은 그것을 쓴다. 첫 판의 보고와 표는 덮어쓰지 않고 남겼다. scratch 경로는 남긴 기록에서 `<scratch>` 로 썼다.
- **계획 sidecar 의 사본**(`scratch6/plans_v4_ns2/`)은 scratch 경로를 `<scratch>` 로 바꿨다. 그래서 `s6_outputs.sha256` 의 값과 바이트가 다르다.
- 이 단계는 다섯 빌드의 scratch 출력을 지웠다. sha256 은 모두 `s6_outputs.sha256` 에 남겼다. 선언된 빌드의 쌍둥이 비교는 이 sha256 과 하거나, 점검 빌드를 다시 해서 한다(20.8).

### 20.8 열린 것과 다음 차례
- **다음 차례**(§19.8 (8)의 뒤 절반):
  1. 선언을 커밋한다. 허용 차이는 출력 종류·칸마다 `compare_2/` 를 `declared/` 로 삼는다. relink 는 열넷, 다시 링크되지 않아야 할 것은 110(빌드 바로 전의 sha256)이다. 첫 ctest 실패는 20.4 의 여섯 이름이고, 공간은 20.4 의 표다. 쌍둥이 비교를 sha256 으로 할지 정한다.
  2. kz-static `bf2ec2ee` 의 사본을 만든다(xz, 왕복 점검, 등록부 행).
  3. MC 빌드를 한다(`make kz-static`, `make all`).
  4. 첫 ctest(`-V`)에서 여섯 실패를 확인하고 golden 일곱을 갱신하며 `scratch6/checkbuild/golden_scratch/` 와 대조한다.
  5. 누적 점검을 한다: 바이트 동일성 목록, `ledger_check.sh`, `tools_check.sh --deep`, 전체 ctest, lint, `env` 단위 점검.
  6. 다섯 빌드를 한 번 다시 뽑는다(마지막 옵션, 기본 경로; 절마다 sha256).
  7. relations, 쌍 파일을 낸 order, 새 자료 파일 위의 개정 4·5 계획(대조 예산 사유는 "control runs of design rows")을 돌린다.
  8. 끝 점검을 한다: 선언된 차이만 있는지, 두 C1 빌드의 키 글이 같은지, 두 C1 계획의 모든 계획된 행에 쌍 파일이 완전한지, 탐침을 도구 줄이 재현하는지, 새 줄마다 다섯 빌드에서 서는 곳과 빠지는 곳.
  9. 결과 절을 쓴다.
- **머리 줄의 `other` 세기**(20.5): 괄호 안의 `; ` 로 까닭을 나누어 c1 에서 333 행이 `other` 로 더 세어진다. 고칠지(소스 변경, 다시 scratch 점검)는 lead 의 판단을 기다린다. 고치지 않으면 선언된 빌드의 머리 줄은 이 수를 그대로 낸다.
- **lead 의 판단 (a)**: 평가 갈래 Re 가 17 번 조건 (6)을 덮는지 본다(설명 스크립트 단계). 덮지 않으면 PI 의 열린 것이다.
- **lead 의 판단 (b)**: 성공 경로 줄의 추출 틈. 열린 것이다.
- 틀 없는 문맥(`plan_missing(no call site tells the context apart)`)은 fixture 에도 다섯 빌드에도 없다(§19.8). 이 단계에서도 시험하지 않았다.
- `framework_namespaces2_*.txt` 머리의 "No tool reads this file yet"(§19.8)은 그대로다. 자료 단계의 일이다.
- 다음은 그대로 기다린다.
  - lead 의 판단: §18.8 의 배열 표시, key-clearing 이름 문제.
  - PI 결정: C 라이브러리 항목(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)).
  - absence origins 의 이름 칸 문제.

## 21. C1 닫기의 kz-static 한 빌드: 선언, 도구 사본, MC 빌드와 빌드 뒤 점검 (2026-11-19)

§20.8 의 다음 차례 1–9 를 한다. 이 절은 이 단계만 고친다. 기록: `../records/closure-kzstatic-mc-20261119/`(`README.md`). 출력: `/home/user/work/redesign/obs1/cz_*`(새 디렉터리 열). cFS 를 부팅하지 않고 push 하지 않는다.

### 21.0 선언 (2026-11-19, 도구 사본과 빌드 전에 적고 커밋함) [D]

아래 밖의 차이는 결함이다. 결함이면 그 점검에서 멈추고, 까닭을 밝혀 새로 커밋한 덧붙임으로 선언하기 전에는 다음으로 가지 않는다. 덧붙임은 그것을 쓰는 비교를 돌리기 전에 커밋한다. 모든 일은 공간 감시(`env/tools/space_guard.sh`, 하한 314,572,800 B) 아래에서 돌리고, 감시 기록은 `guard/` 에 둔다. 출력은 덮어쓰지 않고 새 이름으로 둔다.

#### 21.0 (가) lead 의 판단 (2026-11-19, §17 의 열린 것에 대해)
lead 가 2026-11-19 에 정한 것을 그대로 적는다. 이 빌드는 이 판단을 바꾸지 않는다.
- (a) 판정 규칙 8.13 의 17 번 조건 (6)은 쓰인 대로 적용한다. writer 집합은 필드 단위 규칙과 형 단위 규칙 **모두**에서 완전해야 한다. 도구를 필드 단위로만 읽게 바꾸지 않는다. 필드 단위만의 읽기는 설계 행을 본 뒤의 규칙 읽기다. 이미 커밋된 평가 갈래 Re 가 그것을 덮는지는 설명 스크립트 단계와 평가 파일이 정한다. Re 의 커밋된 글이 17 번 조건 (6)을 덮지 않으면 PI 의 열린 것으로 적고, 지금 어떤 글도 넓히지 않는다.
- (b) 성공 경로 줄에서 out 으로 저장되는 지역 변수의 추출 틈은 그대로 둔다. 이 빌드는 `include/` 아래를 바꾸지 않는다(바꾸면 모든 바이너리가 다시 링크된다). 성공 경로 줄은 그 까닭과 함께 계속 missing 이라고 적는다. 열린 것이다.
- (c) 저장된 이름 버퍼: scalar-array FIELD_STATUS 상태가 건전하게 complete 가 될 수 있는지는 scratch 출력에서 깃발마다 정한다. 건전하게 뺄 수 없는 깃발이 하나라도 있으면 그 줄은 not complete 와 그 깃발을 적고, 설계 행이 나오게 하려고 아무것도 느슨하게 하지 않는다(§18.2 에서 정한 대로; 이 빌드의 소스가 그것이다).
- (d) 바뀌는 golden(order golden 셋, `resultflow_np`, `closure_np`, 새것)은 선언된 빌드에서 커밋한다. 선언은 첫 ctest 의 실패를 **완전한 소스**의 scratch 점검 빌드에서 확인한 이름으로 적는다(아래 (마)).
- (e) C1 빌드의 둘째 주소 넘김 호출 자리와 열린 까닭의 `other` 종류는 맞으면 이 빌드의 측정에서 가리고 센다. 아니면 열린 것으로 둔다.

#### 21.0 (나) 소스와 도구 정체성
- **소스**: 이 선언을 커밋하는 HEAD 의 `kairos/` 다. `kairos/` 는 `aa8968f` 뒤로 바뀌지 않았다(`git log aa8968f..HEAD -- research/cfs_procrace/kairos` 가 비어 있음). 그러므로 scratch 점검 빌드(§20.2: `aa8968f` 의 `git archive`, 681 파일)와 같은 소스다. §15–§20 의 소스 진행분이 모두 이 빌드로 도구에 들어간다.
- **정체성이 바뀌는 도구는 kz-static 하나다**(`prebuild/pre_mc.txt`). 파일 sha256 `bf2ec2ee…` 이 바뀐다. `--identity` 는 `tool_sha256` `73c3491c…` → `36e0299521014b926b2f7d2bf6784b5b4ec97a929ff67799a34f72d610509eb5`, `tool_source` `2b8834da…` → `3b89477d1e4fe817d172d12fa839005f3f598e39143a21ff0fb3120279be972c` 여야 한다. 이 값은 scratch 점검 빌드의 값이다(`../records/closure-kzstatic-20261119/scratch6/checkbuild/scratch_tool_identity.txt`). 정체성은 소스 파일의 sha256 에서 나오고 빌드 구성에 기대지 않는다(`kairos/cmake/KzSourceIdentity.cmake`). MC 는 RelWithDebInfo, 점검 빌드는 Release 라서 파일 sha256 은 다를 수 있다. `core_source` `568e5061…`, `schema_revision=12`, `schema_fingerprint` `6fbff6c6…` 는 그대로다.
- **그대로인 도구**(파일과 `--identity`): kz `fd40bc4d…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz_lint `3d4d5a8f…`, kz-rt-dump `40e23d4d…`, kz-instr-clang `36becd04…`.
- **다시 링크되는 것 14**(`prebuild/relink_expected.txt`, 빌드 전 sha256 `prebuild/relink_expected.sha256`): 이 실행의 dry run(`make -n all`, 감시 `guard/d01_make_dryrun.log`, `prebuild/make_dryrun.log`)은 여섯째 실행의 dry run 과 같은 바이트다. 링크 스크립트 여섯(`kz-llvm`, `kz-static`, `static_golden`, `test_static_cli`, `test_static_plan`, `test_static_reasons`), 다시 컴파일되는 목적 파일 열넷이다. 여기에 MC 의 `link.txt` 가운데 `libkz-llvm.a` 를 이름 대는 것을 더한다. 목록: `lib/llvm/libkz-llvm.a`, `tools/kz-static/kz-static`, `tests/static/{static_golden,test_static_cli,test_static_elf32,test_static_expr,test_static_flow,test_static_gitrev,test_static_link,test_static_plan,test_static_reasons,test_static_recipe,test_static_unroll,test_static_wrappers}`. 반드시 바뀌는 것은 `libkz-llvm.a`, kz-static, `static_golden`, `test_static_cli`, `test_static_plan`, `test_static_reasons` 다 [I]. 나머지 여덟은 같은 바이트로 다시 링크될 수도 있다.
- **다시 링크되지 않아야 하는 것 110**(`prebuild/not_relink_paths.txt`, 빌드 전 sha256 `prebuild/not_relink.sha256`): 이 선언 직전에 다시 쟀고, 여섯째 실행의 `not_relink_now.sha256` 과 같은 바이트다. 빌드 뒤에도 같은 sha256 이어야 한다. `CMakeCache.txt` 의 sha256 `bc2f43b4…` 도 그대로여야 한다 [I]. `CMakeLists.txt` 가 바뀌었으므로 다시 구성된다. 그래서 시험이 268 에서 271 로 는다: `static_fixture_compile_closure`, `static_fixture_compile_planv5`, `static_golden_closure_np`(`prebuild/mc_tests_not_in_scratch.txt` 와 점검 빌드의 목록).

#### 21.0 (다) 도구 보존
PI 결정은 이렇다: in-place MC 재빌드는 사본을 압축하고 왕복 확인한 뒤 중간 도구를 덮을 수 있다(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)). 빌드 전에 kz-static `bf2ec2ee…` 를 새 디렉터리 `/home/user/work/preserve/tools-20261119-b/` 에 xz 로 사본하고 왕복 확인한다(`tool_copy/copy.sh`, 감시 필요량 150,000,000). 사본 직후 등록부(`records/TOOLS.tsv`)의 그 행을 in-place+copy 로 고치고, 빌드 뒤 copy 로 고치며 새 kz-static 을 in-place 로 더한다. 장부 행은 `TOOLS_20261119_B` 다.

#### 21.0 (라) 빌드
MC `/home/user/work/merge-check/kz-llvm` 를 제자리에서 빌드한다. `make kz-static` 다음 `make all` 이고, 각각 감시 아래 필요량 480,000,000 이다. 근거는 §12.11 의 dip 170,074,112 와 40,079,360 이다(같은 relink 집합). 빌드 뒤 `--identity` 가 (나)의 값을 적고, 다시 링크된 것과 되지 않은 것이 (나)의 목록과 맞을 때까지 출력을 내지 않는다(`relink_result.txt`).

#### 21.0 (마) 시험
- **첫 ctest**(전체 271, `-V`): 실패는 정확히 다음 여섯이어야 한다(`ctest_first_expected_fail.txt`): `static_golden_order_np`, `static_golden_resultflow_np`, `static_golden_resultflow_order_np`, `static_golden_looplock_order_np`, `static_golden_closure_np`, `test_static_plan`. 근거는 완전한 소스의 scratch 점검 빌드다. 그 첫 ctest(static·lint 82, `-V`)에서 이 여섯이 실패했고, 나머지는 통과했다(그 가운데 처음 Not Run 이던 9 는 빌드한 뒤 통과; §20.2, `cb_ctest_first_V.log`, `cb_ctest_g5-7.log`). 정체성 시험 둘도 통과했다(`cb_ctest_identity.log`). 그 밖의 187 은 다시 링크되는 바이너리의 시험이 아니다. 이 선언 전에 지금 MC 에서 돌렸고, 실패 0·건너뜀 1(`test_probe_anchor_reached`)이었다(`prebuild/ctest_prebuild_187.log`, 감시 `guard/p01_ctest_prebuild_187.log`).
- **golden**: 그 여섯 시험만 `KZ_GOLDEN_UPDATE=1` 로 돌려 쓴다. 바뀌는 golden 은 일곱이다(`kairos/tests/static/data/`). `order_np.golden`, `resultflow_np.golden`, `resultflow_order_np.golden`, `looplock_order_np.golden`, `planv4_np.plan.golden` 은 바뀌고, `closure_np.golden`, `planv5_np.plan.golden` 은 새로 생긴다. 각각 `../records/closure-kzstatic-20261119/scratch6/checkbuild/golden_scratch/` 의 같은 이름 파일과 `cmp` 같아야 한다(`golden_expected.sha256`). `kairos/` 의 그 밖의 파일은 바뀌지 않는다(`git status`; 갱신이 다시 쓰는 `plangen_np.plan.golden` 도 같은 바이트).
- **그 뒤**: 전체 ctest 271, 실패 0, 건너뜀 1. 확인 수는 남긴 출력에서 센다 [I: scratch 점검 빌드]: `test_static_cli` 1,217, `test_static_reasons` 771, `test_static_plan` 389, lint selftest 114 경우 실패 0, lint 규칙 일곱 clean, `test_core_schema` 34,017(그대로). 단위 점검은 `env/tools/tests/run_unit.sh` 17/17, `env/redesign/step2a_tests_unit.sh` 7/7 이다(빌드 전 `prebuild/unit_*_pre.txt` 도 같음).
- **누적 바이트 동일성**: 빌드 뒤와 실행 뒤에 각각 `env/tools/ledger_check.sh`(빌드 전 147 행 실패 0: `prebuild/ledger_check_pre.txt`)와 `env/tools/tools_check.sh --deep`(빌드 전 50 행 실패 0: `prebuild/tools_check_pre.txt`)를 돌린다. 출력은 단계마다 새 이름이다.

#### 21.0 (바) 다섯 빌드의 실행과 허용 차이
- **실행**: `run.sh <step>`. 단계는 여섯째 실행의 scratch 실행기(`scripts/scratch6_run.sh`)와 같은 인자다. 출력 뿌리만 `/home/user/work/redesign/obs1/cz_<step>` 이다. 단계는 relcheck, order, extract_off, extract_final, off_order, final_order, plans_t8cf, plans_final, plans_v4, plans_v4_ns2 이다. 다시 뽑기는 단일 빌드 실행기 `env/redesign/one_build.sh` 로 한 번씩 한다(마지막 옵션과 기본 경로). order 단계는 `--pairs` 로 쌍 파일을 쓴다. 개정 4 계획(`plans_v4`: 이름 공간 자료 개정 1, `plans_v4_ns2`: 개정 2, 계획 규칙 개정 5)의 대조 예산 이유 글은 "control runs of design rows … not a test of the name-binding rule" 다(`plan_one.sh`, `plan_one_ns.sh`). 자료 파일은 단계마다 `data_inputs.sha256` 과 대조한다. 실행 단계의 감시 필요량은 extract·order 150,000,000, 그 밖 30,000,000 이다.
- **허용 차이, 출력 종류와 칸마다**(`compare.sh <step>`, 보고는 `compare/`): 고친 도구 `bf2ec2ee` 의 남긴 출력(`rf2_*`)과의 차이는 §20.3 의 표와 `../records/closure-kzstatic-20261119/scratch6/compare_2_table.txt` 그대로다. 출력 종류마다 이렇다.
  - relations: 도구 줄, 개정 14 의 줄, 새 머리 셋, 새 행 줄들(`success path`, `writers [..]`, `alternatives`, `lifetime end`, `holder`, `absence origins`, `stored-name buffer`, `callee-check (success path)`). 없어지는 줄은 0 이다.
  - order: 도구 줄과 c1 의 행 키 줄 −3/+3(A20, A27, A30). keys 는 개정 14 줄과 c1 의 키 줄 −3/+3, `edges.tsv`·`cdr.txt` 는 같다.
  - 쌍 파일: 새 출력이다. 머리·꼬리·값의 수를 선언한다.
  - 컨테이너: 절마다 BASIS(c1 −49/+52, 그 밖 −1/+1), RESULT_EFFECT −72/+72(c1), STRINGS 수, 머리 칸.
  - 보기, `basis.txt`·`sha256.txt`, 계획(개정 이름, plan id, 생성기·입력·요청 sha256, c1 의 움직인 키 셋, 개정 5 가 더하는 줄).
- **수까지**(`compare.sh declared`): 이 실행의 보고 260(`.samples` 를 뺀 것)이 커밋된 `compare_2/` 의 같은 이름 보고와 **같은 바이트**여야 하고, 이름 집합도 같아야 한다. 그 보고들은 `declared_reports.sha256` 으로 고정한다. 보고는 줄의 종류와 수, 쌍 파일의 줄 수·바이트·sha256·머리·꼬리·값의 수, 컨테이너의 절과 레코드 수를 적는다.
- **쌍둥이**(`compare.sh twin`, `twin_sha.py`, `twin_map.tsv`): §20.8 이 남긴 고름이다. scratch 출력은 지웠고 sha256 만 `s6_outputs.sha256` 에 남았으므로 그것과 견준다. MC 출력의 바이트에서 선언된 문자열 셋만 scratch 쪽 문자열로 바꾸고 sha256 을 견준다. 셋은 출력 뿌리, 도구 디렉터리, MC kz-static 의 파일 sha256(scratch 쪽 `de7fcdd1…`)이다. 둘째 걸음에서는 바꾼 뒤 같지만 바이트로 다른 파일의 sha256 쌍을 바꿈에 더한다(§12.0 (나-2) 의 2 항). scratch 쪽 뿌리는 남긴 계획 sidecar 사본(`<scratch>` 로 가린 것)의 sha256 을 맞추어 되찾았다(`selftest_twin_sha.txt`: 열 파일이 같고, 한 바이트를 바꾸면 DIFF, 지우거나 더하면 한 쪽에만 있음). 견주지 않는 것은 셋이다. `space_guard.log`, `status.txt`(실행 시각과 저장소 HEAD 를 적음; `result PASS` 는 비교가 본다), `again/`(첫 실행과 같은 바이트임을 비교가 본 뒤 지움)이다. 선언: 320 파일이 모두 같고, DIFF 0, 한 쪽에만 있음 0 이다 [I]. 컨테이너(facts.kz, basis.kz)도 바이트로 같아야 한다. 경로가 없고 도구 정체성이 같기 때문이다.
- **그 밖**: order 의 두 실행이 같은 바이트, 로그 빈 파일, 다시 뽑기의 `relations.log` 가 남긴 것과 같은 바이트, `status.txt` 가 `result PASS`, `plan.log` 가 남긴 것과 같은 바이트.

#### 21.0 (사) 끝 점검과 측정 (비교가 모두 OK 인 뒤; 판정이 아님)
1. 두 C1 빌드의 키 글이 같다: 367 가운데 367 [I](`scripts/keys_compare.py`).
2. 쌍 파일이 완전하다: 열 사실 모두 `scripts/pair_check.py` 가 PASS 다. 두 C1 계획(개정 1·2 자료)의 계획된 행 13 이 모두 쌍 파일에 줄을 가진다 [I].
3. 탐침을 도구 줄이 재현하는지 본다. 남긴 탐침 출력(`../records/closure-kzstatic-20261119/probes/out/`)과 새 줄의 수를 탐침마다 놓는다. 맞지 않으면 적고 아무것도 맞추지 않는다.
4. 새 줄마다 다섯 빌드에서 서는 곳과 빠지는 곳의 수를 센다: 성공 경로(held/missing 과 그 까닭), absence origins, holder, alternatives 와 정한 호출, 버퍼·수명 끝. 모두 이 실행의 relations 글에서 `scripts/` 의 셈 스크립트로 센다. 설계 행에서만 보이는 줄이 없게 하려는 것이다.
5. lead 의 판단 (e): 주소 넘김 호출 자리와 `other` 종류를 이 실행의 사실에서 다시 센다. [I]: 자리 2(필드 1), `other` 2,398 이다(§20.5).

#### 21.0 (아) 공간
선언 직전 여유는 1,188,790,272 B(`guard/p03_tools_check_pre.log` 끝)이고 하한은 314,572,800 이다.

| 일 | 감시 필요량 | 예상 크기 [I] |
|---|---|---|
| kz-static 사본(xz) | 150,000,000 | 약 47 MB, 남음 |
| `make kz-static`, `make all` | 480,000,000 씩 | dip 약 170 MB, 잠깐 |
| ctest, golden, 단위 점검 | 60,000,000 | 작음 |
| `tools_check --deep` | 300,000,000 | 잠깐 |
| 실행 열 | 150,000,000 / 30,000,000 | 남길 출력 245.7 MB, `again/` 97.5 MB(비교 뒤 지움) |
| 비교의 facts 보기 | 150,000,000 | 88.5 MB, 빌드마다 지움 |

가장 낮은 값은 약 0.70 GB 로 예상한다. 압축 대체(§12.10 2 항)는 필요 없다 [I]. 모자라면 그 대체만 쓴다.

#### 21.0 (자) 함께 커밋하는 것
- 스크립트: `run.sh`, `compare.sh`, `twin_sha.py`, `twin_map.tsv`, `selftest_twin_sha.txt`, `tool_copy/copy.sh`. Python 은 `python3 -I` 로 돌린다. `diffkinds.py`·`sections_cmp.py`·`plan_one*.sh`·셈 스크립트는 앞 기록 디렉터리의 것을 그 자리에서 읽는다.
- 목록: `declared_reports.sha256`, `golden_expected.sha256`, `ctest_first_expected_fail.txt`, `data_inputs.sha256`, `prebuild/`(dry run, relink 목록과 sha256, `pre_mc.txt`, `cmakecache.sha256`, 빌드 전 ctest·점검·단위 점검).

#### 21.0 (차) 덧붙임 (2026-11-19, golden 갱신 뒤 전체 ctest 의 확인 수가 선언과 달라 멈춘 뒤, 다음 점검 전에 커밋함) [D]
전체 ctest 는 271 가운데 실패 0, 건너뜀 1 이었다(`ctest_full.log`). 그러나 `test_static_reasons` 의 확인 수가 804 였다. (마)에 적은 [I] 771 과 다르다. 선언 밖이므로 멈추고 까닭을 밝힌다.
- `test_static_reasons` 는 자료 디렉터리의 `.golden` 파일을 모두 읽는다. relations golden 의 행마다 `incomplete:` 줄의 까닭이 표에 있는지 확인한다(`kairos/tests/static/test_static_reasons.cpp:105-138`). 그래서 확인 수는 자료 디렉터리의 golden 에 따라 달라진다.
- 선언의 771 은 scratch 점검 빌드의 **첫** ctest 의 수다. 그때 자료 디렉터리에는 HEAD 의 golden 만 있었고 새 `closure_np.golden` 은 없었다. 같은 점검 빌드가 갱신한 golden 일곱을 둔 scratch 자료 디렉터리에서 돌린 수는 804 다(`../records/closure-kzstatic-20261119/scratch6/checkbuild/logs/cb_golden_scratchdata.log`). 이 빌드의 첫 ctest 도 771 이었다(`ctest_first.log`). 그러므로 선언이 golden 갱신 뒤의 수로 잘못 옮겨 적은 것이다. 도구의 차이가 아니다.
- **덧붙임**: golden 갱신 뒤 `test_static_reasons` 의 확인 수는 804 다. 첫 ctest 에서는 771 이다. 그 밖의 (마)는 그대로다.

### 21.1 바뀐 것 [F]
- 커밋: 선언 `4bef01f`(21.0 (가)–(자)), 덧붙임 `44b65bf`(21.0 (차)), 빌드와 빌드 뒤 점검 `5d2b3c3`, 실행·비교·끝 점검과 이 절은 그 뒤 커밋이다.
- 저장소에서 바뀐 것:
  - golden 일곱(`kairos/tests/static/data/`): `order_np`, `resultflow_np`, `resultflow_order_np`, `looplock_order_np`, `planv4_np.plan` 은 바뀌었고 `closure_np`, `planv5_np.plan` 은 새로 생겼다.
  - `records/TOOLS.tsv`: `bf2ec2ee` 행은 in-place → in-place+copy → copy 로 바뀌었고, 새 행 `6f40dd60` 이 생겼다.
  - `records/LEDGER.tsv`: 행 열하나가 생겼다(`TOOLS_20261119_B`, `OBS_cz_*` 열).
  - 기록 디렉터리와 이 절.
- `kairos/` 의 소스는 바꾸지 않았다. 빌드한 소스는 `aa8968f` 의 `kairos/` 와 같다(21.0 (나)).

### 21.2 빌드 [M]
- 사본: `/home/user/work/preserve/tools-20261119-b/kz-static.bf2ec2ee.xz`(47,808,316 B, sha256 `698d4d06…`). `xz -t` OK 이고, 푼 스트림의 sha256 과 `cmp` 가 제자리 파일과 같다(`tool_copy/ROUNDTRIP.txt`, 감시 `guard/b01_tool_copy.log`).
- `make kz-static`, `make all`: 모두 감시 아래에서 rc 0 이다(`make_kz-static.log`, `make_all.log`, `guard/b02`·`b03`).
- 새 kz-static 의 파일 sha256 은 `6f40dd60b29bb6bdf9b38f0a1a8c6e6159e8b448941ff66da06e8f363db10a5b` 다(222,216,240 B). `--identity` 는 `tool_sha256=36e02995…`, `tool_source=3b89477d…` 로 선언과 scratch 점검 빌드의 값 그대로다. `core_source`·스키마 12·`schema_fingerprint` 도 그대로다(`identity_kz-static.txt`).
- relink(`relink_result.txt`): 다시 링크될 수 있는 14 가 모두 바뀌었다. 다시 링크되지 않아야 할 110 은 모두 같은 sha256 이고, `CMakeCache.txt` 도 같다. 다른 도구 여섯의 파일 sha256 도 같다. 정체성이 바뀐 도구는 kz-static 하나다. 다시 구성으로 시험은 271 이 되었다.

### 21.3 시험 [M]
| 시험 | 결과 | 출력 |
|---|---|---|
| 첫 ctest(271, `-V`) | 실패 6. 이름이 선언과 같다: `static_golden_{order_np,resultflow_np,resultflow_order_np,looplock_order_np,closure_np}`, `test_static_plan`(389 가운데 2: planv4·planv5 의 plan golden). 건너뜀 1 | `ctest_first.log`, `ctest_first_check.txt`(`FIRST_CTEST_FAILURES_AS_DECLARED`) |
| golden 갱신(`KZ_GOLDEN_UPDATE=1`, 여섯 시험) | 6/6 통과. 바뀐 golden 일곱이 scratch 사본과 모두 같은 바이트다. `kairos/` 에서 바뀐 것은 이 일곱뿐이다. `plangen_np.plan.golden` 은 다시 쓰였지만 같은 바이트다 | `golden_update.log`, `golden_cmp.txt`, `golden_git_status.txt` |
| 전체 ctest | 271 가운데 실패 0, 건너뜀 1(`test_probe_anchor_reached`). `test_static_reasons` 804 는 선언 밖이었다. 21.0 (차)를 커밋한 뒤 다시 돌렸다 | `ctest_full.log`, `ctest_full_2.log` |
| 확인 수(`ctest_counts_2.txt`) | `test_static_cli` 1,217, `test_static_reasons` 804, `test_static_plan` 389, `test_core_schema` 34,017, lint selftest 114 경우 실패 0, lint 규칙 일곱 clean | |
| 단위 점검 | `run_unit.sh` 17/17, `step2a_tests_unit.sh` 7/7 | `unit_tools.txt`, `unit_step2a.txt` |
| 누적 바이트 동일성, 빌드 뒤 | `ledger_check` 148 행 실패 0, `tools_check --deep` 51 행 실패 0 | `ledger_check_after_build.txt`, `tools_check_after_build.txt` |
| 누적 바이트 동일성, 끝 | `ledger_check` 158 행 실패 0, `tools_check --deep` 51 행 실패 0 | `ledger_check_end.txt`, `tools_check_end.txt` |

- 두 전체 ctest 사이에 `test_probe_observe` 의 확인 수가 3,334 → 3,342 로 달랐다. 그 바이너리는 다시 링크되지 않는 110 가운데 하나다(sha256 같음). 앞 기록에도 같은 시험의 수로 3,334, 3,342, 3,350 이 있다(`records/*/ctest*`). 그러므로 실행마다 달라지는 수로 읽는다 [미확인: 그 시험의 코드에서 까닭을 따라가지 않음]. 도구 정체성의 차이는 아니다.

### 21.4 바이트 동일성과 비교 [M]
| 비교 | 결과 | 출력 |
|---|---|---|
| 실행 단계 열 | 모두 rc 0. 다섯 빌드 모두 `result PASS` | `run_all.txt`, `run_<step>.txt` |
| 고친 도구의 출력과의 보고 | OK 15/35/50/50/50/60/30/30/35/35, BAD 0(relcheck, order, extract_off, extract_final, off_order, final_order, plans_t8cf, plans_final, plans_v4, plans_v4_ns2). 수는 여섯째 실행의 scratch 비교와 같다 | `compare_<step>.txt`, `compare/` |
| 보고의 바이트(`declared`) | 260/260 이 커밋된 `compare_2/` 의 보고와 같은 바이트다. 이름 집합도 같다 | `compare_declared.txt` |
| 쌍둥이(`twin`) | 320/320 같음. DIFF 0, 한 쪽에만 있음 0, 견주지 않음 85(`again/` 65, `space_guard.log` 10, `status.txt` 10). 둘째 걸음의 바꿈 58. facts.kz·basis.kz 20 도 바이트로 같다 | `compare_twin.txt` |
| `again/` | 두 실행이 같은 바이트임을 본 뒤 15 디렉터리를 지웠다 | `compare_again_rm.txt` |

그러므로 이 빌드의 출력은 선언한 그대로다. 고친 도구 `bf2ec2ee` 의 출력과 다른 줄은 모두 §20.3 표의 종류와 수 그대로이고, 파일은 완전한 소스의 scratch 점검 빌드의 출력과 바이트로 같다(선언된 문자열 셋을 바꾼 뒤). 출력: `/home/user/work/redesign/obs1/cz_*`(245,512,941 B, `outputs_sizes.txt`). 파일마다 sha256 은 장부 행 `OBS_cz_*`(`cz_<step>.sha256`)에 있다.

### 21.5 끝 점검과 측정 [M]
수는 `exit/` 의 파일에서 왔다(`exit_checks.sh`, `probe_vs_tool.py`). 판정이 아니다. 차례는 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642 다.
1. **키 글**(`exit/keys_compare.txt`): 두 C1 빌드의 키 글이 367 가운데 367 같고, 키 집합도 같다. 고친 도구와 견주면 키 글이 움직인 행은 3/3/0/0/0(A20, A27, A30)이다. 서로 다른 글이 키를 나누는 경우는 0 이다.
2. **쌍 파일**(`exit/pair_check_*.txt`): 열 사실 모두 `result PASS` 다(마지막 옵션 22 점검, 기본 경로 20 점검). 두 C1 계획(개정 1·2 자료)의 계획된 행 13 이 모두 쌍 파일에 줄을 가진다(그 줄 1,125: unordered 1,051, unknown 35, same_thread 39; writer 1,118, creator 6, creator-not-a-pair(probe-create) 1). 텍스트의 listed 쌍 1,988 은 모두 같은 값의 줄을 가진다.
3. **새 줄이 서는 곳과 빠지는 곳**(다섯 빌드 모두):
   - 성공 경로(`exit/success_path_*.txt`): creator callee 14/14/12/15/15, held 0/0/0/0/0 이다. missing 의 까닭은 이렇다.
     - c1 둘: out-id 저장 없음 3, 저장값이 성공 경로에 묶이지 않음 4, 조건 사이의 함의를 정하지 못함 7.
     - r2: 2/3/7.
     - c5 둘: returns 를 다 따라가지 못함 1, 5/4/5.
     - 이름 공간 블록의 callee 줄도 모두 missing 이다(19/19/13/17/17).
   - absence origins(`exit/absence_origins_*.txt`): 줄 39/39/17/23/23 이다. 기원이 둘인 줄은 7/7/4/0/0 이다. 기원의 종류는 `[search no-match]` 39/39/17/23/23, `[other]` 7/7/4/0/0 이다.
   - holder(`exit/holder_*.txt`): 줄 26/26/21/18/18, 그 가운데 필드 holder 8/8/2/0/0 이다(c5 는 모두 지역 변수). 값을 시험하는 reader 18/18/5/0/0, 시험 없이 쓰는 reader 26/26/4/0/0 이다.
   - alternatives(`exit/alternatives_*.txt`): 줄 65/64/26/66/66 이다. found(문맥이 세워지지 않음) 5/6/2/0/0, none found 3/3/3/0/0 이고, 나머지는 not established 다. 정한 호출 0/0/0/0/0 이다. 전제 (6)이 걸린 줄은 c1_pre_tx 에 1 이다.
     - **(2026-11-19 덧붙임, `rules-06`)** none found 9 줄(c1_pre_tx·c1_post_tx·r2_core 에 셋씩; c1_pre_tx 는 `obs1/cz_final_order/c1_pre_tx/relations.txt:856`, `:901`, `:1455`, 행 A23·A25·A48)은 모두 `rests on: none` 이다. 그 행들의 absence 효과는 `terminal [CFE_PSP_Panic (terminal operation)]` 이고, `CFE_PSP_Panic` 은 `abort()` 를 부른다(`psp/fsw/pc-linux/src/cfe_psp_support.c:133`, 묶음 `1df262a`). 그 행들의 `layers` 줄은 `P0-unverified(abort, …)` 를 적는다. 대안 줄의 `rests on` 은 효과의 직접 callee 의 항목만 적어(`kairos/lib/llvm/relations.cpp:4987-4994`, `:5050-5053`) 감싼 함수 아래의 P0 미대조 항목 `abort` 를 빠뜨린다. 동결한 판정 부분은 `rests on` 이 아무것도 이름 대지 않으면 그 줄을 `held` 로 읽는다(`env/obs1/explain_judge.py:1027-1039`). 이 주기의 계획된 행에는 그런 줄이 없다. 다음 kz-static 빌드로 미룬다(§26.0 (하)). 줄은 `../records/closure-notes-20261119/check.txt` F. [§26.1]
   - 저장된 이름 버퍼(`exit/buffer_*.txt`, `heads_*.txt`): 버퍼 16/16/17/0/0, complete 0 이다. FIELD_STATUS disabled 11/11/12, 다른 writer 5/5/5 이다. 깃발 address-escape 는 모든 버퍼에서 풀리지 않았다. array-element 깃발은 모든 버퍼에서 풀렸다(lead 의 판단 (c), §18.2). 주소 사용 20/20/21 가운데 풀린 것 6/6/6 이다.
   - 수명 끝: clear-table 5/5/6/0/0, clear-slot 10/10/6/18/18(free-record 조건이 보인 것 0), release 1/1/0/23/23, 매개변수를 통한 release 1/1/1/0/0 이다.
     - **(2026-11-19 덧붙임, `semantics-03`)** 이 clear-slot 수는 lifetime end 의 수가 아니다. C1 의 10 줄(`obs1/cz_final_order/c1_pre_tx/relations.txt:207`, `:256`, `:329`, `:352`, `:353`, `:354`, `:627`, `:980`, `:1834`, `:1835`; c1_post_tx 도 같은 자리 10) 가운데 이름을 지우는 것은 2 다: `cfe/modules/tbl/fsw/src/cfe_tbl_api.c:637`(`CFE_TBL_Unregister`)과 `cfe_tbl_internal.c:1267`(`CFE_TBL_CleanUpApp`), 둘 다 주석 "Remove Table Name" 아래의 `Name[0] = '\0'` 이다. 여섯은 같은 필드로 `strncpy` 한 바로 다음 줄의 NUL 종결 저장이다(`bp_storage.c:132`, `cfe_psp_start.c:251`, `cfe_tbl_api.c:442`, `:312`, `osapi-filesys.c:462`, `osapi-file.c:375`). `osapi-sockets.c:97` 은 소켓 이름을 만들 때의 빈 이름 대체이고, `osapi-idmap.c:640` 은 할당의 초기화다(`semantics-02`, §25.11 의 덧붙임). relations rules revision 14 가 그것들을 `clear-slot` 으로 센다. 도구의 한계이고 다음 kz-static 빌드로 미룬다(§26.0 (하)). r2_core 6 과 c5 18 은 다시 보지 않았다 [미확인]. 계획된 행의 결과는 바뀌지 않는다: 두 C1 계획의 저장 쪽 수명 끝 site 는 `osapi-idmap.c:640` 의 site 264 하나다(`obs1/cz_plans_v4_ns2/c1_<pre|post>_tx/plan.kzs` 의 `object-clear` site 둘 가운데 다른 하나는 `OS_ObjectIdInit` 의 `memset` 호출). 줄의 자리는 `../records/closure-notes-20261119/check.txt` G. [§26.1]
   - 이 수들은 §16–§20 의 scratch 수와 같다. 줄이 설계 행에만 있지 않다. 다섯 빌드 모두에 서는 곳과 빠지는 곳이 있다.
4. **탐침과 도구 줄**(`exit/probe_vs_tool.txt`; 탐침은 고친 도구의 사실에서 돌린 §15.1 의 출력):
   - P6 키: 탐침은 "소스 트리 토큰이면 367/367" 이다. 도구 줄도 그렇다(1). 재현된다.
   - 열린 까닭 모집단: 탐침은 필드 호출 자리 1/1/0/0/0 이다. 도구 머리 줄은 호출 자리 2/2/0/0/0 이고, 그 가운데 필드 1 이다. 다른 하나는 `bp_app.c:818:12` 의 지역 변수 `run_status`, 덩이 4 다(`exit/address_passing_*.txt`: 338:12 의 68, 818:12 의 4). 재현된다.
   - P4 holder:
     - A20(두 빌드): 탐침은 "다른 정의 `bp_flow.c:132:18`(OS_CountSemCreate)로 성립하지 않음" 이다. 도구 holder 줄도 그 다른 정의를 writer 로 적는다.
     - A21: 탐침은 "writer 집합이 완전하지 않아(per-field DISABLED) 세우지 못함" 이다. 도구도 FIELD_STATUS DISABLED 와 깃발 다섯을 적는다.
     - A22·A26: 지역 holder 다. 도구는 readers not decided 다.
     - 재현된다.
   - P1 alternatives:
     - A20(두 빌드): 탐침은 retry 와 같은 함수의 생성을 찾았다. 도구도 둘을 적고, retry 는 열린 event-send 둘 때문에 문맥을 세우지 않는다.
     - A21 c1_pre_tx 의 첫 문맥: 탐침은 "none found" 다. 도구는 "not established" 다. 열린 exit-app 이 있고, `CFE_ES_RunLoop 338:12` 의 필드 저장값 규칙이 (5)·(6) 전제에 걸리기 때문이다. lead 의 판단 (a) 대로다. 둘째 문맥은 둘 다 not established 다.
     - A21 c1_post_tx: 탐침은 문맥 둘(위로 걸어 올라간 것)이다. 도구는 `CF_CFDP_InitEngine@cf_cfdp.c:1003:23` 의 문맥 하나에서 retry 를 찾고, 열린 return-result 때문에 세우지 않는다. 문맥을 나누는 꼴이 탐침과 다르다 [미확인: 두 빌드의 호출 꼴 차이를 따라가지 않음].
     - A22·A26: 탐침은 "효과가 호출자로 돌아가는 함수에서 끝남" 이다. 도구는 호출자까지 걸어 올라가 다른 까닭으로 not established 다(열린 exit-app, RESULT_EFFECT 행 없음).
     - 읽기(세우지 못함)는 모두 같다. 글은 다르다.
   - P2 성공 경로: 탐침은 `OS_CountSemCreate` 를 HELD 로 읽었다. 도구는 "저장값이 성공 경로에 묶이지 않음" 으로 missing 이다. 이것은 lead 의 판단 (b)의 추출 틈이다(out 으로 저장되는 지역 변수). 재현되지 않으며 열린 것이다. 나머지 넷(OS_TimeBaseCreate, OS_mkfs, OS_initfs, OS_mv)은 둘 다 missing 이지만 까닭의 글이 다르다.
   - P5 쌍: 탐침의 C1 텍스트 쌍 합은 24,796 이다. 남긴 쌍 측정과의 차이 187 은 §15 에서 설명했다. 쌍 파일의 쌍은 26,572 다(same_thread 1,920 포함). 계획된 행의 줄은 1,125 다. 탐침이 센 그 행의 텍스트 쌍 1,087 보다 38 많다. 쌍 파일의 영역은 not-a-pair 생성자와 같은 스레드의 생성자까지 넣는다(§15.2). 그것으로 보이지만, 행마다 맞춰 보지 않았다 [미확인].
5. **lead 의 판단 (e)**(`exit/address_passing_*.txt`, `exit/open_other_*.txt`, 보기는 감시 아래에서 만들고 지움, sha256 은 `exit/ruling_e_views.sha256`):
   - 둘째 주소 넘김 호출 자리는 `CFE_ES_RunLoop` at `bp_app.c:818:12` 다(덩이 4, 지역 변수). 필드 저장값 규칙이 필드에 적용되는 자리는 C1 에서 `cf_app.c:338:12` 하나다(덩이 68).
   - `other` 종류는 2,398 행이다(c1 두 빌드, 열린 행 8,549/8,554). 2,065 행은 "상수도 그 호출의 결과도 아닌 값을 돌려줌" 이다. 392 행은 괄호 안의 `; ` 에서 잘린 꼬리이고, 그 가운데 333 행은 잘림 때문에만 `other` 로 세어진다. §20.5 의 scratch 수와 같다.

### 21.6 공간 [M]
| 시점 (UTC, 2026-11-19) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 시작(05:51) | 1,190,694,912 | `free_start.txt` |
| 선언 직전 | 1,188,790,272 | `guard/p03` 끝 |
| 사본 뒤 | 1,138,950,144 | 사본 47.8 MB |
| `make kz-static` 중 가장 낮은 값 | 798,343,168 | dip 340,557,824(선언의 [I] 170 MB 보다 큼; 감시 필요량 480,000,000 안) |
| `make all` 중 가장 낮은 값 | 1,073,557,504 | dip 33,517,568 |
| 실행 중 가장 낮은 값 | 737,320,960 | 새 출력과 `again/` |
| 비교 중 가장 낮은 값 | 692,862,976 | 마지막 옵션 회차의 facts 보기(`guard/k_extract_final.log`) |
| 끝(06:30) | 833,384,448 | `again/` 를 지우고 scratch 를 모두 지운 뒤(`free_end.txt`) |

- 하한(314,572,800)을 넘지 않았다. 감시 기록 45 가 모두 `verdict=completed` 다(`guard/`). 압축 대체는 쓰지 않았다.
  - **(2026-11-19 덧붙임, `builds-06`)** '모두 `verdict=completed`' 는 하한의 근거가 아니다(감시는 끝난 뒤의 값이 하한 아래여도 `completed` 를 적는다; §12.11 (마)의 덧붙임). 근거는 감시 기록 45 의 가장 낮은 값 692,862,976 B 와, 끝 값이 하한 아래인 기록이 0 이라는 것이다(`../records/closure-notes-20261119/check.txt` A). [§26.1]
- 남긴 것: 새 출력 245,512,941 B, 사본 47,808,316 B, 기록 디렉터리 약 6 MB.

### 21.7 계획과 다르게 한 것
- **확인 수 하나의 선언 오기.** `test_static_reasons` 의 golden 갱신 뒤 수를 scratch 첫 ctest 의 수(771)로 적었다. 전체 ctest 에서 804 로 멈췄고, 까닭(그 시험은 자료 디렉터리의 golden 을 모두 읽음)을 덧붙임 21.0 (차)로 커밋한 뒤 전체 ctest 를 다시 돌렸다. 첫 전체 ctest 의 출력(`ctest_full.log`, `ctest_counts.txt`)은 덮어쓰지 않고 남겼다.
- **쌍둥이 비교의 꼴.** scratch 출력은 여섯째 실행이 지웠다. 그래서 그 sha256 목록과 견주었다. 그러려면 scratch 쪽 뿌리의 바이트가 필요했다. 남긴 기록에서는 `<scratch>` 로 가려져 있었다. 그것을 가린 sidecar 사본의 sha256 을 맞추어 되찾았고(선언 21.0 (바)), 커밋한 파일에는 `<scratch>` 로만 적었다.
- **`make kz-static` 의 dip** 은 340 MB 로 선언의 예상(약 170 MB)의 두 배였다. 감시 필요량(480,000,000) 안이고 하한 위다. 이번 빌드는 `CMakeLists.txt` 가 바뀌어 다시 구성까지 했기 때문으로 보인다 [미확인].
- 탐침과 도구 줄의 대조(21.5 4)는 탐침마다 대상을 짝지어 놓은 것이다. 글이 다른 곳은 읽기만 견주었다. 도구도 탐침도 고치지 않았다.
- **(2026-11-19 덧붙임, `builds-03`)** 실행 단계와 비교 단계가 나란히 돌았다: 실행 `extract_final`(`guard/r_extract_final.log` 06:21:38.980–06:23:15.285Z)이 도는 동안 비교 relcheck(06:22:04Z)·order(06:22:06Z)·extract_off(06:22:08–06:22:20Z)가 돌았고, 실행 `off_order` 는 06:23:15Z 에 그 비교들을 기다리지 않고 이어졌다. `extract_final` 실행은 그 비교들의 rc 가 나오기 전에 시작했다(§12.10 6 항 '명령마다 rc 를 본 뒤 다음으로' 와 어긋남). 모든 rc 는 0 이었다. `run_all.txt` 가 적은 `extract_final` 의 가장 낮은 값 889,491,456 B 에는 함께 돈 비교의 facts 보기(`guard/k_extract_off.log`, dip 26,329,088 B)의 공간이 들어 있을 수 있다 [I]. 겹침의 목록은 `../records/closure-notes-20261119/check.txt` B. [§26.1]

### 21.8 열린 것
- **끝 조건**: 모두 섰다. MC 의 kz-static 은 새 도구 `6f40dd60…`(`tool_sha256=36e02995…`)이고, 빌드 뒤 점검이 모두 통과했으며, 지금까지의 차이는 모두 선언되었다(21.0, 21.0 (차)).
- lead 의 판단 (a): 평가 갈래 Re 가 판정 규칙 8.13 의 17 번 조건 (6)을 덮는지는 설명 스크립트 단계가 본다. 덮지 않으면 PI 의 열린 것이다. 이 빌드에서 그 조건이 걸린 것은 C1 두 빌드의 덩이 66 이다(머리 줄 "(5) or (6) rests on a premise 66").
- lead 의 판단 (b): 성공 경로 줄의 추출 틈(out 으로 저장되는 지역 변수)은 그대로다. 탐침 P2 의 `OS_CountSemCreate` HELD 를 도구가 재현하지 않는 까닭이 이것이다.
- lead 의 판단 (c): 버퍼는 모두 not complete 다. address-escape 깃발이 풀리지 않았기 때문이다. 아무것도 느슨하게 하지 않았다.
- 머리 줄의 `other` 세기(괄호 안의 `; ` 로 나눔, c1 에서 333 행)는 그대로 냈다. 고칠지는 lead 의 판단을 기다린다.
- 탐침 P1 의 A21 c1_post_tx 문맥 꼴과, 계획된 행의 쌍 줄 1,125 와 텍스트 쌍 1,087 의 차이 38 을 행마다 맞추는 일은 하지 않았다 [미확인].
- `test_probe_observe` 의 확인 수가 실행마다 달라지는 까닭은 보지 않았다.
- §20.8 의 그 밖의 열린 것은 그대로 기다린다: 틀 없는 문맥의 시험, `framework_namespaces2_*.txt` 머리 글, §18.8 의 배열 표시와 key-clearing 이름 문제(lead), C 라이브러리 항목(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)), absence origins 의 이름 칸 문제.

## 22. C1 닫기의 kz-static 한 빌드: 끝 점검의 남은 것과 빌드 전체 표 (2026-11-19)

이 절은 선언된 kz-static 빌드(§21.0)의 다섯 빌드·쌍 파일·계획·끝 점검 단계에서 §21 이 하지 않은 것만 더한다. 다섯 빌드는 §21 에서 선언된 도구(kz-static `6f40dd60…`, `tool_sha256=36e02995…`)로 한 번 다시 뽑혔고(`cz_*`, `ba6a157`), relations·order·쌍 파일·계획·비교·끝 점검이 모두 거기서 끝났다. 끝낸 일은 다시 하지 않았다: 다시 뽑기, 새 출력 디렉터리, 도구 바꿈이 없다. 이 단계는 남긴 출력을 읽기만 하고, 기록 디렉터리 `../records/closure-kzstatic-exit-20261119/`(`README.md`)에만 쓴다. cFS 를 부팅하지 않고 push 하지 않는다. 빌드가 없으므로 선언도 없다. 모든 명령은 공간 감시 아래에서 돌렸다(`guard/`).

### 22.0 lead 의 판단 (2026-11-19)
§21.0 (가)에 적은 lead 의 판단을 그대로 다시 적는다. 이 단계는 어느 것도 바꾸지 않는다.
- (a) 판정 규칙 8.13 의 17 번 조건 (6)은 쓰인 대로 적용한다. writer 집합은 필드 단위 규칙과 형 단위 규칙 **모두**에서 완전해야 한다. 도구를 필드 단위로만 읽게 바꾸지 않는다. 필드 단위만의 읽기는 설계 행을 본 뒤의 규칙 읽기다. 커밋된 평가 갈래 Re 가 그것을 덮는지는 설명 스크립트 단계와 평가 파일이 정한다. 덮지 않으면 PI 의 열린 것으로 적고, 지금 어떤 글도 넓히지 않는다.
- (b) 성공 경로 줄에서 out 으로 저장되는 지역 변수의 추출 틈은 그대로 둔다. 이 빌드에서 `include/` 아래는 바뀌지 않는다. 성공 경로 줄은 그 까닭과 함께 missing 이라고 적는다. 열린 것이다.
- (c) 저장된 이름 버퍼의 scalar-array FIELD_STATUS 상태는 깃발마다 정한다. 건전하게 뺄 수 없는 깃발이 있으면 그 줄은 not complete 와 그 깃발을 적는다. 설계 행이 나오게 하려고 아무것도 느슨하게 하지 않는다.
- (d) 바뀌는 golden 은 선언된 빌드에서 커밋한다. 첫 ctest 의 실패는 완전한 소스의 scratch 점검 빌드에서 확인한 이름으로 적는다(§21.0 (마), §21.3).
- (e) 둘째 주소 넘김 호출 자리와 열린 까닭의 `other` 종류는 맞으면 이 빌드의 측정에서 가리고 센다(§21.5 5). 아니면 열린 것으로 둔다.

### 22.1 시작 점검 [M]
- 여유 831,086,592 B(06:35Z, `free_start.txt`).
- `ledger_check.sh`: 158 행, 실패 0(`out/ledger_check_start.txt`). `OBS_cz_*` 열 행의 파일이 모두 남긴 sha256 과 같다.
- `tools_check.sh --deep`: 51 행, 실패 0(`out/tools_check_deep_start.txt`).
- 다섯 빌드의 relations·order·쌍 파일의 도구 줄 15 개가 모두 `tool_sha256=36e0299521014b926b2f7d2bf6784b5b4ec97a929ff67799a34f72d610509eb5 tool_source=3b89477d…` 이다. 제자리 도구의 파일 sha256 은 `6f40dd60…` 이다(`out/tool_lines.txt`).

### 22.2 컨테이너의 절마다 sha256 [M]
§21 은 컨테이너를 절마다 견주었지만(`compare/extract_*_sections_*.txt`) 절마다의 sha256 은 남기지 않았다. 이 단계가 남겼다.
- 대상: `cz_final_extract` 와 `cz_off_extract` 의 다섯 빌드, 각 `facts.kz` 와 `basis.kz`(컨테이너 20).
- 방법: `section_sha.py`(`records/semantics-20261116/` 의 사본)가 kz-dump(`out/kz-dump.sha256`)의 글을 읽는다. 절마다 수, crc32, 그 절 글의 sha256 을 적는다.
- 줄 수: 마지막 옵션 회차 265(facts 235, basis 30)와 옵션 없는 회차 225(facts 195, basis 30)다(`out/section_sha_final.txt`, `out/section_sha_off.txt`).
- 감시 dip 은 53,248 B 다.

### 22.3 남긴 쌍 측정과 `pairs:` 줄의 합, 쌍 파일 [M]
선언된 도구의 출력(`cz_final_order/<b>/`: `cdr.txt`, `order.txt`, `pairs.tsv`)으로 다시 셌다. 쓴 것은 `pair_measure.py` 이고, 결과는 `out/pair_measure_2_<b>.txt` 다. 세 같음은 세어진 줄로만 확인하고, 빌드의 상수는 쓰지 않는다. 다섯 빌드 모두 `result PASS` 다.

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| K 남긴 쌍 측정(`cdr.txt` 의 `pairs of the rows`) | 24,609 | 24,609 | 8,566 | 4,307 | 4,298 |
| T `pairs:` 줄의 합 | 24,796 | 24,796 | 8,739 | 4,393 | 4,382 |
| 쌍 파일의 쌍(no_pair 뺌) | 26,572 | 26,572 | 9,622 | 5,160 | 5,149 |
| unknown(no root reaches) 줄 | 187 | 187 | 173 | 86 | 84 |
| same_thread 줄: 글이 한 스레드 unknown 으로 셈 / 세지 않음 | 144 / 1,776 | 144 / 1,776 | 135 / 883 | 84 / 767 | 83 / 767 |

1. **T − K = unknown(no root reaches) 줄.** 어느 뿌리도 닿지 않는 쌍을 글은 unknown 으로 센다. caller-dependent 측정은 `pair()` 를 계산한 쌍만 `pair_values_` 에 기록한다(`kairos/lib/llvm/order.cpp:1519`). 그래서 c1 에서 24,796 − 24,609 = 187 이다. §15.4 의 scratch 설명이 선언된 도구의 출력에서도 다섯 빌드 모두 선다.
2. **쌍 파일 − T = 글이 세지 않는 same_thread 줄.** 글이 same_thread 줄을 세는 경우는 하나다. 경로 B·C 에서 그 두 위치의 줄이 하나뿐일 때 한 스레드 unknown 으로 센다(`pair_check.py` 점검 4 의 규칙). 쌍 파일은 영역이 넓다(order rules revision 8: 행이 이름 대는 모든 creator 와 writer, not-a-pair 생성자, 같은 스레드의 생성자). 그래서 c1 에서 26,572 − 24,796 = 1,776 이다.
   - c1 의 1,776 은 writer 경로 B 1,256, 경로 C 507, `creator-not-a-pair(same actor only)` 12, `creator-not-a-pair(probe-create)` 1 이다.
   - r2 의 883 은 경로 B 756, 경로 C 112, same actor only 12, `key constants differ` 3 이다.
   - c5 의 767 은 경로 B 749, same actor only 18 이다.
3. **K = 순서 값의 쌍 + 글이 세는 한 스레드 쌍.** c1 은 hard_before + fixed_early_proved + unordered 24,465 에 144 를 더해 24,609 다. 글의 unknown 331 은 187 + 144 다. unordered 는 셋 모두 21,664 로 같다.

### 22.4 계획된 행의 쌍 줄 1,125 와 텍스트 쌍 1,087 의 차이 38: 행마다 [M]
§21.8 에서 [미확인] 으로 둔 것이다. `pair_measure_2_<b>.txt` 의 행 표와 `planned_same_thread_<b>.txt` 로 행마다 맞췄다.
- C1 두 빌드의 계획(`cz_plans_v4` 와 `cz_plans_v4_ns2`, 같은 13 행)에서 13 행 모두가 "쌍 파일 줄 − 텍스트 합 = 글이 세지 않는 same_thread 줄 + no_pair 줄" 을 만족한다.
- 38 은 세 행에서 나온다. 셋 모두 same_thread 이고, 글이 세지 않는 줄이다.
  - B73: writer 경로 B 33(두 위치에 줄이 여럿).
  - B99: writer 경로 B 4(두 위치에 줄이 여럿).
  - A20: `creator-not-a-pair(probe-create)` 1(`bp_flow.c:132:18` 의 같은 함수 생성).
  - B125 의 same_thread 줄 1 은 글이 한 스레드 unknown 으로 세므로 차이에 들지 않는다.
- 탐침 P5 의 1,087 은 고친 도구의 출력(`rf2_final_order`)과 그 계획(`rf2_plans_v4`)에서 센 것이다. 선언된 도구의 텍스트에서도 같은 13 행의 합은 1,087 이다.
- `rf2_plans_v4` 의 sidecar 를 `cz` 출력에 대면 1,086/1,123 이 된다. A20 의 키가 소스 트리 토큰 때문에 움직였기 때문이다(§15.3, §21.5 1). 그래서 sidecar 의 A20 키가 `cz` 의 행에 맞지 않는다. 같은 행끼리 맞추면 차이는 없다.
- 다른 빌드에서는 차이가 r2 15, c5 310/310 이다. 모두 같은 규칙으로 설명된다(행마다 PASS). 예를 들어 r2 의 대조 행 A2–A4 는 텍스트가 `pairs: none` 인데, 쌍 파일에는 같은 행위자 생성자의 same_thread 줄이 행마다 5 있다.
- 첫 실행(`out/pair_measure_<b>.txt`)은 tier 계획이면서 대조이기도 한 행(A21)의 선택 칸을 `control` 로 적었다. 덮어쓰지 않고 남겼고, 칸을 고친 실행이 `_2` 다. 그 칸 말고는 같다.

### 22.5 계획 규칙 개정 5 가 선언된 계획에 적은 것 [M]
`plan_rev5_counts.py` 를 `cz_plans_v4_ns2` 다섯에 돌렸다(`out/plan_rev5_counts_v4_ns2.txt`). 차례는 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642 다.
- 계획된 경로 A 행은 5/5/4/2/2 다. 그 가운데 `namespace-entry` 줄이 있는 행은 4/4/4/0/0 이다(c1: A20·A21·A22·A26). 없는 행은 1/1/0/2/2 다(c1 의 A63, c5 의 A16·A17).
- `namespace-layer` 줄은 1/1/1/2/2 다.
- `kind-value` site 는 4/4/4/0/0 이다.
- `object-rollback` site 는 4/4/6/0/0, `object-clear` site 는 2/2/2/0/0 이다.
- `context-frame` site 는 4/0/0/0/0 이다. 문맥 줄 2 는 c1_pre_tx 의 A21 한 행에 있고, 틀은 2 를 찾았으며 틀 없는 문맥은 0 이다.
- `lifetime-complete` 는 종류마다 yes 2/2/2/0/0 이다.
- c5 두 빌드의 need 는 `object-delete not-planned plan_missing(no namespace data entry)` 2 뿐이다.
- §19 의 scratch 수, §21.4 의 선언된 비교(같은 바이트)와 같다. 선택, 층, 층 라벨은 바뀌지 않았다.
- 대조 예산의 이유 글은 "the boots of these rows are control runs of design rows" 다(`cz_plans_v4*/<b>/plan.side` 의 `control-budget`). "prior test" 라는 말은 다섯 계획 어디에도 없다.

### 22.6 kz-static 한 빌드의 전체 표 [M]
- 수는 c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642 의 차례다.
- 출처는 둘이다. 하나는 `../records/closure-kzstatic-mc-20261119/exit/`(§21.5)이고, 다른 하나는 이 절의 `out/`(`design_rows_<c1>.txt`, `plan_rev5_counts_v4_ns2.txt`, `pair_measure_2_*.txt`)다.
- 셋째 칸은 C1 의 계획된 행이 그 줄에서 보이는 것이다. 대조 선택 행은 A20·A21·A22·A26·A63 이다. 이 행들의 부팅은 설계 행의 대조 실행이고, A21 은 층 계획 행이기도 하다. 층 계획 행 B13·B73·B81·B99·B113·B118·B119·B125 는 경로 B 라서 경로 A 의 새 줄이 없다.
- 두 C1 빌드에서 이 칸의 읽기는 같다. 위치 글은 다르다.

| 내용 | 서는 곳 / 빠지는 곳(다섯 빌드) | C1 의 계획된 행 |
|---|---|---|
| 소스 트리 토큰의 키 글 | `source_tree` 항목 3/3/0/0/0. 키가 움직인 행 3/3/0/0/0. 두 C1 빌드의 키 글 367/367 같음. 공유 키 0 | A20 의 키가 움직임(A27·A30 은 계획되지 않음) |
| 성공 경로 줄 | creator callee 14/14/12/15/15. held 0 이고 모두 missing(까닭별 수는 §21.5 3) | A20·A21 `OS_CountSemCreate`: missing(저장값이 성공 경로에 묶이지 않음; lead 의 판단 (b)). A22·A26 `OS_TimeBaseCreate`: missing(조건 사이의 함의를 정하지 못함). A63 `OS_mv`: missing(나르는 경로에 out-id 저장 없음) |
| absence origins 줄 | 39/39/17/23/23. 기원이 둘인 줄 7/7/4/0/0. 값이 정해지지 않은 absence 에는 줄 없음 | A20·A21·A22·A26: −17 의 기원 둘(`[other]` `OS_ObjectIdFindByName:972:5`, `[search no-match]`). A63: 줄 없음(absence 값이 평가되지 않음) |
| holder 줄 | 26/26/21/18/18. 필드 holder 8/8/2/0/0. 값을 시험하는 reader 18/18/5/0/0 | A20: 필드 `BP_Throttle_t.Sem`, writer 둘(자기 out 저장, 다른 정의 `bp_flow.c:132:18`). A21: 필드 `CF_Channel_t.sem_id`, writer 둘(채움 0 의 clearing, 자기 out 저장), FIELD_STATUS DISABLED 와 깃발 다섯, reader 하나는 지운 값을 먼저 시험함. A22·A26: 지역 변수, reader 를 정하지 못함. A63: 줄 없음 |
| 문맥별 alternatives 줄과 저장 필드 규칙 | 65/64/26/66/66. 정한 호출 0/0/0/0/0. 주소 넘김 호출 자리 2/2/0/0/0(필드 1). 전제 (5)·(6)에 걸린 덩이 66(c1 두 빌드) | A20: 같은 함수의 생성과 retry 를 찾음. 열린 event-send 둘 때문에 세우지 않음. A21 c1_pre_tx 문맥 1: not established(열린 exit-app; `CFE_ES_RunLoop` 338:12 의 저장 필드 규칙이 전제 (6)에 걸림, `CF_AppData_t.run_status` per-field=writer-set-complete, type-level=DISABLED; lead 의 판단 (a)). 문맥 2: 효과가 호출자로 돌아가는 함수에서 끝남. A21 c1_post_tx: 문맥 하나에서 retry 를 찾음, 열린 return-result. A22: 열린 exit-app. A26: RESULT_EFFECT 행 없음. A63: absence 값이 평가되지 않음 |
| 저장된 이름 버퍼 줄 | 16/16/17/0/0, complete 0. 깃발 address-escape 는 모든 버퍼에서 풀리지 않음. array-element 깃발은 모두 풀림(lead 의 판단 (c)) | 행의 이름 공간 `OS_common_record_t.name_entry` 블록에 버퍼·수명 끝 줄 19. 셈 세마포어(partition {3})와 시간 기준(partition {8})의 버퍼는 not complete(FIELD_STATUS disabled; address-escape 가 풀리지 않음) |
| 수명 끝 줄 | clear-table 5/5/6/0/0, clear-slot 10/10/6/18/18(모두 not shown), release 1/1/0/23/23, 매개변수를 통한 release 1/1/1/0/0 | 같은 블록의 clear-table·clear-slot·release-through 줄 |
| relations 머리의 도구 sha256 줄 | 다섯 빌드의 relations·order·쌍 파일 15 개 모두 `tool_sha256=36e02995…` | — |
| 완전한 쌍 파일 | pair_check PASS 10/10(마지막 옵션과 기본 경로). 쌍 26,572/26,572/9,622/5,160/5,149 | 계획된 13 행 모두 줄이 있음: 1,125 줄(텍스트 1,087 과의 차이 38 은 22.4) |
| 남긴 쌍 측정과 `pairs:` 줄의 합 | 22.3 의 세 같음이 다섯 빌드 모두 섬 | — |
| 계획 규칙 개정 5 | `namespace-entry` 행 4/4/4/0/0, 빠진 행 1/1/0/2/2. 그 밖의 줄은 22.5 | A20·A21·A22·A26: entry 줄 있음. A63: 없음(적용되는 항목이 없음). A21(c1_pre_tx): 문맥 틀 2 |

- **(2026-11-19 덧붙임, `rules-06`)** 위 표의 alternatives 줄에 든 none found 9 줄의 `rests on: none` 은 감싼 함수 `CFE_PSP_Panic` 아래의 P0 미대조 항목 `abort` 를 빠뜨린다(§21.5 3 의 덧붙임). [§26.1]
- **(2026-11-19 덧붙임, `semantics-03`)** 위 표 '수명 끝 줄' 의 C1 clear-slot 10 가운데 이름을 지우는 것은 2 다. 여섯은 `strncpy` 뒤의 NUL 종결, 하나는 빈 이름 대체, 하나는 할당의 초기화다(§21.5 3 의 덧붙임). [§26.1]

### 22.7 공간 [M]
- 시작 831,086,592 B(06:35Z), 끝 830,013,440 B(06:45Z, `free_end.txt`)다.
- 감시 기록은 19 개이고 모두 `verdict=completed` 다. 가장 큰 dip 은 절마다 sha256 의 53,248 B 이고, 하한을 넘지 않았다. 압축 대체는 쓰지 않았다.
- 새로 남긴 것은 이 기록 디렉터리(262,789 B)뿐이다. scratch 는 만들지 않았다.

### 22.8 계획과 다르게 한 것
- **다시 뽑기와 실행을 다시 하지 않았다.** 이 단계의 일(선언된 도구로 다섯 빌드를 한 번 다시 뽑고, relations·order·쌍 파일·계획·비교를 돌림)은 §21 에서 이미 끝났다(`ba6a157`; 모든 비교 OK, BAD 0). 한 번만 뽑는다는 규칙대로 다시 하지 않았다. 남은 것만 했다: 절마다 sha256, 쌍 측정의 설명, 38 의 행별 대조, 계획된 행의 줄, 전체 표.
- **"개정 4 계획" 과 "개정 5 계획".** 같은 계획이다. `cz_plans_v4*` 는 manifest 개정 4 의 꼴에 계획 규칙 개정 5(plan rules revision 5)를 담는다(§19, §21.4).
- **감시 밖에서 돌린 작은 읽기 둘.** `planned_same_thread.py` 두 번과 도구 줄의 grep 은 감시 없이 돌렸다. 각 출력은 수 KB 의 읽기였다. 그 밖의 명령은 모두 감시 아래에서 돌렸다.
- **세는 스크립트의 칸 하나를 고쳤다.** `pair_measure.py` 의 첫 실행은 A21 의 선택 칸을 잘못 적었다. 첫 출력은 남기고, 고친 실행은 새 이름으로 두었다(22.4).

### 22.9 열린 것
- **끝 조건**: 모두 섰다.
  - 선언된 차이만 있다(§21.4: 260 보고가 선언된 보고와 같은 바이트, 쌍둥이 320/320).
  - 두 C1 빌드의 키 글이 같다(367/367).
  - 두 C1 계획의 계획된 행마다 쌍 파일이 완전하다(13/13).
  - 탐침이 도구의 줄로 재현된다. 재현되지 않는 곳은 줄과 까닭을 적었다: P2 의 `OS_CountSemCreate` 는 lead 의 판단 (b), A21 문맥 1 의 not established 는 판단 (a)의 전제 (6)(§21.5 4).
  - 새 줄마다 서는 곳과 빠지는 곳을 다섯 빌드 모두에서 셌다(22.6).
  - 쌍 측정의 차이를 설명했다(22.3).
  - 판단 (e)를 쟀다(§21.5 5).
  - 장부와 등록부의 깊은 점검이 통과했다(22.1, 끝 점검 `out/*_end.txt`).
- lead 의 판단 (a): 평가 갈래 Re 가 판정 규칙 8.13 의 17 번 조건 (6)을 덮는지는 설명 스크립트 단계가 본다. 덮지 않으면 PI 의 열린 것이다. C1 두 빌드의 덩이 66 이 그 전제에 걸린다.
- lead 의 판단 (b): 성공 경로 줄의 추출 틈은 그대로다.
- lead 의 판단 (c): 버퍼는 모두 not complete 다(address-escape).
- 머리 줄의 `other` 세기(괄호 안의 `; ` 로 나눔, c1 에서 333 행)를 고칠지는 lead 의 판단을 기다린다.
- 탐침 P1 의 A21 c1_post_tx 문맥 꼴(문맥 하나 대 탐침의 둘)은 두 빌드의 호출 꼴 차이를 따라가지 않았다 [미확인].
- `test_probe_observe` 의 확인 수가 실행마다 달라지는 까닭은 보지 않았다 [미확인].
- §20.8 의 그 밖의 열린 것은 그대로 기다린다.
  - 틀 없는 문맥의 시험.
  - `framework_namespaces2_*.txt` 의 머리 글.
  - §18.8 의 배열 표시와 key-clearing 이름 문제(lead).
  - C 라이브러리 항목(PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)).
  - absence origins 의 이름 칸 문제.

## 23. C1 닫기의 런타임·덤프·변환·플러그인 한 빌드 (2026-11-19)

§12.10 4 항 (5)의 빌드다: dump 형식 1.5(build-id 레코드와 생성 토큰의 예약 레코드), `kz-rt-dump` 의 글 줄, `kz convert` 의 build_id 칸, 연결 점검의 같은 빌드 줄, 두 플러그인(kzgcc 0.4, kzllvm 0.2)의 `kzs-sites 4` 읽기. 기록: `../records/closure-runtime-20261119/`(README). 출력: 저장소 밖 `/home/user/work/redesign/obs1/rtb_*`(새 디렉터리). cFS 를 부팅하지 않는다. push 하지 않는다.

### 23.0 선언 (빌드 전에 커밋함)

**(가) lead 의 판단 (2026-11-19, §17 의 열린 점; 이 빌드는 어느 것도 건드리지 않음).** (a) 판정 규칙 8.13 17 항 조건 (6)은 글대로 적용한다: writer 집합은 필드별 규칙과 형 수준 규칙 **둘 다**에서 완전해야 한다. 도구를 필드별로만 읽게 바꾸지 않는다. 필드별로만 읽는 것은 설계 행을 본 뒤의 규칙 읽기이고, 커밋된 평가 갈래 Re 가 그것을 덮는지는 설명 스크립트 단계와 평가 파일의 몫이다. Re 의 글이 덮지 않으면 PI 의 열린 점으로 적고 지금 어느 글도 넓히지 않는다. (b) 성공 경로 줄의 out 저장 지역 변수 추출 빈틈은 그대로 둔다(이 빌드에서 `include/` 를 바꾸지 않음; 바꾸면 모든 바이너리가 다시 링크됨). 성공 경로 줄은 그 까닭과 함께 missing 이고, 열린 점이다. (c) 저장된 이름 버퍼: scalar-array FIELD_STATUS 를 complete 로 할 수 있는지 표지마다 scratch 출력에서 정하고, 하나라도 건전하게 지울 수 없으면 줄은 not complete 와 그 표지를 말하며 설계 행을 위해 아무것도 느슨하게 하지 않는다. (d) 바뀌는 golden 은 선언된 빌드에서 커밋하고, 선언은 완전한 소스의 scratch 점검 빌드에서 확인한 첫 ctest 실패를 이름으로 적는다. (e) C1 빌드의 둘째 주소 전달 호출 자리와 'other' 열린 까닭 부류는 빌드 단계의 측정에서 맞으면 세고, 아니면 열린 점이다. — (a)–(d)는 kz-static 의 것이고 이 빌드에서는 해당 없음; (d) 의 방법(첫 ctest 실패를 scratch 점검 빌드에서 확인)은 아래 (마)에 그대로 쓴다. (e)는 §22 가 처리했다.

**(나) 소스.** 형식 덧붙임 `kairos/docs/event_semantics.md` 13.5(1.5 의 바이트 배치·종류 번호·적재 객체를 찾는 방법·convert·연결 점검의 객체 대응)와 13.6(두 플러그인의 `kzs-sites 4` 읽기)을 코드보다 먼저 `2ea634f` 로 커밋했다. 코드는 `c0a87fa`(런타임 `kzrt 0.5.0`, 복호기, `kz-rt-dump`, `kz convert`, `env/tools/chain_check.py`, kzgcc 0.4, kzllvm 0.2, 시험)와 `139330e`(시험 하나의 include 차례; scratch 점검 빌드가 `-Werror` 로 멈춘 것)다. `kairos/include/`, `kairos/cmake/`, 최상위 `CMakeLists.txt` 는 바뀌지 않았다(`git diff fa31655 HEAD` 빈 출력). 적재 객체를 찾는 방법은 선언 전에 정해 시험했다: 등록 호출의 인자 주소(플러그인이 그 단위에 넣는 32 바이트 상수)가 든 객체를 적재기의 프로그램 머리 훑기로 찾는 더한 호출 `kz_rt_objects` 와 POSIX 쪽 `kz_rt_posix_objects` 이고, 플러그인 ABI 와 hook ABI 는 그대로다. scratch 런타임 단독 빌드에서 `rt_objects`·`rt_objects_readelf` 가 시험 실행 파일·build-id 가 있는 시험 모듈·없는 모듈·어느 객체에도 없는 주소를 `readelf -n` 과 맞춰 통과했다(`scratch/plugin_and_runtime_trials.txt`: 런타임 시험 71 통과).

**(다) 정체성이 바뀌는 도구.** `kz`(`fd40bc4d…`; `--identity` 의 `tool_source` 가 `runtime/`·`lib/trace`·`tools/kz` 를 덮으므로 바뀜), `kz-rt-dump`(`40e23d4d…`), `kz-instr-clang`(`36becd04…`; 자기 소스와 manifest 파서 소스는 그대로이고 다시 링크만 됨: 같은 바이트일 수 있음 [I]). 플러그인은 새 디렉터리의 새 파일이다: kzgcc 0.4 `/home/user/work/instr/kzgcc-0.4/plugin-build/kzgcc.so`, kzllvm 0.2 `/home/user/work/instr/kzllvm-0.2/plugin-build/kzllvm.so`. 옛 플러그인 둘(`56a96b79`, `27a49a6a`)은 건드리지 않는다. **바뀌지 않아야 하는 것**(빌드 전 sha256, `prebuild/not_relink.sha256`, 61): kz-static `6f40dd60…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz_lint `3d4d5a8f…` 를 포함한 링크 대상 51(`prebuild/not_relink_paths.txt`)과 명령으로 만드는 시험 프로그램 10(`prebuild/not_relink_other_paths.txt`). `CMakeCache.txt` 도 그대로(`prebuild/cmakecache.sha256`).

**(라) 다시 링크되는 것.** MC 의 dry run(`make -n all`, 감시 `guard/d01_make_dryrun.log`, `prebuild/make_dryrun.log`: 다시 컴파일되는 목적 파일 36, 링크 스크립트 30)에, MC 의 `link.txt` 가운데 `libkzrt*.a`·`libkz-trace.a`·`libkz-hold.a`·`libkz-loader.a`·`libkz-plan.a` 를 이름 대는 것을 더해 61 이다(`prebuild/relink_expected.txt`, 빌드 전 sha256 `prebuild/relink_expected.sha256`; `kz-instr-clang`·`test_static_plan` 포함). 여기에 kz-instr-clang 을 쓰는 명령이 만드는 `instrument/clang/transform_test/{instr,orig}` 둘. dry run 은 다시 구성(configure)을 하지 않으므로 kz 의 정체성 값이 바뀌어 다시 컴파일되는 `tools/kz` 의 목적 파일은 거기 나오지 않는다: 빌드는 구성을 다시 하고(`CMAKE_CONFIGURE_DEPENDS`) kz 를 모두 다시 컴파일한다 [I]. 새 대상(구성 뒤): `test_rt_objects`, `test_rt_format15`, `librt_objects_mod_id.so`, `librt_objects_mod_noid.so`.

**(마) 첫 ctest(`-V`)에서 실패할 시험: 없음.** 시험 수는 271 에 새 시험 5(`rt_objects`, `rt_objects_readelf`, `rt_objects_noreader`, `rt_format15`, `rt_format15_dump`)를 더한 276 이다. 근거: (1) scratch 점검 빌드(완전한 소스 `139330e`, Release, LLVM 도구 없이, `KZ_WERROR=ON`; 대상은 다시 링크되는 61 가운데 LLVM 이 없는 59 와 새 대상 넷, `scratch/checkbuild_targets.txt`): 시험 202 가운데 지은 바이너리의 것 112 통과(`scratch/checkbuild_tests_passed_112.txt`, 새 시험 다섯 포함), 실패 10 은 모두 짓지 않은 kz-dump·kz-phi 를 부르는 시험(`scratch/checkbuild_tests_failed_10.txt`), 80 은 바이너리가 없어 돌지 않음. (2) 그 112 밖의 MC 시험 164(`prebuild/mc_tests_not_in_scratch_164.txt`)를 지금의 MC 트리(빌드 전)에서 돌려 164 통과(`test_probe_anchor_reached` 는 앞과 같이 건너뜀; `prebuild/ctest_prebuild_164.log`, 감시 `guard/p01_ctest_prebuild.log`). 그 가운데 이 빌드가 바꾸는 것을 쓰는 것은 다섯이다: `instr_clang_transform`(kz-instr-clang, 새 런타임 라이브러리와 머리, kz-rt-dump), `test_hold_ptrace`·`test_hold_sequence`(런타임 머리), `test_probe_anchor_reached`(건너뜀), `test_static_plan`(다시 링크): 통과할 것이다 [I]. 바뀌는 golden 은 없다. 플러그인 시험: scratch 에서 kzgcc 18 개(새 kz 를 준 `kzgcc_hold_*` 다섯 포함) 통과, kzllvm 8 개(첫 실행에서 `kzllvm_v4` 가 시험 manifest 의 잘못으로 실패했고 manifest 를 고친 뒤 통과; 고친 manifest 가 `c0a87fa` 의 것) 통과.

**(2026-11-19 덧붙임, `builds-05`)** 위 (2)의 '이 빌드가 바꾸는 것을 쓰는 것은 다섯이다' 에서 하나가 빠졌다. 164 의 마지막 줄 `tool_identity_schema_revision`(`prebuild/mc_tests_not_in_scratch_164.txt:164`)은 다시 지은 `kz` 를 부른다(`kairos/tests/unit/CMakeLists.txt:41-46`). scratch 점검 빌드에서는 짓지 않은 kz-dump·kz-phi 때문에 실패한 열 가운데 하나였고(`scratch/checkbuild_tests_failed_10.txt:10`), 빌드 전 MC 트리에서 옛 바이너리로만 돌았다(`../records/closure-notes-20261119/check.txt` E). 첫 실행 전에 새 소스로 돌린 적이 없는 시험은 이것을 더해 여섯이다. 결과는 선언대로였다(276 시험 통과, `test_probe_anchor_reached` 는 건너뜀; §23.3). [§26.1]

**(바) 출력 종류별 허용 차이.**
1. 남긴 부팅 둘의 dump(형식 1.4)를 옛 도구와 새 도구로 다시 읽은 것(`scripts/regress.sh` → `obs1/rtb_regress_old`, `obs1/rtb_regress_new`; 비교 `scripts/compare_regress.py`): `rc.txt` 의 첫 줄(도구와 연결 점검 스크립트의 sha256); container 글의 `tool_sha256=`·`artifact_sha256=` 줄과 container 바이트의 오프셋 120..155(도구 sha256 과 머리 crc); convert 로그의 `wrote … artifact_sha256` 줄과 **새로 더한 줄 하나** `build-id records: none (kzrt-dump 1.4)`; 연결 점검에서 옛 `same_build` 줄 하나가 `same_build_object` 줄들(모두 `unconfirmed(no build-id in dump)`)과 요약 `same_build unconfirmed(no build-id in dump)` 줄로 바뀌는 것(그 밖의 줄, `chain` 판정 줄 포함, 같음). `kz-rt-dump` 의 글(manifest 를 준 것과 안 준 것, 두 부팅과 32·64 비트 ABI 점검 dump 둘)은 **같은 바이트**다.
2. 판 1–3 manifest 의 묶음(`scripts/regress_manifest.sh` 를 옛 플러그인 `27a49a6a` 와 새 kzgcc 0.4 로 → `obs1/rtb_regress_manifest_old`·`_new`; 비교 `scripts/compare_manifest.py`): 판 1 `kzgcc_target.kzs`, 판 2 `kzgcc_shapes.kzs`, 판 3 `kzgcc_owner.kzs`, 그리고 첫 관측 경로의 C1 계획(`obs1/plangen/c1_pre_tx/plan.kzs`, 판 3)을 아래 (사) 5 의 열두 단위에: 목적 파일은 같은 바이트, binding 파일은 2 번째 줄(플러그인 판 글과 dumpbase)만 다름. scratch 에서 이미 그렇게 나왔다(`scratch/regress_manifest_trial.txt`; 증거가 아님).
3. 그 밖의 출력은 없다(이 빌드는 계획·추출·relations 를 다시 만들지 않는다).

**(사) 끝 점검.** 1. MC ctest 의 `rt_objects`·`rt_objects_readelf`: dump 의 build-id = `readelf -n` (시험 실행 파일, 시험 모듈; note 없는 모듈은 `absent`, 어느 객체에도 없는 주소는 `unreadable`). 2. 부번호 4 dump 를 그대로 읽음(위 (바) 1). 3. 합성 1.5 dump(build-id 와 토큰 레코드 셋, 잘못된 크기 둘의 거부): `rt_format15`·`rt_format15_dump`. 4. 판 1–3 manifest 가 같은 바이트로 묶임(위 (바) 2). 5. **묶음 점검**: kz-static 단계의 두 C1 계획(`obs1/cz_plans_v4_ns2/c1_pre_tx`, `c1_post_tx`)에서 sidecar 의 `control-state=planned` 행(경로 A 대조 선택; 설계 행을 담음)의 site 가 든 단위**만**, 남긴 compile 명령(`/home/user/work/procrace/c1_build/{pre,post}/1df262a/build/native/default/compile_commands.json`)으로, 새 kzgcc 0.4 를 붙여 감시 아래 컴파일하고(목적 파일은 출력 디렉터리로; 참조 빌드 트리에는 아무것도 쓰지 않음), 그 행들의 site 마다 binding 상태를, 묶이지 않은 site 는 까닭과 함께 적는다(`scripts/bind_check.py` → `obs1/rtb_bind_c1_pre_tx`, `obs1/rtb_bind_c1_post_tx`). 파생 빌드가 아니다; 전체 binding 보고는 주기 준비 단계의 몫이다. GCC 의 호출 열과 `col=`·`nth=` 의 대응을 여기서 잰다(`# v4 calls` 줄). 6. 두 플러그인의 ctest(새 디렉터리, kzgcc 는 `KZGCC_KZ` = 새 MC kz). 7. 빌드 뒤 점검: 누적 바이트 동일성 목록(`ledger_check.sh`), `tools_check.sh --deep`, 전체 ctest(첫 실행 `-V`), lint(ctest 의 lint 시험), `env` 의 단위 점검 둘(`env/tools/tests/run_unit.sh` 18, `env/redesign/step2a_tests_unit.sh`).

**(아) 순서.** 사본(kz, kz-rt-dump, kz-instr-clang; xz, 왕복 확인, 등록부 줄; `/home/user/work/preserve/tools-20261119-c/`) → 옛 도구의 다시 읽기(`rtb_regress_old`)와 옛 플러그인의 판 1–3 묶음(`rtb_regress_manifest_old`) → MC 빌드(`make all`, 감시 아래) → 빌드 뒤 점검 → 새 도구의 다시 읽기와 비교 → 플러그인 두 빌드(새 디렉터리)와 그 ctest → 새 플러그인의 판 1–3 묶음과 비교 → 묶음 점검 → 등록부·원장 줄 → 결과. 명령마다 rc 를 본 뒤 다음으로 간다. 선언 밖의 차이가 나오면 그 단계에서 멈추고 덧붙임을 커밋한 뒤에만 잇는다.

**(자) 공간.** 시작 678,703,104 B(바닥 314,572,800). 먼저 이 단계의 scratch(런타임·플러그인·점검 빌드, 약 140 MB)를 지운다. 필요량: 사본 약 40 MB(앞 사본 크기에서: kz 18 MB, kz-instr-clang 21 MB, kz-rt-dump 1 MB) [I]; MC 빌드 감시 필요량 250 MB(목적 파일 약 60 과 바이너리 63 을 제자리에서 다시 씀; 가장 큰 kz 77 MB) [I]; 플러그인 빌드 둘 약 30 MB(scratch 의 12·14 MB 에서); 다시 읽기 출력 둘 약 2 MB, 판 1–3 묶음 둘 약 2 MB, 묶음 점검 둘 약 4 MB; 기록 수 MB. 바닥 위에서 맞는다. 맞지 않으면 rule 3 의 압축 허용만 쓴다.

#### 23.0.1 덧붙임 1 (묶음 점검의 site 집합; 다시 하는 묶음 점검보다 먼저 커밋함)

첫 묶음 점검(`scripts/bind_check.py` → `obs1/rtb_bind_c1_pre_tx`, `_post_tx`)은 행의 site 를 `row=` 열쇠가 그 행을 이름 대는 site 로만 잡았다. 그러나 계획 규칙 개정 5 의 생성 쪽 종류 값·되돌림·삭제·지우기 site 는 `row=` 를 갖지 않고, 그 행의 `need … state=planned sites=…` 문이 이름 댄다(예: c1_pre_tx 의 `s0256.object-rollback`, `s0264.object-clear` 는 `osapi-idmap.c` 에 있어 첫 점검에서 컴파일되지 않았다). 그래서 (사) 5 의 "행의 site" 를 다음으로 바로잡는다: `row=` 가 그 행을 이름 대는 site, **또는** 그 행을 subject 로 하는 계획된 need 가 `sites=` 로 이름 대는 site. 그 행들의 `gap` 문도 보고에 적는다. 다시 하는 점검은 `scripts/bind_check_2.py` 로 새 디렉터리 `obs1/rtb_bind2_c1_pre_tx`, `obs1/rtb_bind2_c1_post_tx` 에 낸다(첫 점검의 출력은 그대로 남긴다). 컴파일하는 단위가 늘 뿐 도구·허용 차이는 그대로다. 공간: 출력 둘 약 1 MB 씩.

### 23.1 바뀐 것 [F]

- **런타임 `kzrt 0.5.0`, dump 1.5** (`runtime/`): BUILDID 레코드(주 실행 파일 하나, 모듈 등록의 적재 객체마다 하나), LOSS `no-build-id`(15)·`object-table`(16), 생성 토큰의 예약 레코드 셋(읽는 쪽만). 더한 호출 `kz_rt_objects` 와 POSIX 쪽 `kz_rt_posix_objects`(시작 객체가 attach·init 뒤에 부름). 값은 store 에 있으므로 되살린 dump 도 같은 레코드를 낸다. 핵심은 여전히 라이브러리를 부르지 않는다(`rt_no_library_symbols` 통과).
- **복호기·`kz-rt-dump`**: 1.0–1.5 를 읽고, `buildid` 줄과 `tok-create`·`tok-return`·`tok-start` 줄을 13.1·13.2 의 글로 찍는다. 토큰 레코드는 actor 의 aseq 점검에 든다.
- **`kz convert`**: BUILDID 마다 NBUILD `link=build-id` 레코드(13.5 (바)); `no-build-id`·`object-table` 은 중립 손실로 옮기지 않음; 출력 줄 `build-id records: …`. `include/` 는 그대로이므로 이미 있는 칸(`build_id`, `path`, `reason`, `index`)만 채운다.
- **`env/tools/chain_check.py`**: `same_build_object` 줄과 `same_build` 요약 줄(13.4, 13.5 (사)); 사슬 판정에 넣지 않는다. 단위 점검 18 을 더했다.
- **kzgcc 0.4·kzllvm 0.2**: 13.6 대로. 판 1–3 manifest 의 경로는 손대지 않았다.

### 23.2 빌드와 정체성 [M]

`make -j8 all` rc 0(감시 `guard/b05_make_all.log`, dip 21,458,944 B; `make_all.log` 경고 0). `relink_result.txt`: 바뀌지 않아야 하는 61 이 모두 같은 바이트, `CMakeCache.txt` 같음, 다시 링크된다고 적은 63 가운데 33 이 바뀌고 30 이 같은 바이트(선언대로 둘 다 허용), 새 대상 넷이 생김. 도구(`post_mc.txt`): kz `fd40bc4d…` → `94cba72b6da18361b50989893dd2c5a4c5146c17ccb982d5462a0426c59c60e8`(`--identity` `tool_sha256` `80064cc3…` → `2a7bbe09…`, `tool_source` `500185a6…` → `8ded611a…`, `core_source` `568e5061…` 그대로); kz-rt-dump `40e23d4d…` → `d83fdde9b5009a3c0c0508125c08b4a424216065906fe2622cd384fd8e6645dc`; kz-instr-clang `36becd04…` → `4041f6b43abee6ad172f6437879f5c2ae838e482801b28a3231fe139b62ba9bd`(다시 링크로 바뀜; 선언이 허용한 쪽). kz-static `6f40dd60…`, kz-dump `32a96ae7…`, kz-phi `df79fb82…`, kz_lint `3d4d5a8f…` 그대로. 사본(`tool_copy/ROUNDTRIP.txt`, `/home/user/work/preserve/tools-20261119-c/`): 셋 모두 왕복 같음. 플러그인(새 디렉터리): kzgcc 0.4 `985fc1025de287bfa9cd7466407ddb33a03b6800d728dbaa75f00d4630a707a2`(`/home/user/work/instr/kzgcc-0.4/plugin-build`; scratch 빌드와 같은 바이트), kzllvm 0.2 `1900207f48576199e3f7e3fad4a94ab3664e7f7f9c6a5b6242d3b79c718ac665`(`/home/user/work/instr/kzllvm-0.2/plugin-build`); 빌드 로그 경고 0. 옛 플러그인 둘은 건드리지 않았다(`tools_check_end.txt`).

### 23.3 시험 [M]

- 첫 ctest(`-V`, `ctest_first.log`): 276 시험 100% 통과, `test_probe_anchor_reached` 는 앞과 같이 건너뜀. 선언한 실패 없음과 같다. golden 을 바꾸지 않았으므로 이 실행이 전체 ctest 다(lint 시험 포함).
- 새 시험: `rt_objects`(BUILDID 다섯: main, 실행 파일 안의 모듈 3, 적재한 모듈 7 과 8, 어느 객체에도 없는 주소의 모듈 9), `rt_objects_readelf`(dump 의 build-id = `readelf -n`: main 과 모듈 3 `e840b203…`, 모듈 7 `03cb9ef0…`; 모듈 8 은 readelf 에도 없음), `rt_objects_noreader`(읽는 쪽이 없으면 BUILDID 없음, 모듈 3·9 의 `no-build-id`), `rt_format15`(합성 1.5 dump 의 BUILDID 셋·토큰 레코드 셋을 읽고, 잘못된 크기 둘을 거부), `rt_format15_dump`(그 글 줄) — 모두 `ctest_first.log` 에서 통과.
- 플러그인: kzgcc 18/18(`plugins/kzgcc_ctest.log`; `kzgcc_v4_O0`·`_O2` 와 새 MC kz 로 도는 `kzgcc_hold_*` 다섯 포함), kzllvm 8/8(`plugins/kzllvm_ctest.log`, `kzllvm_v4` 포함).
- `env` 단위 점검: `unit_tools.txt` 18/18, `unit_step2a.txt` 7/7.
- 누적 목록과 등록부: `ledger_check_after_build.txt` 158 행 실패 0, `ledger_check_end_2.txt` 166 행 실패 0; `tools_check_after_build.txt` 54 행, `tools_check_end.txt` 56 행, 모두 `--deep` 실패 0.

### 23.4 끝 점검 [M]

1. **build-id = readelf**: 위 23.3(`rt_objects_readelf`).
2. **부번호 4 dump 를 그대로 읽음**(`regress/compare_regress.txt`, rc 0): 두 부팅(C1 `tx.kzrt`, C5 `cpu1.kzrt`, 둘 다 1.4)과 32·64 비트 ABI dump 의 `kz-rt-dump` 글 여섯 파일이 같은 바이트; convert 로그는 선언한 한 줄 `build-id records: none (kzrt-dump 1.4)` 만 더해짐; container 는 도구 sha256·머리 crc 36 바이트만 다름; 연결 점검은 옛 `same_build` 한 줄이 객체 줄 7 과 요약 `unconfirmed(no build-id in dump)` 로 바뀌고 나머지 18 줄(사슬 판정 `confirmed` 포함) 같음.
3. **합성 1.5 dump**: 위 23.3(`rt_format15`, `rt_format15_dump`).
4. **판 1–3 manifest 가 같은 바이트로 묶임**(`regress/compare_manifest.txt`, rc 0): 옛 kzgcc 0.3 `27a49a6a` 와 새 kzgcc 0.4 에서 넷 묶음(판 1·2·3 의 시험 단위와 판 3 C1 계획의 열두 단위)의 목적 파일 15 가 모두 같은 바이트, binding 파일 11 이 판 줄만 다름.
5. **묶음 점검**(덧붙임 23.0.1 의 site 집합, `bind/report2_c1_pre_tx.txt`, `bind/report2_c1_post_tx.txt`; 출력 `obs1/rtb_bind2_*`): 두 계획 모두 sidecar 의 `control-state=planned` 행 다섯(A20, A21, A22, A26, A63)이다.
   - c1_pre_tx: 단위 13 컴파일, 실패 0. 행의 site 52(`row=` 42, need 가 이름 댄 것 10)가 모두 묶임(bound 45, inlined 7). 규칙별: use-call 9, creator-call 8, effect-call 7, handling-chain 5(+ use-call 과 겹친 1), key-text 4(+ kind-value 와 겹친 1), kind-value 3, context-frame 3(+ handling-chain 과 겹친 1), object-delete 4, object-rollback 4, object-clear 2. 같은 단위의 다른 site 3 도 묶임.
   - c1_post_tx: 단위 12, 실패 0. 행의 site 49 가운데 48 묶임(bound 41, inlined 7), **묶이지 않은 것 하나**: site 297 `s0297.wait-exit`(행 A21 의 대기 루프 나가는 문, `cf_cfdp.c:1008` 의 `break`, place=line-start) — 까닭 `no statement on line 1008 of the function`: GIMPLE 에서 `break` 는 그 줄의 문으로 남지 않는다(가지로만 남음) [I: 까닭의 해석; 사실은 plugin 의 binding 줄]. 고친 빌드의 재시도 루프를 나가는 것은 그 행의 판별 요소이므로, 그 사건은 이 site 로는 기록되지 않는다(열린 점).
   - 두 계획의 `gap` 문은 행 A63 의 하나씩(`key text of OS_mv parameter 0 …`, `record_insufficient(key text)`).
   - **GCC 의 호출 열과 `col=`·`nth=`**(`# v4 calls` 줄의 합): c1_pre_tx 호출 site 47, c1_post_tx 43 이 모두 그 줄의 유일한 호출이었고(`line-unique`), GCC 의 열과 차례가 `col=`·`nth=` 와 다른 것은 0. 한 줄의 둘 이상의 호출을 열로 가르는 경우는 이 단위들에 없었다(단위 시험 `kzgcc_v4_*` 만이 그것과, 중첩 호출에서 GCC 의 열이 바깥 호출의 열이 되는 것을 보인다).
   - 첫 점검(`row=` site 만, `bind/report_c1_*.txt`, `obs1/rtb_bind_*`): c1_pre_tx 12 단위 행 site 42 모두 묶임, c1_post_tx 11 단위 39 가운데 38(같은 site 297 이 빠짐). 덧붙임으로 바로잡은 까닭은 23.0.1.
6. **플러그인 ctest**: 23.3.
7. **빌드 뒤 점검**: 23.2·23.3.

### 23.5 공간 [M]

시작 678,703,104 B(`free_start.txt`), scratch 를 지운 뒤 818,524,160 B, 끝 729,747,456 B(`free_end.txt`). 감시 기록 27, 모두 `completed`; 가장 큰 dip 39,907,328 B(사본, `guard/b01_copy.log`), MC 빌드 21,458,944 B. 남긴 것: 사본 39,454,236 B, 플러그인 디렉터리 둘 10,653,237 B 와 11,483,888 B, 출력 `obs1/rtb_*` 여덟 5,287,921 B, 기록 수 MB. 압축 허용은 쓰지 않았다. scratch 는 이 단계가 만든 것만 지웠다(scratch 점검 빌드, 런타임·플러그인 scratch 빌드, 시험 출력).

**(2026-11-19 덧붙임, `builds-06`)** '모두 `completed`' 는 하한의 근거가 아니다(§12.11 (마)의 덧붙임). 근거는 감시 기록 27 의 가장 낮은 값 679,084,032 B 와, 끝 값이 하한 아래인 기록이 0 이라는 것이다(`../records/closure-notes-20261119/check.txt` A). [§26.1]

### 23.6 벗어난 것

1. **"두 플러그인"의 읽기.** 계획의 두 플러그인을 GCC 플러그인(kzgcc)과 LLVM pass 플러그인(kzllvm)으로 읽었다(둘 다 같은 manifest 파서를 쓴다). 등록부의 옛 kzgcc 둘(0.2, 0.3)을 뜻한 것이라면 kzgcc 0.4 하나가 둘을 대신한다. kzllvm 0.2 는 판 1 의 꼴만 묶고 판 4 의 `place=`·identity·spawn site 를 `unsupported` 로 센다(13.6); C1 의 묶음 점검은 kzgcc 로만 했다(kzllvm 의 묶음 점검은 clang 의 compile 명령이 필요하다).
2. **kzllvm 0.1 과의 판 1–3 바이트 동일성은 재지 않았다.** kzllvm 0.1 바이너리는 등록부에 없다(지어 둔 적이 없음). 판 1–3 에서 0.2 의 경로가 0.1 과 같다는 것은 코드(판 4 에서만 갈리는 가지)와 단위 시험 일곱의 통과에 기댄다 [I].
3. **묶음 점검의 site 집합을 한 번 바로잡았다**(덧붙임 23.0.1, 다시 한 점검보다 먼저 커밋). 첫 출력도 남겼다.
4. **scratch 점검 빌드는 LLVM 도구 없이**(Release) 지었다: `kz-instr-clang`·`test_static_plan` 과 LLVM 이 필요한 시험은 scratch 에 없고, 빌드 전 MC 트리의 시험 164 와 빌드 뒤 첫 ctest 로 보았다.
5. 등록부의 kz-instr-clang 은 같은 바이트일 수 있다고 적었으나 바뀌었다(`4041f6b4…`): 다시 컴파일된 manifest 파서의 목적 파일이 새 런타임 머리로 바뀐 것으로 본다 [I]. 선언은 바뀌는 것도 허용했다.
6. **(2026-11-19 덧붙임, `builds-05`)** 첫 ctest 의 기대가 빌드 전 옛 바이너리의 실행에만 기댄 시험은 여섯이다: 23.0 (마)가 이름 댄 다섯과, 다시 지은 kz 를 부르는 `tool_identity_schema_revision`(23.0 (마)의 덧붙임). 건너뛴 `test_probe_anchor_reached` 밖에 모두 통과했다(23.3). 뒤 선언은 다시 링크되는 대상의 `TARGET_FILE` 인자에서 시험 집합을 낸다. [§26.1]

### 23.7 열린 점

1. 고친 빌드(c1_post_tx)의 행 A21 의 대기 루프 나가는 문(`s0297.wait-exit`, `cf_cfdp.c:1008` 의 `break`)은 kzgcc 0.4 의 `line-start` 로 묶이지 않는다. 그 사건이 필요하면 계획 규칙(나가는 문 대신 루프 뒤 첫 문, 또는 그 가지의 끝)이나 플러그인(GIMPLE 가지의 위치)의 다음 개정이 정한다.
2. 생성 토큰은 예약만 했다: 쓰는 쪽(런타임의 생성 래퍼 `kz_rt_posix_create`)과 결합(`kz convert`)은 없다. 그래서 C1 계획의 `spawn` 문과 `decoder=spawn-token` 은 묶음 뒤에도 `not-applied` 이고, `link_unlinked(no creation token)` 은 그대로다.
3. `place=load` site 는 kzgcc 0.4 가 묶지 않는다. 다섯 빌드의 `cz_plans_v4_ns2` 계획에는 하나도 없다(`grep -c place=load` 0); 그것을 계획하는 경로 C 읽기 규칙이 site 를 내면 그때 묶을 꼴이 필요하다.
4. 실제 cFS 파생 빌드의 모듈(`dlopen` 한 앱 .so)에서 BUILDID 가 나오는지는 부팅 전이라 재지 않았다 [미확인]; 시험 모듈로만 보였다. 파생 빌드는 주기 준비 단계에서 새 런타임 트리(0.5.0)와 kzgcc 0.4 로 다시 짓는다.
5. lead 의 판단 (a)–(c)의 열린 점은 그대로(§22.9).
6. 설명 스크립트 개정 4 는 이 빌드의 끝 점검 뒤에 동결한다(§12.10 4 항 (6)): 읽는 쪽은 `buildid` 줄과 `same_build_object` 줄을 13 절의 글로 읽는다.

## 24. 설명 스크립트 개정 4 와 그 요약, 동결 (2026-11-19)

§12.10 4 항 (6)의 단계다: 명세(`plan/c1_closure_explain_rev4_spec_20261118.md`)와 판정 규칙 8.13 대로 개정 4 를 짓고, 2026-11-16 입력의 회귀, 두 C1 빌드의 정적 출력과 런타임 단계의 시험 dump 에 대한 예행 뒤에 동결했다. 도구 빌드는 없고(MC 바이너리를 바꾸지 않음), cFS 를 부팅하지 않았으며 push 하지 않았다. 기록: `../records/closure-explain-rev4-20261119/`(README). 출력: `/home/user/work/redesign/obs1/xr4_*`(새 디렉터리 열둘).

### 24.0 lead 의 판단 (2026-11-19, §17 의 열린 점; 이 단계에서 본 것)

(a) 판정 규칙 8.13 17 항 조건 (6)은 글대로 적용한다(필드별 규칙과 형 수준 규칙 둘 다). 도구를 바꾸지 않는다. 커밋된 평가 갈래 `Re` 가 그것을 덮는지는 이 단계의 몫이다 — 아래 24.4 (라). (b) 성공 경로 줄의 out 저장 지역 변수 추출 빈틈은 그대로이고 그 줄은 missing 이다: 기초 4 는 그 까닭으로 빠진다(24.4). (c) 저장된 이름 버퍼는 어느 것도 complete 가 아니다: 조건 (다)는 `Rf` 가 아니면 빠진다. (d) golden 은 이 단계와 관계없다(도구 빌드 없음). (e) 둘째 주소 넘김 호출 자리와 `other` 부류는 §22 가 다뤘고 이 단계는 건드리지 않았다.

### 24.1 바뀐 것 [F]

- 명세: 10 절(`55edd4b`, 코드보다 먼저)과 10.7(`6c459f7`, 코드 커밋보다 먼저). **고침 하나**: 평가 갈래를 명령줄 인자(`--data-branch`·`--readings`)가 아니라 규칙으로 만든 입력 파일 `kz-eval-branches 1`(`--branches`, `explain_branches.py` 가 만듦; 행 번호·키·기대 판정 없음)로 받는다. 그 밖은 실제 출력에 맞춘 읽기(개정 14 줄의 자리, 쌍 파일의 `edge` 줄과 꼬리, 쌍 줄 찾기, `bind=` 로 정한 사건의 객체, dump 1.5 줄, 문맥 틀, 지역 holder, `Re` 와 필드 저장값 규칙, (다)의 두 집합, 후보 목록이 없을 때)와 코드를 쓰며 정한 것(서식 검사 16 과 읽기 갈래, 자기 sha256, 프로세스 첫 스레드의 생성과 기초 2, 사용의 수, 순서 근거는 연결된 쌍에서만, 삭제의 성공 값, 경로 B·C 의 writer 호출, 요약의 미확정 종류)이다.
- 코드(`cb9a4d4`, 고침 `e4b2b98`): `env/obs1/explain_read.py`(읽는 부분: 입력·형식 점검·중간 글 `kz-explain-normal 1`), `env/obs1/explain_judge.py`(판정 부분: 집계 전 사용 표 `uses.tsv`, 생성과 수명 끝 사건, (종류 값, 이름 글)로 맞춘 세대와 이름 상태, 이름 결합 쌍의 열두 항목과 하위 사유, 모든 항목·사용 조건 요소에 거는 자료 상태 규칙, 열린 세대에만 하는 인스턴스 연결, 8.3 의 규칙으로만 내는 refuted, 쌍 파일에서만 읽는 순서(사실·도구 sha256 을 relations 머리와 sidecar 와 대조, 대신하는 값 없음, `order_basis_unknown`), 쌍 종류, 중립 필드, 서식 검사 1–17), `env/obs1/explain.py`(구동부), `explain_branches.py`, `explain_summary.py`(요약), `explain_expect.py`(기대 대조, 기록을 바꾸지 않음). 개정 3 은 바이트 그대로 `env/obs1/explain_rev3.py`(`acfd4271…`). `env/tools/tests/run_unit.sh`: 점검 15 를 개정 3 사본으로, 점검 19(개정 4 fixture)를 더함.
- `e4b2b98` 의 고침은 정적 예행에서 드러났다: 실제 대안 줄에서 필드 저장값 규칙의 전제 글이 600 글자 뒤에 있어 `Re` 의 판단이 그것을 읽지 못했고, holder 줄이 없는 행(A63)이 "개정 14 줄이 없음"이라고 잘못 적혔다. 첫 실행 출력(`_2` 없는 디렉터리)은 남겼다.

### 24.2 시험 [M]

- fixture(`out/fixtures.txt`): **127 점검 모두 통과**. 합성 이름만 쓴다(설계 행이 아닌 꼴: 모든 항목이 서서 `observed` 에 닿는 경로 A 행, 경로 B 행, 지역 holder 등). 항목 열둘이 하나씩 빠진 것, 기초 1 의 refused·옛 dump, 기초 3 의 널 키·다른 기원, 기초 4 의 두 하위 사유와 실패한 생성, 기초 6 의 겹침과 모순(`contradicted`), (다)의 버퍼·종류에 묶이지 않은 writer·FIELD_STATUS, (라)의 종류 값·계획되지 않음, (마)의 완전하지 않음·묶이지 않음·표 초기화·되돌림·`reentry=possible`·빈 자리 지우기(보인 것과 보이지 않은 것), refuted 쌍·기록과 세 모자람, `differs_from_use_key`, bound 둘, 찾은 사용의 인스턴스 연결(열린 세대, 삭제된 세대, 수명 끝 불완전), 부재 뒤 찾음(바탕이 다름), 키 글 둘, 쌍 종류 넷, 쌍 파일(다른 사실·다른 도구·쌍 없음·꼬리 어긋남·`same_thread`·`hard_before` 와 `unmet`), 문맥 둘과 틀 없는 사용, (i)·(iii)의 각 상태, 필드 저장값 규칙 표지, 읽기 `Ra`–`Rf` 하나씩, 부분 인정, 인정 파일 점검 셋(모두 rc 2), 후보 목록 없음, `tok-*` 줄, 실행 없음, 판정 부분이 중간 글·어휘·자기 파일 말고 열면 실패하는 시험과 그 시험이 실패를 잡는지, 서식 검사 9–17 을 하나씩 깬 기록 12(모두 `--check` rc 4), 요약과 기대 대조(2 match, 1 mismatch; 기록 불변).
- 단위 점검: `out/unit_tools_3.txt` 19/19(점검 15 개정 3 22/22, 점검 19 127/127), `out/unit_step2a_2.txt` 7/7. 첫 실행 `out/unit_tools.txt` 는 18/19 였다: fixture 의 열기 시험이 어휘 파일 경로가 절대 경로일 때 다른 자리를 바꿨다(시험 코드의 결함). 고친 뒤 `unit_tools_2.txt` 19/19.
- 원장 `out/ledger_check.txt` 178 행 실패 0(새 행 `OBS_xr4_*` 12), 등록부 `out/tools_check_deep.txt` 56 행 실패 0(이 단계는 도구를 바꾸지 않음).

### 24.3 회귀: 2026-11-16 입력, 규칙을 정하기 전에 본 기록 [M]

- 기준: 개정 3 을 같은 입력(`records/obs1-20261116-reviewfix/inputs.txt` 의 15 파일, sha256 모두 그대로)에 먼저 돌림 → 45 기록, 물림 0(`out/regress_rev3.log`, `obs1/xr4_regress_rev3`). 남긴 개정 2 의 `index.tsv` 와 1–12 칸이 45/45 같다(`out/rev2_vs_rev3_index.txt`).
- 개정 4(동결본) 를 같은 입력과 D1 갈래 여덟에(`obs1/xr4_regress_rev4_2`, 기록마다 `record_label = seen before the rules`): 갈래마다 45 기록, 물림 0. **축 상태와 판정 값이 여덟 갈래 모두 45/45 같다**(`out/compare_r3_r4_2_<갈래>.txt`): unlinked/partial/events_only/undetermined 22, unlinked/partial/record_insufficient/undetermined 23.
- 다른 칸(필드 경로 부류별 목록, `out/compare_r3_r4_2_D1-R0.txt`)은 선언한 것이다: 더한 칸(`eval_branch`, `data_branch`, `uses`·`uses_reached`·`uses_decided`, `same_build`, `order_ground`, `other_order_seen_in_boots`, `pair_kind`, `events`, `link.name_binding`, `use_cond.branch_i`·`branch_iii`, `order.pairs`, 사건 참조의 `object`·`src`, `script` 의 두 sha256, `record_label`); 이름 바뀜(프로세스 쪽 앱 부류 `creator` → `process`, 쌍 종류 `creator-app` → `process-app`, 10.5); `use_cond.caller_dependence` 가 처리 줄의 종료 호출 대신 (i)·(iii)에서 옴(8.13-15); `order.per_consumer` 는 연결된 생성에서만(8.13-14, 같은 키 글의 게시 사건은 더 이상 w 를 정하지 않음); `link.instance_evidence` 의 같은 글 증거는 쌍 목록으로 옮김.
- 그 입력에는 쌍 파일·개정 14 줄·새 sidecar 줄·`buildid` 줄이 없어 새 항목은 그 입력을 이름 대며 빠진다: 사용 표 177 사용(`D1-R0/uses.tsv`) 가운데 경로 A 사용 1(A21)의 이름 상태는 `undecided(the use's kind value is not recorded)`(옛 계획에 종류 값 site 가 없음).

### 24.4 예행 [M]

**(가) 두 C1 빌드의 정적 출력, 실행 없음**(`obs1/xr4_dry_static_<빌드>_2`; 입력: `cz_final_order` 의 relations·keys·pairs, `cz_plans_v4_ns2` 의 계획, 그 계획이 이름 댄 이름 공간 파일 다섯, `obs1/xr4_dry_inputs` 의 facts 보기와 런타임 단계 묶음 점검의 binding 줄). 두 빌드 모두 rc 0, 갈래 여덟마다 46 기록 물림 0, 입력 점검 refused 0, 읽지 못한 줄 0(`input_checks.txt`; 개정 14 줄·쌍 파일의 꼴과 꼬리·도구 sha256 세 곳 같음·이름 공간 sha256 다섯 모두 ok). 경로 A 의 다섯 행(`out/dry_static_path_a_rows_2.txt`, D1-R0):

| 행 | (i) | (iii) 문맥마다 | `use_cond` |
|---|---|---|---|
| A21 c1_pre_tx | `not_held`: 지우는 채움(`cf_cfdp.c:965:5`, reader 는 지운 값을 가른 뒤 씀) | 1 `not_established`(열린 효과, 필드 저장값 규칙이 전제에 기댐), 2 `not_established`(호출자에게 돌아감) | partial |
| A21 c1_post_tx | 같음 | 1 `not_held`(found: retry) | partial |
| A20 (두 빌드) | `not_held`: 같은 함수의 생성 out 저장 | 1 `not_held`(같은 이름 공간·partition 의 자기 생성) | partial |
| A22, A26 (두 빌드) | `not_established`(지역 holder) | 1 `not_established` | partial |
| A63 (두 빌드) | `not_established`(holder 줄 없음) | `-` `not_established`(부재 값이 평가되지 않음) | partial |

평가 절차 6 절의 F5(고친 빌드의 행과 탐침 행의 (iii)이 `held` 가 아님, 탐침 행의 (i)이 `held` 가 아님)는 정적 줄 수준에서 선다 [I: 부팅 기록이 없으므로 사용의 문맥 배정은 아직 없음]. 읽기 갈래: `Rb` 에서 A21 의 (i)은 `not_established`(지우기를 빼도 FIELD_STATUS 가 두 규칙 모두 DISABLED), `Rc` 에서 (c2) present, `Rd` 에서 (c4) present(전제 표지), `Re` 에서 (i)은 그대로 `not_held`(그 holder 는 필드별 규칙도 DISABLED). 어느 갈래에서도 `derived` 는 없다.

**(나) 런타임 단계의 시험 dump**(`obs1/xr4_dry_rt_<이름>_2`; `kz-rt-dump` `d83fdde9` 의 글, 연결 점검은 그 시험 바이너리 셋의 대역 빌드 기록으로). 읽는 부분 rc 0 둘: `rt_objects` 의 `buildid` 다섯(main, module:3·7 present, module:9 unreadable, module:8 absent)과 연결 점검의 `same_build_object` 다섯(confirmed 3, `no build-id note` 2), `rt_format15` 의 `buildid` 셋(module:4 `path_cut=yes`)과 `tok-*` 셋(수만 셈). 판정 부분은 둘 다 rc 3: 그 dump 는 계획의 manifest 를 싣지 않으므로 결합 점검이 거부하고 모든 기록이 `build_mismatch` 다(계약대로; 46 기록 물림 0). 그래서 끝 조건 "예행 rc 0" 은 읽는 부분의 rc 로 본다(24.6).

**(라) lead 의 판단 (a) — `Re` 가 8.13 17 항 (6)을 덮는가.** 커밋된 `Re` 의 글(8.13-5: "필드 단위 완전성 규칙만 선 writer 집합(형 단위 상태는 DISABLED)을 사실로 받는다", 움직이는 요소에 "(c3)(iii)의 저장값 규칙")은 도구가 적는 전제(`premises: (6) the writer set of CF_AppData_t.run_status is complete under both rules (FIELD_STATUS per-field=writer-set-complete type-level=DISABLED)`, `cz_final_order/c1_pre_tx/relations.txt` 의 A21 문맥 1)를 글로 덮는다. 그러나 그 줄은 전제 아래에서 규칙이 정할 호출 결과와 그 결과로 닫히는 효과를 적지 않는다(`[the stored-field rule at CFE_ES_RunLoop 338:12: not decided: (5) or (6) rests on a premise]`). 그래서 판정 부분은 `Re` 의 어느 갈래에서도 그 문맥의 (iii)을 `not_established(the alternatives line gives no call result under the premise (reading Re): …)` 로 둔다(`out/dry_static_path_a_rows_2.txt` 의 D1-Re). 어느 글도 넓히지 않았다. **PI 의 열린 점**: `Re` 가 실제로 움직이려면 도구가 전제 아래의 호출 결과를 내야 한다(도구 빌드가 필요; 글은 그대로).

### 24.5 동결 [M]

`../records/closure-explain-rev4-20261119/freeze.tsv`(부팅 선언이 인용): 읽는 부분 `explain_read.py` `ff0a06252cf74db9862588913384b0dd82989636b2b22e18c7b5c9f1b2d97752`, 판정 부분 `explain_judge.py` `531a6b3e91ba3e574c9263d2eb9307a282dd06f09dcbf9ee8e8f10717afc4587`; 함께 구동부 `af9d4395…`, 갈래 파일 생성 `a722e4af…`, 요약 `17c3a9eb…`, 기대 대조 `442d2e7d…`, fixture `b0bf84ae…`, 개정 3 사본 `acfd4271…`. 저장소의 파일과 8/8 같음(`out/freeze_check_2.txt`). 동결한 코드는 `e4b2b98` 이고, 동결본으로 낸 기록(`_2` 디렉터리)은 모두 그 두 sha256 을 싣는다.

### 24.6 공간 [M]

시작 728,752,128 B(07:50Z), 끝 548,642,816 B(08:33Z), 바닥 314,572,800. 감시 기록 14, 모두 completed; 가장 큰 dip 72,712,192 B(fixture 실행의 scratch, 끝에 지움), 가장 낮은 값 478,576,640 B. 남긴 출력 열둘 합 약 145 MB(`xr4_regress_rev4*` 23 MB 씩, 정적 예행 13 MB 씩, 시험 dump 예행 11–12 MB 씩, 기준 2.5 MB). 압축 허용은 쓰지 않았다. scratch 는 이 단계가 만든 것만 지웠다.

### 24.7 계획과 다르게 한 것

1. **평가 갈래를 입력 파일로**(10.2): 명세 3 절 1 의 인자 대신. 명세를 코드보다 먼저 고쳐 커밋했다.
2. **서식 검사 16 의 읽기**(10.7 1): 글자대로면 전제를 받는 읽기(`Rd`, `Re`)의 `D1` 기록이 모두 물린다(첫 정적 예행 scratch: `D1-Rd` 46/46). 읽기가 이름 댄 전제는 그 갈래에서만 받고 칸 `reading` 을 단다. lead 의 확인이 필요하다.
3. **D2 갈래는 실제 출력에서 돌리지 않았다**: 후보 목록과 인정 파일은 주기 준비 단계가 규칙으로 만들어 커밋할 것이라(평가 절차 4·12 절) 이 단계는 fixture 로만 시험했다. 예행과 회귀는 D1 여덟 갈래다.
4. **시험 dump 예행의 rc**: 판정 부분 rc 3 은 결합 점검의 계약이다(24.4 (나)).
5. **동결 전 고침 한 번**(`e4b2b98`)과 첫 실행 출력·첫 단위 점검·첫 동결 대조(`freeze_check.txt`, 첫 줄을 빠뜨린 7 줄) 를 남겼다.
6. 요약이 두 빌드를 "부팅"의 자리로 읽은 예행 요약(`out/summary_dry_static_2.txt`)은 형식 시험일 뿐이다.

### 24.8 열린 점

1. lead 의 판단 (a): 24.4 (라) — `Re` 의 글은 (6)을 덮지만 입력 줄이 결과를 주지 않아 움직이지 않는다. PI 의 열린 점.
2. 기초 2 는 프로세스의 첫 스레드가 만든 생성에서 글대로 서지 않는다(생성 인자 묶기가 없음; 시간 기준 행의 생성자가 그렇다). 규칙을 바꾸지 않았다. lead·PI 의 판단.
3. 서식 검사 16 의 읽기(24.7 2)의 확인.
4. 후보 목록·인정 파일·기대 파일(`kz-expect 1`)·미리 정한 사용 목록은 주기 준비 단계의 것이다. 판독지와 읽는 사람은 PI 결정 그대로다.
5. 적재된 cFS 모듈의 `buildid` 줄은 부팅 전이라 본 적이 없다 [미확인]. 줄을 읽는 쪽의 고침만 부팅 뒤에 생기면 판정 부분의 sha256 이 같고 diff 를 남긴다(명세 2 절).
6. lead 의 판단 (b), (c)의 열린 점(성공 경로 줄의 지역 변수 빈틈, 완전하지 않은 버퍼)은 그대로이고, 이 판정 부분에서는 기초 4 와 (다)의 `missing` 으로 나온다.

## 25. C1 자동 주기 하나의 준비: 계획, 파생 빌드 둘, 결합, 부팅 전에 커밋하는 것 (2026-11-19, 부팅 없음)

§12.10 4 항 (7)의 단계다. kz-static 단계의 추출(`cz_final_extract`)에서 낸 두 C1 계획(`cz_plans_v4_ns2/c1_pre_tx`, `c1_post_tx`)을 그대로 쓰고(새 추출 없음), 재생성 점검, 런타임 트리, kzgcc 0.4 의 파생 빌드 둘, 결합 보고, 파생 사실과 기준-파생 비교를 한 뒤, 부팅 선언 전에 다음을 차례대로 커밋 하나씩으로 낸다: (1) 미리 정한 사용의 목록, (2) 판독지의 정적 부분, (3) 규칙으로 만든 미대조 항목의 후보 목록과 평가 갈래 파일, (4) 모든 평가 갈래의 기대, (5) 부팅 선언. 기록: `../records/closure-c1prep-20261119/`(README). 출력: `/home/user/work/redesign/obs1/c1p_*`(새 디렉터리). cFS 를 부팅하지 않는다. push 하지 않는다. 이 부팅들은 설계 행의 **대조 실행**이고 사전 시험이 아니다(§12.10 3 항).

### 25.0 lead 의 판단 (2026-11-19, §17 의 열린 점에 대해; 이 단계는 어느 것도 바꾸지 않음)

- (a) 판정 규칙 8.13 17 항 조건 (6)은 글대로 적용한다: writer 집합은 필드 단위 규칙과 형 단위 규칙 **둘 다**에서 완전해야 한다. 도구를 필드 단위로만 읽게 바꾸지 않는다. 필드 단위만의 읽기는 설계 행을 본 뒤의 규칙 읽기다. 커밋된 평가 갈래 `Re` 가 그것을 덮는지는 설명 스크립트 단계와 평가 파일의 몫이다(§24.4 (라): 글은 덮지만 입력 줄이 전제 아래의 호출 결과를 주지 않아 움직이지 않음). 덮지 않으면 PI 의 열린 점으로 적고, 지금 어느 글도 넓히지 않는다.
- (b) 성공 경로 줄에서 out 으로 저장되는 지역 변수의 추출 빈틈은 그대로 둔다(`include/` 를 바꾸면 모든 바이너리가 다시 링크됨). 성공 경로 줄은 그 까닭과 함께 missing 이다. 열린 점이다.
- (c) 저장된 이름 버퍼: scalar-array FIELD_STATUS 가 건전하게 complete 가 될 수 있는지는 깃발마다 scratch 출력에서 정한다. 하나라도 건전하게 뺄 수 없으면 줄은 not complete 와 그 깃발을 적고, 설계 행이 나오게 하려고 아무것도 느슨하게 하지 않는다(§18.2: address-escape 는 어느 버퍼에서도 풀리지 않음).
- (d) 바뀌는 golden 은 선언된 빌드에서 커밋하고, 선언은 완전한 소스의 scratch 점검 빌드에서 확인한 첫 ctest 실패를 이름으로 적는다(§21 에서 끝남; 이 단계는 Kairos 도구를 짓지 않으므로 해당 없음).
- (e) C1 빌드의 둘째 주소 넘김 호출 자리와 `other` 열린 까닭 부류는 빌드 단계의 측정에서 맞으면 세고, 아니면 열린 점이다(§21.5 5, §22 가 다룸; 이 단계는 건드리지 않음).

### 25.1 시작 점검과 공간 [M]

- 시작: 여유 541,810,688 B(08:41:11Z, `out/free_start.txt`), 바닥 314,572,800 B. `ledger_check` 178 행 실패 0(`out/ledger_check_start.txt`), `tools_check --deep` 56 행 실패 0(`out/tools_check_deep_start.txt`). MC 의 도구 일곱은 kz-static `6f40dd60…`, kz `94cba72b…`, kz-dump `32a96ae7…`, kz-rt-dump `d83fdde9…`, kz-phi `df79fb82…`, kz_lint `3d4d5a8f…`, kz-instr-clang `4041f6b4…`(`out/pre_mc.txt`); kzgcc 0.4 `985fc102…`.
- 필요량(`out/space_need.txt`, 어느 일보다 먼저 적고 `c131a87` 로 커밋): 두 빌드에 남는 것 약 156 MB, 마지막 파생 단계에서 감시가 바닥 + 100,000,000 을 요구하므로 230 MB 가 여유 227 MB 를 넘는다. 바닥 위에서 맞지 않는다.
- 그래서 §12.10 2 항이 허용한 압축 대체를 옵션 없는 추출 회차 넷에만 썼다(`reclaim/archive.sh`, 자기 시험 2/2 `reclaim/selftest.txt`): `objacc_off_extract`, `sem_off_extract`, `rf_off_extract`, `ll_off_extract`(각 50 파일, 47,149,056 B 블록)를 `/home/user/work/preserve/archives-20261119/<이름>/<이름>.tar.xz`(3,396,416 / 3,394,656 / 3,383,000 / 3,404,512 B)로 바꿨다. 디렉터리를 지우기 전에 각 압축을 흘려 50 구성원 모두의 sha256 이 그 행의 목록과 줄마다 같음을 확인했다(`reclaim/roundtrip_<이름>.txt`). 원장의 네 행은 `parts` 방식으로 압축 디렉터리를 가리키고(목록 칸은 구성원 목록 그대로, 압축 디렉터리에 `PARTS.sha256` 과 `MEMBERS.sha256`), 원장 머리에 그 약속을 적었다(`reclaim/ledger_rewrite.txt`). 바꾼 뒤 `ledger_check` 178 행 실패 0(`reclaim/ledger_check_after_archive.txt`), 여유 713,269,248 B. 커밋 `f9ce046`. 중간 옵션 회차 셋은 건드리지 않았다. 그 회차들을 경로로 읽던 앞 단계의 비교 스크립트는 그대로는 다시 돌지 않는다(남긴 출력은 그대로다).

### 25.2 계획과 예산, 재생성 점검 [M]

- 계획은 kz-static 단계의 것을 그대로 쓴다: `cz_plans_v4_ns2/c1_pre_tx`(plan id `943b8cc0d0e200e4`, manifest `69a278b0…`, sidecar `a18a0370…`, 요청 `b79604d5…`)와 `cz_plans_v4_ns2/c1_post_tx`(plan id `b99c203f12de1e24`, manifest `ef35173e…`, sidecar `4ef73c91…`, 요청 `1b909f07…`). 둘 다 `cz_final_extract` 의 사실(c1_pre_tx `7403a377…`, c1_post_tx `77532840…`)에서 kz-static `6f40dd60…`(`tool_sha256=36e02995…`)이 냈다.
- 예산(`out/plan_budgets.txt`, sidecar 15–22 줄): 층 예산 `tier:D07`(까닭: 기록 풀이 고정이고 생성된 계획의 사건 양을 아직 재지 않음; 밝힘: 이 빌드들에서 우선 층에 어느 행이 드는지 알고 정한 값), c1_pre_tx 계획된 9·잘린 358, c1_post_tx 8·359, `unresolved="whether the rows cut fit the record pool (no event volume measured)"`, `sensitivity="not measured"`. 대조 예산 `all`(까닭의 글은 "control runs of design rows", §12.10 3 항의 말), 행 5(a 1, b 4), 계획된 5, 잘린 0, `complete=yes`, 같은 `unresolved`·`sensitivity`. 대조 행은 두 계획 모두 A20·A21·A22·A26·A63 이고, c1_pre_tx 의 A21 은 층 1 이기도 하다(c1_post_tx 의 A21 은 층 2, label H). 값 요청: c1_pre_tx 행 4·요청 36·계획 30, c1_post_tx 4·30·24.
- 재생성 점검(`env/tools/plan_regen.sh` 를 MC 의 kz-static 그대로, 각 계획의 `args.txt` 로; 출력 `obs1/c1p_regen/<빌드>`, 감시 `guard/regen_<pre|post>.log`): 두 계획 모두 manifest·sidecar·요청 파일이 같은 바이트, module 표와 실행 파일 목록이 첫 줄의 sidecar 경로 밖에서 같음, `PASS`(`derived/<pre|post>/regen.txt`). 계획을 낸 도구가 MC 에 그대로 있으므로 사본을 풀지 않았다.

### 25.3 파생 빌드 둘의 선언 (빌드 전에 적고 커밋함) [D]

아래 밖의 차이는 결함이다. 결함이면 그 점검에서 멈추고, 까닭을 밝힌 덧붙임을 커밋한 뒤에만 그 비교를 새 이름으로 다시 돌린다. 모든 일은 감시(`env/tools/space_guard.sh`, 바닥 314,572,800 B) 아래에서 돌리고 감시 기록은 `guard/` 에 둔다. 출력은 새 디렉터리에만 쓰고 덮어쓰지 않는다. 단계는 `scripts/run.sh` 의 것이다.

- **Kairos 도구**: 짓지 않는다. MC 의 도구 일곱과 정체성·정의·`CMakeCache.txt` 는 앞뒤가 같아야 한다(`out/pre_mc.txt` 와 끝의 `post_mc.txt`, `build_record.sh`). 정체성이 바뀌는 도구: 없음. 다시 링크되는 MC 바이너리: 없음. 플러그인은 kzgcc 0.4 `985fc102…`(`/home/user/work/instr/kzgcc-0.4/plugin-build/kzgcc.so`, 판 글 `kzgcc 0.4`) 그대로.
- **런타임 트리**(빌드마다 하나, `obs1/c1p_<pre|post>_rt/rt`): `build_runtime.sh` 로 `kairos/runtime`(HEAD, 마지막 변경 `139330e`, `kzrt 0.5.0`, dump 1.5)을 대상 gcc 로 따로 짓는다. backend 는 `kzgcc 0.4 (gcc-plugin)`, manifest 는 그 빌드의 계획. 라이브러리·객체·kz-rt-dump 의 sha256 은 `runtime.record` 에 기록으로 남는다(등록부의 도구가 아니다).
- **단계**(빌드마다, c1_pre_tx 먼저): runtime → prepare(`derive_c1.sh prepare <pre|post> 1df262a obs1/c1p_<pre|post>_derived`) → ref(같은 자리의 기준 빌드, 해시 뒤 지움) → instr(kzgcc 0.4 와 그 런타임 트리) → check → rx → buildids → bprime(`one_build.sh <빌드> obs1/c1p_<pre|post>_bprime --path-map`, kz-static `6f40dd60`, B 와 같은 추출 옵션 모두: `--object-access`, `--semantics` 자료 여덟, `--result-flow`, `--loop-defs`, `--lock-handles`) → bindings → compare.
- **기준과 파생 사이에 허용하는 차이**(레코드·칸 수준; 조건 그대로는 `derived/declared_diff.tsv`, 37 줄): 첫 파생 빌드의 목록(`records/obs1-20261116-derived/declared_diff_after.tsv`; 관측 경로 결과 §5.0·§5.3 의 원래 목록과 덧붙임)을 그대로 쓴다. B 는 계획을 낸 사실 `cz_final_extract/<빌드>/<빌드>/NP` 이다. 갈래: 정체성(머리·절 줄·`extract.units_from`·글 출력의 해시 줄·`args.txt` 의 맵 줄), 플러그인 옵션(BASIS `instrumentation.*`·`front_end.arguments_removed`·`units.distinct_flag_sets`, DEVIATIONS 하나, TUS `flags`·`command_sha256`, `compile_commands.json`·플러그인 파일·모듈 `link.txt` 의 FILE_HASHES, facts 가 옮긴 편차, `relations.txt` 의 편차 수 줄), 링크된 런타임(BASIS `link.*`·`binary.*`·`closure.*`·`module[*]` 의 값, `basis.txt` 의 같은 줄, 컴파일 명령 없는 런타임 객체의 경고 줄), D2 의 module 판 표, D4 의 빌드 환경 표와 그 단위에서 읽은 레코드, 계측·링크된 파일의 FILE_HASHES, D1 경로 글의 `bplib.so`. 그 밖은 모두 같아야 한다(facts 의 다른 절은 맵 뒤 레코드가 차례까지 같음).
- **객체 비교**: 다른 객체는 계측 단위(결합 파일이 있는 단위)와 D4 의 `cfe_build_env_table.c.o` 둘(tx, rx)뿐이어야 한다(`check.record` 의 `UNEXPECTED` 0, 바뀌지 않은 계측 단위 0).
- **설치 파일**: 기준과 다른 것은 계획의 실행 파일 목록의 tx 일곱과 같은 이름의 rx 일곱(bplib.so 는 site 가 없어 D1 경로 글로만 다름), 그리고 경로 글로 다른 `cfe_assert.so` 둘(tx, rx; 첫 파생 빌드 §5.2 와 같은 까닭)뿐이어야 한다(`install_inventory.tsv`).
- **결합 보고**(`env/tools/plan_bindings.py`, `--file-hashes` 로 B′ basis 의 FILE_HASHES 와 단위의 소스 sha256 을 묶음): 계획의 모든 site 에 결합 줄이 있어야 하고(`not-seen` 0), bound·inlined·duplicated 가 아닌 site 는 플러그인의 까닭과 함께 나열한다. 단위마다 manifest sha256 = 계획의 것, module id = 계획의 표와 플러그인 규칙. 런타임 단계의 묶음 점검이 이미 본 것 [I]: c1_post_tx 의 `s0297.wait-exit`(`cf_cfdp.c:1008` 의 `break`)는 `no statement on line 1008 of the function` 으로 묶이지 않는다(§23.4 5). site 의 상태를 그 밖에 미리 기대하지 않는다.
- **build-id 를 적을 객체**(값은 빌드 뒤, 어느 부팅보다 먼저 `derived/<pre|post>/exe_buildids.tsv` 에 적는다: 설치된 모든 ELF 의 sha256 과 `readelf -n` 의 GNU build-id): 부팅 기록의 `buildid` 줄이 이름 댈 객체는 주 실행 파일 `exe/tx/core-tx` 와 site 가 있는 단위의 모듈 등록이 적재된 객체다. 계획의 module 표로는 bp(1) `exe/tx/cf/bp.so`, cf(3) `cf.so`, ci_lab(4) `ci_lab.so`, core-tx(5, 실행 파일 자신), sch_lab(6) `sch_lab.so`, to_lab(7) `to_lab.so` 다 [I]. bplib(2)는 site 가 없어 등록이 없으리라 본다 [I]. rx 의 파일은 `KZ_RT_MODE=off` 로 돌아 기록을 쓰지 않지만 값은 함께 적는다.
- **시험**: 결합 보고(위), `hookcheck` rc 0(계획의 목록, tx), module id = 계획의 표, rx 의 (함수, hook, id) 목록 = 계획의 그룹, 바인딩 수와 site 의 상태는 측정으로 적는다.
- **누적 바이트 동일성**: 끝에 `ledger_check.sh` 전체와 `tools_check.sh --deep`. 새 출력은 장부 행으로 더한다(`OBS_c1p_*`). 바깥 이름 점검을 새 출력 디렉터리에 돌린다(파생 작업 공간의 upstream 소스 사본에 걸리는 것은 첫 파생 빌드 §5.4 와 같이 따로 적는다).
- **공간**: `out/space_need.txt` 의 빌드마다의 값 [I]. 지금 여유 713,269,248 B. 감시 `--need`: runtime·prepare·ref·instr·check 100,000,000, bprime 60,000,000(`ONE_BUILD_NEED`), compare 10,000,000. `derive_c1.sh` 가 같은 자리 기준 빌드 트리를 해시 뒤 지우는 것 밖에는 아무것도 지우지 않는다.

### 25.4 첫 파생 빌드가 멈춘 것: kzgcc 0.4 의 컴파일러 멈춤 [M]

- c1_pre_tx 의 런타임 트리(`obs1/c1p_pre_rt/rt`, backend `kzgcc 0.4 (gcc-plugin)`, `kzrt 0.5.0`; `libkzrt.a` `7e1f2c47…`, `libkzrt-posix.a` `70b88536…`, 시작 객체 `813b529c…`, build-info `be4edb60…`, kz-rt-dump `d55a9081…`; `derived/pre/runtime.txt`), prepare(맵 `a298a530…`, 복사본과 기준의 차이 0, `derived/pre/prepare.txt`), 같은 자리의 기준 빌드(rc 0, 해시 뒤 지움)는 선언대로 끝났다.
- 계측 빌드에서 `make rc=2`(`derived/pre/attempt1/build_instr.record`; `derive_c1.sh` 는 make 의 rc 를 기록에만 적고 0 으로 끝나므로 `run.sh` 의 rc 0 은 성공이 아니었다). kzgcc 0.4 가 `cfe/modules/tbl/fsw/src/cfe_tbl_task_cmds.c` 의 함수 `CFE_TBL_GetTblRegData` 에서 GCC 를 멈추게 했다: `internal compiler error: in make_ssa_name_fn, at tree-ssanames.cc:357`(`derived/pre/attempt1/build_instr_log_ice.txt`). 실행 파일과 모듈은 설치되지 않았다. **그 점검에서 멈췄다**(선언 25.3: 선언 밖의 결과).
- 까닭 [M]: 그 단위의 site 242(`s0242.use-call+writer`, `CFE_ES_GetAppName` 의 after-call, 값 `out0`)가 읽는 `*arg0` 의 인자는 문자 배열 필드(`…Payload.OwnerAppName`)의 주소이고 GIMPLE 에서 그 형은 20 바이트 배열을 가리킨다. kzgcc 0.4 는 `out<k>` 의 가리키는 형으로 SSA 이름을 먼저 만들었다(`instrument/gcc/kzgcc.cpp` 의 `make_ssa_name(pointee)` 가 `is_gimple_reg_type` 의 시험보다 앞). GCC 는 레지스터가 아닌 형의 SSA 이름을 거부한다. 이 줄은 kzgcc 0.3 에도 같았다(`git show 1611e18`); 앞 계획들에는 그런 `out<k>` 가 없었다.
- 범위 [M]: 두 계획이 site 를 둔 모든 단위를 남긴 compile 명령으로 kzgcc 0.4 와 함께 scratch 에서 컴파일했다(`scripts/unit_check.py`, 감시 아래; `scratch/unitcheck_kzgcc04_<pre|post>.txt`, `…_units.tsv`). c1_pre_tx 33 단위, c1_post_tx 32 단위에서 멈추는 것은 둘 다 이 한 단위뿐이다.
- 다음: 플러그인을 고친 판(kzgcc 0.5, 새 디렉터리)을 형식 글(`kairos/docs/event_semantics.md` 13.7) → 코드와 시험 → 완전한 소스의 scratch 점검 빌드 → 선언 → 빌드의 차례로 내고, 파생 빌드 둘을 **새 디렉터리**에서 다시 한다(아래 25.5). 첫 시도의 출력(`obs1/c1p_pre_rt`, `obs1/c1p_pre_derived`: 맵과 복사본, 같은 자리 기준 빌드의 객체 해시, 멈춘 계측 빌드 트리)은 그대로 남긴다(덮어쓰지 않음). 이 고침은 이 단계의 범위 밖의 도구 빌드다(25.12 벗어난 것).

### 25.5 kzgcc 0.5 의 빌드와 파생 빌드 둘의 다시 함: 선언 (빌드 전에 커밋함) [D]

**(가) 소스와 scratch 점검.** 형식 글 `kairos/docs/event_semantics.md` 13.7 을 `5f434b4` 로, 코드와 시험을 `116654d` 로 커밋했다(`instrument/gcc/kzgcc.cpp`: `values_bindable` 와 `out<k>` 의 집합체 읽기, 판 글 `kzgcc 0.5`; `tests/kzgcc_outagg.c`·`kzgcc_outagg.kzs`, `run_target_test.sh` 의 `outagg-O0|O2`, `tests/CMakeLists.txt` 의 `kzgcc_outagg_O0`·`_O2`). 완전한 소스(`116654d` 의 작업 트리)를 scratch 에서 지었다(감시 `guard/s_plugin05_checkbuild.log`; 경고 0; `scratch/plugin05/build.log`, kzgcc.so `5e691ce7…`). 그 scratch 점검의 결과 [M, scratch]:
- 플러그인 ctest(`-V`, `KZGCC_KZ` = MC kz): 20 시험 모두 통과(`scratch/plugin05/ctest_scratch.log`; 앞의 18 과 새 `kzgcc_outagg_O0`·`_O2`). kzgcc 0.4 는 새 시험의 컴파일에서 두 최적화 모두 멈춘다(prototype 에서 확인; 25.4 와 같은 `make_ssa_name` 의 멈춤).
- 두 계획의 모든 단위(`scripts/unit_check.py`, 감시 `guard/s_unitcheck_k05_<pre|post>.log`): c1_pre_tx 33 단위·c1_post_tx 32 단위 모두 컴파일됨(`scratch/unitcheck_kzgcc05scratch_<pre|post>.txt`). 결합 줄 c1_pre_tx bound 295·duplicated 2·inlined 7·unsupported 1(site 242, `aggregate operand larger than 8 bytes`), c1_post_tx 291·2·7·2(site 242 와 site 297 `no statement on line 1008 of the function`).
- kzgcc 0.4 와의 같음(`scripts/compare_units.py`, `scratch/compare_units_k04_k05scratch_<pre|post>.txt`): 0.4 가 컴파일한 단위 32/31 모두 목적 파일이 같은 바이트이고 결합 파일이 머리 줄의 판 글과 `dumpbase=`(그 실행의 목적 파일 경로) 밖에서 같다. 첫 비교(`…_trial1.txt`)는 `dumpbase=` 를 가리지 않아 모든 단위를 다르다고 냈다(비교 스크립트의 결함; 남김). 판 1–3 manifest(런타임 단계의 `regress_manifest.sh`; 비교 `scripts/compare_manifest_2.py`, 판 글을 인자로 받는 사본): 네 묶음 모두 목적 파일 15 같은 바이트, 결합 파일 11 이 2 번째 줄만 다름(`scratch/plugin05/compare_manifest_k04_k05scratch.txt`; 판 글을 0.3/0.4 로 박은 원 스크립트의 첫 비교 `…_trial1.txt` 는 남김).

**(나) 플러그인 빌드의 선언.**
- **정체성이 바뀌는 도구**: 없다(MC 의 도구 일곱은 앞뒤가 같아야 한다: `out/pre_mc.txt` 와 끝의 `post_mc.txt`). **새 파일**: kzgcc 0.5 `/home/user/work/instr/kzgcc-0.5/plugin-build/kzgcc.so`(새 디렉터리; 등록부에 새 행). kzgcc 0.4 `985fc102…` 와 그 디렉터리는 건드리지 않는다. 플러그인 프로젝트는 MC 의 빌드 밖이다: MC 의 dry run(`make -n all`, `plugin05/mc_make_dryrun.log` 509 줄)에 다시 컴파일·링크되는 것이 0 이다. 다시 링크되는 MC 바이너리: 없음.
- **첫 ctest 에서 실패할 시험**: 없음(scratch 점검 빌드의 20/20). 첫 실행은 `-V`.
- **출력 종류별 허용 차이**: (1) 판 1–3 manifest 의 묶음(`regress_manifest.sh` → `obs1/c1p_regress_manifest_k05`): kzgcc 0.4 의 `obs1/rtb_regress_manifest_new` 와 목적 파일은 같은 바이트, 결합 파일은 2 번째 줄(판 글과 dumpbase)만 다름. (2) 두 C1 계획의 모든 단위(`unit_check.py` 를 kzgcc 0.4 로 `obs1/c1p_unitcheck_k04_<pre|post>`, 0.5 로 `obs1/c1p_unitcheck_k05_<pre|post>`): 0.4 가 컴파일하는 단위마다 목적 파일 같은 바이트, 결합 줄은 머리의 판 글과 dumpbase 밖에서 같음; 0.4 가 멈추는 `cfe_tbl_task_cmds.c` 는 0.5 에서 컴파일되고 site 242 가 `unsupported`(`aggregate operand larger than 8 bytes`). 그 밖의 차이는 결함이다.
- **빌드 뒤 점검**: 플러그인 ctest(첫 실행 `-V`), MC 의 lint(`ctest -R '^lint'`, 빌드 없이), `ledger_check.sh`, `tools_check.sh --deep`, `env` 의 단위 점검 둘. 
- **공간**: 플러그인 디렉터리 약 11 MB, 위 출력 약 3 MB [I: scratch 의 크기]. 지금 여유 637,272,064 B.

**(다) 파생 빌드 둘의 다시 함(25.3 의 덧붙임).** 25.3 은 그대로 따르고 다음만 바꾼다. 플러그인은 kzgcc 0.5(위 (나)의 파일). 출력은 새 디렉터리 `obs1/c1p2_<pre|post>_rt`, `obs1/c1p2_<pre|post>_derived`, `obs1/c1p2_<pre|post>_bprime` 이고 기록은 `derived2/<pre|post>/`, 비교 출력은 `derived2/compare_<빌드>.tsv` 다(스크립트 `scripts/run2.sh`: `run.sh` 와 같고 이 이름들만 다름). 런타임 트리의 backend 글은 `kzgcc 0.5 (gcc-plugin)` 이다. 허용 차이 목록은 같은 `derived/declared_diff.tsv` 다(플러그인 파일의 FILE_HASHES 줄은 경로를 묻지 않는다). 결합 보고에서 묶이지 않으리라 보는 것 [I: (가)의 scratch]: 두 빌드의 site 242, c1_post_tx 의 site 297. `derive_c1.sh` 는 make 의 rc 를 기록에만 쓰므로 계측 빌드마다 `build_instr.record` 의 `make rc=0` 을 따로 본 뒤에만 다음으로 간다(25.4).

### 25.6 kzgcc 0.5 빌드의 결과와 c1_pre_tx 파생 빌드의 첫 비교, 덧붙임 1 (다시 하는 비교보다 먼저 커밋함) [M][D]

**(가) kzgcc 0.5 빌드** [M]: `scripts/plugin05.sh`(감시 `guard/p_*.log`). `/home/user/work/instr/kzgcc-0.5/plugin-build/kzgcc.so` `5e691ce73c59c95d323d4b332ee59e65d51945bc9ad7b86a90db716630ba1aa0`(scratch 점검 빌드와 같은 바이트; 판 글 `kzgcc 0.5`; 경고 0; `plugin05/build_result.txt`). 첫 ctest(`-V`) 20 시험 모두 통과(`plugin05/ctest_first.log`). 판 1–3 manifest: 네 묶음 목적 파일 15 같은 바이트, 결합 파일 11 이 2 번째 줄만 다름(`plugin05/compare_manifest_k04_k05.txt`; 출력 `obs1/c1p_regress_manifest_k05`). 두 계획의 모든 단위: kzgcc 0.4 로 c1_pre_tx 33·c1_post_tx 32 단위 가운데 `cfe_tbl_task_cmds.c` 하나씩 멈춤, 0.5 로 모두 컴파일; 0.4 가 컴파일한 32/31 단위의 목적 파일과 결합 줄이 판 글·dumpbase 밖에서 같음(`plugin05/compare_units_k04_k05_<pre|post>.txt`, `unitcheck_k0<4|5>_<pre|post>.txt`; 출력 `obs1/c1p_unitcheck_k0<4|5>_<pre|post>`). 빌드 뒤: MC lint 8/8(`plugin05/mc_lint_ctest.log`), 단위 점검 19/19·7/7(`plugin05/unit_tools.txt`, `unit_step2a.txt`), 등록부 새 행 하나, `tools_check` 57 행 실패 0(`plugin05/tools_check_after_build.txt`). 모두 선언 25.5 (나) 대로다.

**(2026-11-19 덧붙임, `builds-01`)** 위 끝의 '모두 선언 25.5 (나) 대로다' 는 맞지 않는다. 선언 25.5 (나)의 빌드 뒤 점검 가운데 셋을 선언대로 하지 않았다. (1) 플러그인 빌드(`guard/p_plugin-build.log`, 09:07:31–09:07:35Z) 바로 뒤의 `ledger_check.sh` 가 없다: 원장 점검은 08:47:02Z(`reclaim/ledger_check_after_archive.txt`, 178 행 실패 0) 다음이 09:23:51Z(`out/ledger_check_after_builds.txt`)이고, 그 사이에 플러그인 빌드와 c1_pre_tx 파생 빌드 전체(`guard/2_runtime_pre.log` 09:10:21Z – `guard/2_bprime_pre.log` 09:11:54Z)가 있다. (2) `plugin05/tools_check_after_build.txt`(09:10:12Z)는 `--deep` 없이 돌았다(끝 줄 `tools_check: 57 rows, 0 failing`, `(deep)` 없음). (3) 플러그인의 첫 ctest(`scripts/plugin05.sh:21`, `ctest -V`)는 감시 밖에서 돌았다(`guard/` 에 ctest 기록 없음; 런타임 단계는 같은 종류의 시험을 감시 아래 돌렸다, `../records/closure-runtime-20261119/guard/b13_kzgcc_ctest.log`). 빌드 뒤의 첫 점검은 `out/ledger_check_after_builds.txt`(193 행 실패 0, 09:23:51Z)와 `out/tools_check_deep_after_builds.txt`(57 행 `--deep` 실패 0, 09:26:05Z)이고 둘 다 지났다. 시각과 끝 줄은 `../records/closure-notes-20261119/check.txt` D. 뒤 단계는 선언한 빌드 뒤 점검을 빌드 스크립트 안에 넣는다. [§26.1]

**(나) c1_pre_tx 의 파생 빌드(둘째 시도)** [M] (`scripts/run2.sh`, 기록 `derived2/pre/`): 런타임 트리 `obs1/c1p2_pre_rt/rt`(backend `kzgcc 0.5 (gcc-plugin)`; build-info `e4caad6c…`, 그 밖의 라이브러리·시작 객체·kz-rt-dump 는 25.4 의 것과 같은 sha256), prepare, 같은 자리 기준 빌드, 계측 빌드 모두 `make rc=0`(계측 빌드 경고 1, 기준 빌드와 같은 수). check: 객체 261 가운데 다른 것 33 = 계측 단위 33(UNEXPECTED 0, 바뀌지 않은 계측 단위 0); D4 의 `cfe_build_env_table.c.o` 는 이번에는 다르지 않았다(선언의 "뿐"에 든다). 결합 305: bound 295, duplicated 2, inlined 7, unsupported 1(site 242). hookcheck rc 0(ok 304, gap 1). 설치 파일 31 가운데 기준과 다른 16 = 계획의 tx 일곱, 같은 이름의 rx 일곱(core-rx 포함), `cfe_assert.so` 둘(선언대로). rx: ELF 9, hook 호출 310, 계획의 그룹과 같은 목록. build-id: 설치된 ELF 21(`derived2/pre/exe_buildids.tsv`). B′(`obs1/c1p2_pre_bprime`, 추출 옵션 모두): `one_build.sh` PASS. 결합 보고(`derived2/pre/bindings_report.tsv`): 실패 0, 단위 33 모두 manifest sha256 = 계획, module 은 표와 플러그인 규칙대로, 단위의 소스 sha256 이 B′ basis 의 FILE_HASHES 와 디스크에서 같음.

**(다) 기준-파생 첫 비교** [M] (`obs1/c1p2_compare/compare_c1_pre_tx.tsv`, 요약 `derived2/compare_c1_pre_tx_summary.txt`, rc 1): 선언 밖 3,787. **그 점검에서 멈췄다.** 갈래 둘이다.
1. *비교기가 읽지 않던 FILE_HASHES 번호 칸* 3,714(`RESULT_EFFECT` 3,462 의 `effect_fh=`, `ARG_TYPES` 252 의 `decl_fh=`): 두 칸은 `fh_of(...)` 가 쓰는 FILE_HASHES 번호다(`kairos/lib/llvm/serialize.cpp:1032`, `:1356`). 파생 작업 공간에서는 FILE_HASHES 의 경로 차례가 달라(첫 파생 빌드 관측 경로 결과 §5.3 의 `tu=` 와 같은 까닭) 번호가 밀린다. 첫 파생 빌드는 옵션 없는 사실이라 이 절들이 없었다. 비교기(`env/redesign/compare_derived.py`)가 facts 의 `decl_fh=`·`effect_fh=` 도 `fh=`·`tu=` 처럼 경로로 읽게 고쳤다(읽기의 고침이고 판정 기준을 넓히지 않는다).
2. *소스 트리 토큰* 73(BASIS 의 `source_tree[0..2]`·`count.type_names_outside_root`·그 때문에 차례가 밀린 `extract.units_from`, 형 이름 등에 토큰이 든 레코드: `LOCKSET` 23, `READS` 13, `OBJECT_ACCESS` 13, `FUNC_WRITES` 4, `FIELD_TYPES` 3, `FIELD_STATUS` 3, `FACT_EXPRS` 3, `CALL_TARGETS` 3; `relations.txt` 21 줄의 `key texts by source tree 3` → `0`): 소스 트리는 파일 위로 가장 가까운 `.git` 항목을 가진 디렉터리다(`kairos/docs/static_facts.md` 5x, `serialize.cpp:1196-1212`). 파생 작업 공간의 복사본은 git 정보 없이 만든다(`derive_c1.sh` D2, `tar --exclude=.git`). 그래서 B′ 에는 소스 트리가 없고 B 가 `<cf>/…` 로 쓰는 이름을 절대 경로로 쓴다. 내용의 차이이고 D2 에서 온다. 비교기에 조건 `treetok`(B 의 토큰을 B 의 `source_tree` 항목의 디렉터리로 적으면 B′ 의 레코드와 같음)을 더해 그 레코드들을 그 갈래로만 받게 했다. `relations.txt` 는 키 글을 싣지 않으므로(키는 `--keys` 파일) 그 밖의 줄은 같다. 이 차이는 평가에 들지 않는다: 설명 스크립트는 B 의 정적 출력(`cz_final_order`)과 계획을 읽고, B′ 는 연결 점검의 `bprime` 고리와 `kz convert --basis` 에만 쓰인다.
- 비교기의 단위 점검 11 에 (c) 를 더했다(밀린 `decl_fh=`·`effect_fh=`, `treetok` 으로 받는 토큰, `treetok` 줄이 없을 때·토큰 밖의 차이·진짜 `effect_fh=` 의 차이는 rc 1): `derived2/unit_tools_comparator.txt` 19/19.
- **덧붙임 1**(`derived/declared_diff_2.tsv` = `declared_diff.tsv` 의 37 줄 + 5 줄): B 의 `source_tree` 항목(old), `count.type_names_outside_root`(두 쪽), `extract.units_from`(두 쪽, 정체성), facts 의 모든 절에서 `treetok`, `relations.txt` 의 규칙 줄(소스 트리 수만 다른 그 줄 전체). 다시 하는 비교는 `run2.sh compare <빌드> 2` → `obs1/c1p2_compare/compare_<빌드>_2.tsv` 다. 첫 비교 출력은 남긴다. c1_post_tx 의 비교도 이 목록으로 한다.

**(라) 덧붙임 1 로 다시 한 비교와 덧붙임 2** [M][D]: `run2.sh compare c1_pre_tx 2`(`obs1/c1p2_compare/compare_c1_pre_tx_2.tsv`, 요약 `derived2/compare_c1_pre_tx_2_summary.txt`): rc 1, 선언 밖 89, 모두 `ARG_TYPES`. **다시 멈췄다.** 89 레코드는 모두 선언 없는 문자 배열 리터럴(`decl_object=""`, `type_class=char-array`)이고, 두 쪽이 `extent=`·`type=` 밖에서 같으며, 크기가 79 개는 9, 10 개는 13 만큼 크다. 그 두 수는 맵의 줄이 경로를 늘리는 길이다(`pre/src/` → 파생 `src/` 가 9, `bundle_1df262a/` 가 13). 소스 경로 리터럴(`__FILE__` 처럼)은 작업 공간의 경로만큼 길다: D1(작업 공간 경로 글)의 결과다. 비교기에 조건 `pathlen`(두 쪽이 `extent=`·`type=` 밖에서 같고, 형이 `[const ]char[<그 extent>]` 이며, 크기 차이가 맵의 한 줄의 길이 차이)을 더하고 단위 점검 11 에 (d) 를 더했다(`derived2/unit_tools_comparator_2.txt` 19/19). **덧붙임 2**(`derived/declared_diff_3.tsv` = 덧붙임 1 의 42 줄 + 1 줄): `ARG_TYPES` 의 문자 배열 리터럴에 `pathlen`. 다음 비교는 `run2.sh compare <빌드> 3` 이다.

### 25.7 파생 빌드 둘, 결합, 파생 사실과 기준-파생 비교 [M]

수는 `derived2/<pre|post>/` 와 `obs1/c1p2_compare/` 의 남긴 출력에서 왔다. 차례는 c1_pre_tx / c1_post_tx 다.

| 점검 | c1_pre_tx | c1_post_tx |
|---|---|---|
| 런타임 트리 (`obs1/c1p2_<pre\|post>_rt/rt`, backend `kzgcc 0.5 (gcc-plugin)`, `kzrt 0.5.0`) | build-info `e4caad6c…` | 같은 꼴(manifest 가 다름) |
| prepare: 복사본과 기준의 차이 | 0 / 0, 맵 `path_map.tsv` | 0 / 0 |
| 기준 빌드(같은 자리), 계측 빌드 | `make rc=0`, `make rc=0`(경고 1) | `make rc=0`, `make rc=0` |
| 객체 261 가운데 다른 것 | 33 = 계측 단위 33 (UNEXPECTED 0) | 32 = 계측 단위 32 (UNEXPECTED 0) |
| 결합 (`bindings_report.tsv`, 실패 0) | site 305: bound 295, duplicated 2, inlined 7, unsupported 1 | site 302: bound 291, duplicated 2, inlined 7, unsupported 2 |
| hookcheck (계획의 목록 일곱) | rc 0, ok 304, gap 1 | rc 0, ok 300, gap 2 |
| 설치 파일 31 가운데 기준과 다른 것 | 16 (tx 일곱, rx 일곱, `cfe_assert.so` 둘) | 16 (같은 꼴) |
| rx | ELF 9, hook 호출 310, 계획의 그룹과 같음 | ELF 9, 306, 같음 |
| B′ (`one_build.sh --path-map`, 추출 옵션 모두) | PASS | PASS |
| 기준-파생 비교 (덧붙임 2, `compare_<빌드>_3.tsv`) | rc 0, 선언 밖 0 | rc 0, 선언 밖 0 |

- **묶이지 않은 site 와 그 까닭**(결합 보고의 플러그인 까닭 그대로): 두 빌드의 site 242 `s0242.use-call+writer`(`cfe_tbl_task_cmds.c` `CFE_TBL_GetTblRegData`, 층 행 B119·B125 의 after-call; `out0` 가 20 바이트 배열을 가리킴): `aggregate operand larger than 8 bytes`. c1_post_tx 의 site 297 `s0297.wait-exit`(`cf_cfdp.c` `CF_CFDP_InitEngine`, 대조 행 A21 의 대기 루프를 나가는 문): `no statement on line 1008 of the function`(§23.4 5 와 같음). 그 밖의 site 는 모두 bound·inlined·duplicated 다. 결합 파일: c1_pre_tx `obs1/c1p2_pre_derived/binds_instr.kzs`, c1_post_tx `obs1/c1p2_post_derived/binds_instr.kzs`(sha256 은 부팅 선언 25.12).
- **build-id**(설치된 ELF 21 개의 sha256 과 GNU build-id, `derived2/<pre|post>/exe_buildids.tsv`; 부팅 기록의 `buildid` 줄이 이름 댈 객체는 25.3 의 목록): c1_pre_tx core-tx `619614002e6d4556fa731365d88ee7f0d49c00cf`, bp.so `01db56cf…`, cf.so `c55b464b…`, ci_lab.so `4074a88b…`, sch_lab.so `310dfddb…`, to_lab.so `1c6beaba…`(bplib.so `9540ee3b…`, 등록 없음 [I]); c1_post_tx core-tx `acee9edd432e2e987903efefe7fc0c19bdee41ac`, bp.so `f359e98b…`, cf.so `3eb1b8b3…`, ci_lab.so `8dc25470…`, sch_lab.so `14e71ebd…`, to_lab.so `a79dc818…`(bplib.so `ccea3094…`). rx 의 모듈은 tx 의 같은 이름 파일과 같은 바이트다(`rx_check.txt`).
- **비교의 갈래**(c1_pre_tx, `compare_c1_pre_tx_3.tsv`): 선언 갈래는 첫 파생 빌드의 것(정체성, 플러그인 옵션, 링크된 런타임, D2·D4, 계측·링크된 파일, D1 의 bplib)에 덧붙임 1·2 의 셋(D2 소스 트리 없음, `treetok`; D1 의 소스 경로 리터럴, `pathlen`)이다. 요약은 그 파일의 꼬리에 갈래별 수로 있다. facts 의 47 절 가운데 35 절이 맵 뒤 레코드가 차례까지 같다.
- **MC**: `out/post_mc.txt` 가 `out/pre_mc.txt` 와 도구 일곱·정체성·정의·`CMakeCache.txt` 에서 같다(HEAD 와 여유 줄만 다름). **누적**: 원장에 이 단계의 출력 행 15(`OBS_c1p_*`, `OBS_c1p2_*`; 목록 `lists/`) — `ledger_check` 193 행 실패 0(`out/ledger_check_after_builds.txt`), `tools_check --deep` 57 행 실패 0(`out/tools_check_deep_after_builds.txt`). 이름 점검 `--tree`: B′·비교 출력 깨끗. 파생 작업 공간은 첫 파생 빌드 §5.4 와 같이 upstream 소스 사본만 걸린다(따로 고치지 않음).
- **공간**: 가장 큰 dip 은 prepare 27,291,648 B 와 계측 빌드 19,394,560 B, 가장 낮은 감시 값 494,067,712 B(`guard/2_*.log`). 지금 여유 466,509,824 B.

### 25.8 (1) 미리 정한 사용의 목록 [F]

평가 절차 5.1 의 규칙으로 두 계획에서 만들었다(`scripts/uses_list.py`; 입력은 계획 `plan.kzs`·sidecar `plan.side` 와 그 계획을 낸 relations 글 `cz_final_order/<빌드>/relations.txt`; 행 번호·키·이름·기대 값은 입력이 아니다). 규칙: sidecar 의 경로 A 행 가운데 계획에 든 것(`state=planned` 의 층 또는 `control-state=planned` 의 대조) × 그 행의 relations `actors:` 줄의 행위자 뿌리 × 그 행의 `use-call` site 의 서로 다른 호출(같은 호출의 at-call·after-call 은 하나) × sidecar 의 그 행의 `context` 줄(없으면 `-` 하나).

- c1_pre_tx: `uses/uses_c1_pre_tx.tsv` sha256 `0ca47839d43db83c9821cab74445dff58cdfa9fb69e71ccf47e5fd4abd214e14`, 사용 6, 행 5(A20 1, A21 2(문맥 1·2, 틀 site 293·292 와 305·304), A22 1, A26 1, A63 1).
- c1_post_tx: `uses/uses_c1_post_tx.tsv` sha256 `955890c2c0876c0b70610acf93baf064431857ed74a2010be8957b2002f5fe20`, 사용 5, 행 5(각 1; A21 은 문맥 줄이 없음).
- 행위자 뿌리: A20 `BP_AppMain (script-entry)`, A21 `CF_AppMain (script-entry)`, A22 `SCH_Lab_AppMain (script-entry)`, A26 `CFE_TIME_TaskMain (table-image-entry)`, A63 `none`(relations 행에 `actors:` 줄이 없음).
- 점검 [M]: 동결한 설명 스크립트가 같은 계획의 정적 예행에서 센 `uses`(명세 10.7 4: 서로 다른 사용 호출 × 문맥)와 행마다 같다(c1_pre_tx A21 2, 나머지 1; c1_post_tx 모두 1; `obs1/xr4_dry_static_<빌드>_2/D1-R0/index.tsv`).
- 이 목록이 분모 표(평가 절차 8 절)의 "정한 사용"이다. 경로 B 의 층 행(B13, B73, B81, B99, B113, B118, B119, B125)은 5.1 의 규칙(경로 A)에 들지 않는다. 그 행들의 기록은 설명 스크립트가 행위자마다 내지만 이 목록에는 없다.

### 25.9 (2) 판독지의 정적 부분: 채우지 않음, 읽는 사람 없음 [F]

읽는 사람은 정해지지 않았다(PI 결정, §12.10 5 항; 평가 절차 9 절). 그래서 정적 부분은 미리 정한 사용 11 개(25.8) 모두 네 칸(호출자가 그 객체를 필요로 하는가, 객체 없이 잇는 경로, 객체를 만드는 task 와 그 종류, 순서를 강제하는 것)이 "not filled: no reader" 다(`sheets/sheets_static.tsv` sha256 `e70c31f2cf4e285df00ff28489ab3952b4d3611e5e9ae7ec52e12ee251458a1f`). 부팅은 진행하고, 정밀도와 계획 안의 재현율은 **정의되지 않음**으로 보고한다(평가 절차 8 절).

읽는 사람에게 줄 것과 주지 않을 것의 목록(`sheets/reader_files.tsv` sha256 `80313f0419640ff1de221a6606da02219a7b98e7e6cbf13f9d5d9680f9b692bd`; 아무것도 주지 않았다): 준다 — 판이 고정된 소스 트리 다섯(묶음 `1df262a64a09…`, CF pre `281a9418…`, CF post `833fdbb2…`, BP `b051eb0d…`, bplib `d4c6db9f…`, git 판), 미리 정한 사용의 목록 둘(sha256), 부팅별 부분에 그 사건들의 dump 글; 원 이슈와 수정 논의의 글은 이 환경에 사본이 없다 [미확인]. 주지 않는다 — 사례 카드 셋(`cases/C1.md`, `cases/C1.yaml`, `plan/ovm_evidence_20261111/g_cases_c1c2.md`, sha256), 이 절차의 채운 기대와 그 파일(25.11), 후보 목록의 종류별 기대, 설명 기록과 요약, 손 읽기 결과와 판정 규칙 5.1·8.8.

### 25.10 (3) 후보 목록과 평가 갈래 파일 [F]

판정 규칙 8.13 의 3·4·5번의 규칙으로 두 계획에서 만들었다. 손으로 고른 줄은 없다. 행 번호·키·이름·기대 값은 어느 도구의 입력도 아니다.

- **후보 목록의 규칙**(`scripts/candidates.py` 의 머리글): 행 = sidecar 의 행 줄 가운데 계획에 든 것(`state=planned` 또는 `control-state=planned`, 경로 A·B). 이름 붙은 항목 = 그 행의 relations 블록이 `P0-unverified(<f>, …)`(층 P0) 와 `sem-unverified(<api>:<column>, …)`(층 framework)로 적은 것, 그리고 그 행의 sidecar `namespace-entry` 줄이 `compare-call=`·`copy-call=`·`length-call=`·`fill-call=` 로 적은 라이브러리 호출(층 P0). 미대조 = 그 계획을 낸 facts(`cz_final_extract/<빌드>/<빌드>/NP/facts.kz`, MC `kz-dump` 로 읽음)의 SEMANTICS 기록 가운데 그 층·함수의 `verify=unverified` 인 것. (층, 함수, 열)마다 한 줄. 미대조 기록이 없는 이름 붙은 항목은 후보가 아니다. 두 빌드의 미대조 집합이 다르면 멈춘다(같았다).
- **만든 명령**: `python3 -I scripts/candidates.py --out candidates --kairos kairos --kzdump <MC>/tools/kz-dump/kz-dump <계획 디렉터리>:<relations.txt>:<facts.kz>`(c1_pre_tx, c1_post_tx 의 차례; 계획 `obs1/cz_plans_v4_ns2/<빌드>`, relations `obs1/cz_final_order/<빌드>/relations.txt`), rc 0.
- **후보 목록**: `candidates/candidates.tsv` sha256 `6a2bc34bd533cc61d191f7a6b184956c344b2277b82d8350b5483e1996435b73`, **44 줄, 종류 6**: `byte-string-comparison` 1(`strcmp` role), `data-function` 4(`memset`·`strncpy` 의 role 과 out-behaviour), `dynamic-loading` 1(`dlsym`), `framework:agreement` 35(`CFE_ES_ExitApp` … `OS_write`), `sleep` 1(`clock_nanosleep`), `termination` 2(`abort`, `pthread_exit`). 이름 붙인 줄의 종류: `sem-unverified` 35, `P0-unverified` 4, `copy-call` 4, `P0-unverified,compare-call` 1. 자료 파일: `kairos/lib/llvm/p0.cpp` 9 줄, `kairos/semantics/framework_semantics_osal-f34759d.txt` 22, `framework_semantics_cfe-6cccb3f.txt` 13(줄마다 그 파일의 sha256 과 줄 번호).
- 이름 붙었으나 후보가 아닌 것: `length-call=` 의 `memchr` 는 facts 에 SEMANTICS 기록이 없다(0; `out/candidates_named_check.txt`, 두 빌드). `fill-call=memset`(`osapi-idmap.c:120`)은 계획에 들지 않은 행의 `namespace-entry` 줄에만 있다. facts 의 미대조 SEMANTICS 기록은 빌드마다 159 이고, 후보는 그 가운데 계획된 행이 이름 붙인 것이다.
- **인정 파일**(8.13 의 4번): `candidates/admitted_D2.tsv`(44 줄 모두) sha256 `d610a779b213c0ca181bf89c90b7b021f26056cf085cdfe56ec2f445a5725ba4`, 그리고 종류 여섯의 비지 않고 전부가 아닌 부분집합마다 `admitted_D2_<종류>….tsv` 62 개. 모두 63 개, 각 파일의 `ground` 줄은 "none: evaluation branch written before the PI's answer" 다. 파일마다의 sha256 은 `candidates/admitted.sha256`.
- **평가 갈래 파일**(8.13 의 5번; 동결한 `env/obs1/explain_branches.py` `a722e4af…` 로 만듦, rc 0): `candidates/branches_16.txt` sha256 `4d06783852faf48db766cfcb6f79265f2e1703679f3bd86871714176cc943234` — `D1`, `D2` × `R0`·`Ra`–`Rf` 하나씩·여섯 모두 = 16 갈래(평가 절차 4 절의 "부팅 전에 기대를 써 두는 조합"). `candidates/branches_all.txt` sha256 `2b22b76af01932678ebbc1a9470a570b59128004c619249f7dffae13c10d1495` — 자료 갈래 64(`D1`, `D2`, 부분 인정 62) × 8 = 512 갈래.
- **부팅에서 평가하는 갈래 [D]**: 부팅마다 `branches_16.txt` 의 16 갈래를 평가한다. 평가 절차 4 절은 부분 인정 갈래도 "모두 평가"한다고 적지만, 정적 예행에서 16 갈래의 출력이 26,869,760 B 였으므로(`guard/s_explaincheck_pre.log`, c1_post_tx 26,845,184 B) 512 갈래는 부팅 하나에 약 860 MB 가 되어 바닥 위의 여유(지금 약 139 MB)에 들지 않는다. 그래서 부분 인정 갈래는 같은 기록과 같은 동결 스크립트로 자료 갈래 하나씩(8 갈래, 약 13 MB) 나중에 평가한다. 기록과 스크립트, 후보 목록, 인정 파일은 부팅 뒤에 바꾸지 않으므로 그 평가는 부팅의 지위를 바꾸지 않는다. 부분 인정 갈래의 기대는 4 절대로 25.11 의 "기대는 종류" 열에서 기계적으로 나온다.
- **점검 [M]**: 동결한 `explain.py`(`af9d4395…`)를 두 빌드의 정적 입력(실행 없음)에 파생 빌드의 결합 파일(`obs1/c1p2_<pre|post>_derived/binds_instr.kzs`)과 B′ 기초(`obs1/c1p2_<pre|post>_bprime/<빌드>/NP/basis.kz`), 이 후보 목록, `branches_16.txt` 로 돌렸다: 두 빌드 rc 0, 입력 점검 16 거부 0·못 읽은 줄 0·불일치 0, 갈래 16, 갈래마다 기록 46·거부 0(`scratch/branches16/explain_<빌드>.txt`, `input_checks_<빌드>.txt`; 출력은 요약과 sha256(`sha_<빌드>.txt`)을 남기고 지웠다). 경로 A 행의 정적 값(갈래 16 × 행): `use_cond` 는 모두 `partial`, `violation` 은 모두 `undetermined`, `link` 는 모두 `unlinked`(`scratch/branches16/static_rows_16.tsv`, 160 줄). 이 값들이 25.11 의 기대의 정적 부분의 근거다.

### 25.11 (4) 써 둔 16 갈래의 기대: 평가 절차 5절의 채운 판 [I][F]

모든 기대는 측정 전이라 [I] 이다. 기대는 판정이 아니라 틀릴 수 있는 수준(쌍마다 빠지는 항목의 목록, 사용마다 이름 상태, 행마다 사용 조건의 요소)에 적었다. 평가 절차 5절의 `[채움]` 자리를 이 절이 채운다(절차 문서 자체는 고치지 않음).

- **기대 파일**: `expect/expect_c1.tsv`(`kz-expect 1`, sha256 `d5501bf61d7ebfdf185bbadc0d89ddd75543bf9b66a93e67f1e8f038fe7fe7a3`), 2,144 줄 = 16 갈래 × 두 빌드의 경로 A 행 다섯 × 필드. 만든 것: `scripts/expect_write.py`(`e42a796a…`; 행 번호·키·값은 상수가 아니고 세 파일에서 읽음)가 (가) 동결 스크립트를 정적 입력에 다시 돌려 남긴 행·갈래별 요소 값(`scratch/branches16/static_elements_16.tsv` `94e7421c…`; 그 출력은 지움, `guard/s_explaincheck2_<빌드>.log`), (나) 손으로 쓴 부팅 예측(`expect/predictions.tsv` `44011827…`, 줄마다 근거), (다) 미리 정한 사용의 목록(25.8; 행 키와 소비자 뿌리)을 합친다. 형식 점검: 동결한 `explain_expect.py` 를 빈 기록 디렉터리에 돌려 rc 0, `no_record 2144`(`expect/form_check_no_records.txt`).
- `explain_expect.py` 의 필드는 사전의 칸 경로만 따라가므로 문맥마다의 (iii) 목록(`use_cond.branch_iii[]`)은 칸으로 쓸 수 없다. 그래서 (iii)은 판정 부분이 쓰는 글 `use_cond.caller_dependence.text`("(i) <상태>; (iii) <상태들>")로 확인하고, 문맥마다의 값은 아래 표에 적는다.
- 부팅의 순서는 통제하지 않는다. 파일의 줄은 시작 스크립트가 정하는 경우(소비자 CF 의 조회가 생성자 BP 의 생성보다 먼저; SCH_LAB 은 둘보다 먼저 시작, `ws/exe/tx/cf/cfe_es_startup.scr` 의 차례 ci_lab, to_lab, sch_lab, cf, bp)의 값이다. 다른 경우의 값은 5.3 의 표에 있고, 그 경우가 보이면 그 줄은 `mismatch` 로 나오며 표대로 읽는다.

**5.2 쌍마다의 항목** (행 A21 은 CF 의 `OS_CountSemGetIdByName`, 키 `BP0_tsem`·`BP1_tsem` 은 CF 표 `ws/bpxfer_defs/tx/tables/cf_def_config.c:40, 54`; 행 A20 은 BP 자신의 조회 `bp_flow.c:126:13`; 생성 자리 `src/bp/fsw/src/bp_flow.c:132:18`(BP, 이름 `BP0_tsem`, `BP1_tsem`)과 `apps/sch_lab/fsw/src/sch_lab_app.c:168:16`(SCH_LAB, `SCH_LAB`)). 쌍은 결과가 부재인 사용마다 행의 생성 사건 모두와 맺어진다(`explain_judge.py` 의 name-binding 쌍).

| 항목 | 같은 글, 다른 앱 (A21 × BP 의 같은 이름) | 다른 글의 생성 (A21·A20 × SCH_LAB, × 다른 throttle) | 같은 행위자의 자기 생성 (A20 × 자기 생성) | 기대는 종류 | 결합에 달린 조건 | 근거 |
|---|---|---|---|---|---|---|
| 기초 1 | held [미확인] | held [미확인] | held [미확인] | 없음 | 연결 점검의 `same_build_object` 가 main 과 그 두 모듈에 confirmed (F10) | 25.7 build-id; cFS 가 적재한 모듈에서 처음 읽음 |
| 기초 2 | held | held | held | 없음 | `pthread_create` site 254·253, 시작 루틴 site 255, 뿌리 site 286·276·6 묶임(bound) | sidecar `creation` 줄 |
| 기초 3 | held | held | held | 없음 | 키 글 site 262 | `absence: -17 …`, `absence origins` 의 `[other]` 기원은 null 시험이라 키 포인터와 글로 뺀다 |
| 기초 4 | missing | missing | missing | 없음 | — | `callee-check (success path): OS_CountSemCreate: missing(the stored value is not tied to the success path)`(A21·A20 행; lead 판단 (b)) |
| 기초 5 | held | held | held | 없음 | 키 글 site 259·262 | 글 길이 8·7 < 64 (2026-11-19 덧붙임, `semantics-07`: 한도는 64 가 아니라 20 이다 — site 259·262 의 `bound=20`(저장된 이름의 extent 에서 옴, §19.4·§19.5), `obs1/cz_plans_v4_ns2/c1_pre_tx/plan.kzs:848`, `:856`; 8·7 < 20 이라 held 는 그대로) |
| 기초 6 | held(생성이 조회 뒤) | missing(생성이 조회 전에 끝남) | held(첫 조회 뒤 자기 생성) | 없음 | — | 시작 스크립트의 차례 |
| (가) | held | held | held | 없음 | — | `namespace-entry … lookup/create/compare …:applies`, 자료 sha256 `e587cee0…` |
| (나) 의미 | D1 missing / D2 held | 같음 | 같음 | `byte-string-comparison` 과 `data-function` 이 둘 다 인정될 때 held (`memchr` 는 후보가 아님) | — | `compare-call=strcmp…`, `copy-call=memset…,strncpy…`, `length-call=memchr…` |
| (나) 한도 | held | held | held | 없음 | 키 글 site | `extent-value=20` |
| (다) | missing | missing | missing | 없음 (`Rf` 에서도: 버퍼 16 가운데 "every cause discharged" 0) | — | relations 요약 줄 `buffers 16: complete 0, not complete 16 (… causes not discharged 11; other writers 5)`, `writers [no key-parameter function reaches OS_ConsoleAPI_Init] of OS_common_record_t.name_entry … value=symbolic`(lead 판단 (c)) |
| (라) | held | held | held | 없음 | 종류 값 site 257·262 묶임 | `namespace-entry … value=3` |
| (마) | missing(사이의 다른 할당의 slot clear) | missing | held(사이에 자기 것뿐) | 없음 | 수명 끝 site 256·258·260·261·264·265 묶임, `lifetime-complete kind=OS_OBJECT_TYPE_OS_COUNTSEM yes` | sidecar `lifetime … clear-slot … not shown`(site 264) |

- **(2026-11-19 덧붙임, `semantics-02`)** 위 (마)의 `missing(사이의 다른 할당의 slot clear)` 와 5.3 의 찾은 사용의 이름 상태 `undecided(a slot clear not shown to be no lifetime end)` 는 site 264 의 저장 `osal/src/os/shared/src/osapi-idmap.c:640`(`obj->name_entry = NULL`)에 기댄다. 소스에서 그 저장은 `return_code == OS_SUCCESS`(`:633`) 아래, `active_id := token->obj_id`(`:639`) 바로 뒤에 있고, `return_code = OS_SUCCESS` 는 `!OS_ObjectIdDefined(obj->active_id)`(`:626-628`) 아래에서만 선다: 빈 레코드에서 할당이 하는 초기화이고 열린 이름을 끝내지 않는다(도구의 줄도 '(the record is allocated here)' 를 적는다, `obs1/cz_final_order/c1_pre_tx/relations.txt:627`). 이 주기에는 판정 규칙 8.13 의 13 번(빈 레코드 조건을 줄이 보이지 않는 slot clear 는 끝으로 봄)을 그대로 따른다(§26.0 (파)). 판정 부분이 구간 안의 slot clear 를 종류나 레코드로 거르지 않는 것(`env/obs1/explain_judge.py:1365-1366`)도 그 규칙대로다. 다음 relations 개정에서 경로 조건으로 빈 레코드 조건을 낸다. 행들의 결과는 이것 하나에 달려 있지 않다(기초 4 와 (다)가 모든 갈래에서 빠짐). 열린 점으로 되살렸다(25.13 열린 점 9). [§26.1]
- 빠지는 항목의 정확한 목록과 결과: 같은 글 다른 앱 — `D1` {기초 4, (나) 의미, (다), (마)} 4 개, `unlinked`; `D2` {기초 4, (다), (마)} 3 개, `unlinked`. 다른 글 — `D1` {기초 4, 기초 6, (나) 의미, (다), (마)} 5, `D2` 4, 결과 `differs_from_use_key`(두 키 글이 다 있고 다름; 기초 4 가 빠져 `refuted` 는 아님). 자기 생성 — `D1` {기초 4, (나) 의미, (다)} 3, `D2` {기초 4, (다)} 2, `unlinked`, `pair_kind = same_actor`. 기초 1 이 서지 않으면 모든 목록에 기초 1 이 더해진다(F10 으로 따로 보고). 어느 갈래에서도 `bound` 는 없다: (다)와 기초 4 는 정적 줄 때문에 모든 갈래에서 빠진다.
- 파일의 쌍 줄은 생성 자리마다의 합집합이다(`pair[<자리>].missing_items` 는 그 자리의 모든 쌍의 항목을 모음): A21 의 BP 자리 `D1` `basis4_creator_success,da_no_name_mapping,ma_no_lifetime_end,na_semantics`, 결과 `differs_from_use_key,unlinked`; A20 의 BP 자리는 둘째 조회(`BP1_tsem`)가 첫 생성 뒤라 기초 6 이 더해진다; SCH_LAB 자리는 둘 다 기초 6 을 더해 결과 `differs_from_use_key`.

**5.3 사용마다** (사용은 25.8 의 목록; 결과·문맥·이름 상태·연결)

| 사용 | 순서의 경우 | 결과 종류 | 문맥 | 이름 상태 | 연결 결과 |
|---|---|---|---|---|---|
| c1_pre_tx A21 (CF_AppMain, `cf_cfdp.c:993:19`, 문맥 1 과 2) | 조회가 먼저(기대) | absent(-17), 한 번(실패하면 `break`, `cf_cfdp.c:996-1001`) | 1(틀 site 293·292). 문맥 2(명령 `CF_CmdEnableEngine`)는 자연 부팅에서 닿지 않음 | `undecided(the row's creator sites are not complete)`(행의 `incomplete:` 줄에 `actor_not_extracted(spawn call with no resolved entry)`) | `unlinked` |
| 같음 | 생성이 먼저 | found 둘(BP0, BP1) | 1 | `undecided(a slot clear not shown to be no lifetime end)` | 열린 세대 위의 인스턴스 연결 `linked` |
| c1_post_tx A21 (`cf_cfdp.c:1003:23`, 재시도 루프) | 조회가 먼저(기대) | absent 몇 번 뒤 found | `-`(문맥 줄 없음) | absent 는 위와 같음, found 는 slot clear 로 `undecided` | absent `unlinked`, found `linked`; 행의 연결은 섞여 `unlinked` |
| 같음 | 생성이 먼저 | found | `-` | slot clear 로 `undecided` | `linked` |
| A20 (두 빌드, BP_AppMain, `bp_flow.c:126:13`) | 조회가 먼저(같은 함수, 늘 그렇다) | absent 둘(BP0, BP1) | `-` | `undecided(the row's creator sites are not complete)` | `unlinked` |
| A22 (두 빌드, SCH_Lab_AppMain, `sch_lab_app.c:176:16`), A26 (CFE_TIME_TaskMain, `cfe_time_task.c:286:16`) | 생성이 먼저(프로세스 주 스레드의 `soft_timebase_Init`) | found | `-` | `undecided(a slot clear not shown to be no lifetime end)` | `linked`, 행의 연결 `established`(instance) |
| A63 (두 빌드, 소비자 뿌리 없음, `OS_FileOpenCheck`) | — | 사건 없음(파일 보내기가 없음) | — | — | `unlinked`(`target_not_reached`) |

- **(2026-11-19 덧붙임, `semantics-02`)** 위 표의 `undecided(a slot clear not shown to be no lifetime end)` 는 5.2 의 덧붙임대로 `osapi-idmap.c:640` 의 저장(할당의 초기화)에 기댄다. [§26.1]
- **(2026-11-19 덧붙임, `semantics-04`)** 고친 빌드의 A21(`cf_cfdp.c:1003:23`, 재시도 루프)에서 루프는 25 번의 재시도 끝(`CF_STARTUP_SEM_MAX_RETRIES 25`, `cf/fsw/platform_inc/cf_platform_cfg.h:338`)이나 `break`(`:1008`)로 나가고, 그 뒤 `if (ret != OS_SUCCESS)`(`:1014`)가 `CFE_EVS_SendEvent(CF_EID_ERR_INIT_SEM, …)`(`:1016`)를 보낸다(고친 빌드의 소스 `cf/fsw/src/cf_cfdp.c:1000-1020`). facts 는 이 실패 이벤트를 놓친다: `obs1/cz_final_order/c1_post_tx/relations.txt` 에 `cf_cfdp.c:1014`·`:1016` 을 이름 대는 줄이 없고, A21 의 absence 줄(`:775`)은 `effects: retry at - open; return-result at …:1053 open` 인데 handling_label 줄(`:777`)은 `branches decided by it at 1006:21, 1014:17, 1048:9` 를 적는다. 그래서 계획(`obs1/cz_plans_v4_ns2/c1_post_tx/plan.side:1102`)의 `event-send not-planned` 의 note "no event send or system log of RESULT_EFFECT …" 는 프로그램의 성질이 아니라 도구의 빈틈(루프를 거쳐 나르는 값을 루프 뒤의 가지로 따라가지 않음)으로 읽는다. 고치기 전 빌드에서는 같은 이벤트(`cf_cfdp.c:997:17`)가 계획되어 있다(`obs1/cz_plans_v4_ns2/c1_pre_tx/plan.side:1078`, 그 계획의 site 297). 부팅에서 모든 재시도 뒤의 실패는 사용 호출의 사건으로는 보인다 [I]. 다음 kz-static 빌드로 미룬다(§26.0 (하)); 열린 점 10(25.13). [§26.1]

**5.4 행마다의 사용 조건 요소** (두 빌드 같음; 정적 확인 `static_elements_16.tsv`)

| 요소 | `R0` 의 기대(`D1` / `D2`) | 움직이는 읽기 | 행 |
|---|---|---|---|
| (c1) callee-check | present / present | — | 모두 |
| (c2) flip | missing / missing(A21 은 `flip: static-undetermined`) | `Rc` 와 all 에서 present(A20, A21, A22, A26); A63 은 모든 갈래에서 missing | |
| (c3)(i) | A21·A20 `not_held`(A21: 지우는 채움 `cf_cfdp.c:965:5`; A20: 같은 함수의 생성 out 저장), A22·A26·A63 `not_established` | `Rb` 와 all 에서 A21 은 `not_established`(지우기를 빼도 FIELD_STATUS DISABLED); `Re` 는 움직이지 않음(24.4 (라)) | |
| (c3)(iii), 문맥마다 | c1_pre_tx A21: 문맥 1 `not_established`(열린 효과, 저장값 규칙이 전제에 기댐), 문맥 2 `not_established`(호출자에게 돌아감). 다른 행: 부팅에서는 `not_established` — 아래의 읽기 문제 | 이 16 갈래 어디서도 움직이지 않음 | |
| (c4) creator | missing / missing | `Rd` 와 all 에서 present(전제 표지 `reading Rd`). `Rd` 가 받는 줄: 행의 `creator:` 줄들(A21: `bp_flow.c:132:18`, `sch_lab_app.c:168:16`)과 그 `incomplete:` 사유(`key_unresolved(creator key)`, `actor_not_extracted(…)`), 쌍 파일의 그 쌍의 줄(`unordered`; A20 의 자기 생성은 `creator-not-a-pair(probe-create) … same_thread`) | 모두 |
| `use_cond.status`, `violation` | **16 갈래 모두, 두 빌드의 행 다섯 모두 `partial`, `undetermined`** | — | |

- **읽기 문제(부팅 전에 찾음, 고치지 않음)**: 동결한 판정 부분은 sidecar 에 `context` 줄이 없는 행의 사용 사건에 문맥 `"-"` 를 준다(`env/obs1/explain_judge.py` 의 `use_context`: 문맥 줄이 없으면 `"-"`). 그런데 그 행들의 alternatives 줄은 `[context 1: …]` 이라 부팅에서는 `branch_iii` 가 "no alternatives line of context -" 로 `not_established` 를 낸다(같은 파일 `branch_iii`). 정적 예행(실행 없음)은 alternatives 줄의 문맥을 그대로 써서 `not_held` 였다(24.4 표; 여기 `static_elements_16.tsv`). 그래서 c1_post_tx A21 의 `found: retry` 와 A20 의 자기 생성 `not_held` 는 부팅 기록에서 `not_established` 로 나오리라 본다 [I: 코드 읽기]. 기대 파일은 동결본이 낼 값(`(iii) not_established`)을 적었다. 이것은 판정 부분의 일이라 고치려면 다시 동결해야 한다(평가 절차 12 절 6). 열린 점으로 lead 에게 넘긴다(25.13).

**5.5 다른 대조 행**: 시간 기준 행 A22·A26 — 생성자는 프로세스 행위자(`main (process)`, `psp/…/cfe_psp_soft_timebase.c:53:14`)라 `pair_kind = process-app`, 쌍 파일의 그 줄은 `unordered`(정적 순서 간선을 이 개정이 찾지 못함), 사용은 찾고(found) 이름 결합 규칙은 돌지 않으며, 인스턴스 연결은 `lifetime-complete kind=OS_OBJECT_TYPE_OS_TIMEBASE yes` 와 수명 끝 site 266–271·264·265 의 묶임으로 선다 [I]. 생성 사건이 기록되지 않으면(프로세스 주 스레드의 PSP 모듈 초기화가 기록보다 앞서면) 연결은 `unlinked(no create of the id in the record)` 이고 F4 가 어긋난다. 파일 이름 행 A63 — 행위자 뿌리가 사용에 닿지 않고(`actor_not_extracted(no root reaches the use)`) 부재 값이 평가되지 않으며(`alternatives [context -]`), 자연 부팅에서 사용이 닿지 않는다: 모든 항목이 빠지고 연결은 `target_not_reached`.

**(2026-11-19 덧붙임, `semantics-06`)** A63 의 '행위자 뿌리가 사용에 닿지 않음'(`actor_not_extracted(no root reaches the use)`; 쌍 파일 `obs1/cz_final_order/c1_pre_tx/pairs.tsv:81` 의 `unknown(no root reaches CF_CFDP_R2_RecvMd)`)은 프로그램의 성질이 아니라 도구의 빈틈이다. relations 는 `CF_CFDP_R2_RecvMd`(`obs1/cz_final_order/c1_pre_tx/relations.txt:1186`, `:1204`, `:1224`)와 `CF_CFDP_S_SubstateSendMetadata`(`:1244`)를 'direct callers 0, called through a function pointer: no' 로 적지만, 소스(고치기 전 트리의 `cf/fsw/src/`)에서 둘은 함수 안의 `static const` 분배 표의 항목이다: `cf_cfdp_r.c:902`(`CF_CFDP_R2_Recv` 안의 `r2_fdir_handlers_normal`, `:899`), `cf_cfdp_s.c:650`·`:668`(`CF_CFDP_S1_Tx`·`CF_CFDP_S2_Tx` 안의 `substate_fns`, `:648`·`:666`). 도구는 함수 범위 정적 객체의 초기값에 든 함수 주소를 간접 호출의 대상으로 모으지 않는다. A63 은 어차피 미달로 기대되어(7 절의 `expect_discriminate`) 결과는 바뀌지 않는다. 다음 kz-static 빌드로 미룬다(§26.0 (하)). [§26.1]

**6 절의 점검에 대한 기대**: F1 은 기대 파일의 줄마다(`match`). F2 `D1` 에 `bound` 쌍 없음, 미대조 항목에 기대어 `held` 인 항목 없음 — 지남. F3 같은 종류·같은 글의 생성이 끝난 뒤 수명 끝 없이 부재인 사용 없음 — 지남(시작 스크립트의 경우에서 그런 사용이 없음). F4 찾은 사용마다 그 id 의 생성 — 지남(A22·A26 의 PSP 생성이 기록될 때). F5 — 글로는 지나지만(고친 행과 탐침 행의 (iii)이 `held` 가 아님, 탐침 행의 (i)이 `not_held`) 위의 읽기 문제 때문에 (iii)은 `not_held` 가 아니라 `not_established` 로 지난다. F6 저장값 규칙으로 선 (iii) 없음(`stored_field_rule` 비어 있음) — 점검할 것 없음. F7 `derived`·`observed` 없음 — 지남. F8 쌍 파일의 줄이 있음(A21 둘, A20 둘, A22·A26 하나씩) — 지남. F9 c1_pre_tx A21 의 사용마다 문맥 하나(틀 site 293·292·305·304 bound) — 지남. F10 기초 1 — [미확인].

**7 절의 두 기대 (대조 행마다)**: `expect_safe` (s1) `D1` 에 `bound` 쌍 없음, (s2) 대조 행에 `held` 인 (c3) 갈래 없음, (s3) 열린 세대 없이 선 인스턴스 연결 없음, (s4) 다섯 조건 없이 난 refuted 없음 — 대조 행 다섯(A20, A21, A22, A26, A63) × 빌드 둘 × 넷 = 40 점검이 모두 지나리라 본다. `expect_discriminate`: 고친 빌드의 행(c1_post_tx A21) **미달**(재시도 줄은 있으나 부팅의 (iii)이 `not_established`, 위의 읽기 문제), 탐침 행(A20) **미달**((i) `not_held` 와 둘째 정의 줄은 나오나 (iii)이 같은 까닭으로 `not_established`), 시간 기준 행(A22·A26) **충족**(열린 세대 위의 인스턴스 연결), 파일 이름 행(A63) **미달**(낼 근거가 없음). 두 수치는 합치지 않는다.

**8 절의 분모의 기대**: 계획된 행 / 추출된 행 — c1_pre_tx 층 9 + 대조 5(A21 은 둘 다) / 367, c1_post_tx 층 8 + 대조 5 / 367(`out/plan_budgets.txt`). 부팅마다 닿은 사용 / 정한 사용 — c1_pre_tx 4 / 6(A21 의 문맥 2 와 A63 이 닿지 않음), c1_post_tx 4 / 5(A63). 판정된 사용 / 닿은 사용 — 0 / 4: 모두 미확정이고 종류는 "정적 입력 없음"(`absent_result_unevaluated`, `key_unresolved`, `writer_set_incomplete`, `fixed_proof_open`, `caller_dependence_unknown`)과 "구조"(`link_unlinked`). 정밀도와 계획 안의 재현율은 정의되지 않음(25.9).

### 25.12 (5) 부팅 선언 (부팅 전에 커밋함; 이 단계는 부팅하지 않음) [D]

**선언의 목록**: `boot/declaration.tsv`(112 줄; 아래의 모든 sha256 과 build-id 를 경로와 함께 담음). 부팅을 하는 단계는 부팅 전에 이 목록을 다시 재어 같음을 보이고, 다르면 부팅하지 않는다.

- **파생 빌드의 정체성**(kzgcc 0.5 `5e691ce7…`, 런타임 `kzrt 0.5.0`, 런타임 트리의 `kz-rt-dump` `d55a9081…` 둘 다): c1_pre_tx 는 `obs1/c1p2_pre_derived`(`build_instr.record` `f2e4ed02…`, 맵 `path_map.tsv` `6590dbe3…`), c1_post_tx 는 `obs1/c1p2_post_derived`(`234eab8e…`, `7b5663f4…`). 설치된 ELF 21 개씩의 sha256 과 GNU build-id 가 목록의 `elf` 줄이다. 부팅 기록의 `buildid` 줄이 이름 댈 객체(25.3 의 목록)와 그 build-id: c1_pre_tx core-tx `619614002e6d4556fa731365d88ee7f0d49c00cf`, bp.so `01db56cfd8f7a50b03665e022ca4c835f8f9fd8b`, cf.so `c55b464be7b96ec2d0001b1a5ea296740c13e5c0`, ci_lab.so `4074a88b…`, sch_lab.so `310dfddb…`, to_lab.so `1c6beaba…`; c1_post_tx core-tx `acee9edd432e2e987903efefe7fc0c19bdee41ac`, bp.so `f359e98bd5420438130647946f80fd562e03edf7`, cf.so `3eb1b8b3762d84c5212a50a1e4ef09f85ee43cd7`, ci_lab.so `8dc25470…`, sch_lab.so `14e71ebd…`, to_lab.so `a79dc818…`(줄임 없는 값은 목록).
- **계획과 sidecar**: c1_pre_tx plan-id `943b8cc0d0e200e4`, `plan.kzs` `69a278b019281fd3015226b2ac188be0702613eb2a950fc12627c2603b53c9a0`, `plan.side` `a18a03702a2e339696a17e187d919d8f9a03694d20a9b268844e4f1f11abddc4`; c1_post_tx `b99c203f12de1e24`, `ef35173ee767be1afbfa5d786c8f473f7505e325cacad3884796adc864655593`, `4ef73c918ac41cdc5e41be1dcc12b996557b0f954208ea80d06d1250dcfbd0e9`. 계획을 낸 facts·basis(`cz_final_extract`), relations·keys·쌍 파일(`cz_final_order`), facts 보기가 목록에 있다.
- **결합 파일**: c1_pre_tx `obs1/c1p2_pre_derived/binds_instr.kzs` `5ed4ddb8b7344db4a445c5273d096d65a5a70bfbb81f670cdccb17eafab448eb`, c1_post_tx `obs1/c1p2_post_derived/binds_instr.kzs` `b719efeea53fec55d5b852380d81b75ddfb105a3bbbe4511027f1318af627a2f`(묶이지 않은 site 와 까닭은 25.7). B′ 기초: c1_pre_tx `basis.kz` `65c6b02e…`, c1_post_tx `324cf73e…`.
- **동결한 스크립트**(`records/closure-explain-rev4-20261119/freeze.tsv` `733feef7…`): 읽는 부분 `explain_read.py` `ff0a06252cf74db9862588913384b0dd82989636b2b22e18c7b5c9f1b2d97752`, 판정 부분 `explain_judge.py` `531a6b3e91ba3e574c9263d2eb9307a282dd06f09dcbf9ee8e8f10717afc4587`, 구동부 `af9d4395…`, 갈래 파일 생성 `a722e4af…`, 요약 `17c3a9eb…`, 기대 대조 `442d2e7d…`. 부팅 뒤 읽는 부분만 고친 것은 부팅의 지위를 바꾸지 않고, 판정 부분을 고치면 그 부팅들은 "규칙을 정하기 전에 본 기록"이 된다(평가 절차 12 절).
- **주기의 파일**: 후보 목록 `6a2bc34b…`, `admitted_D2.tsv` `d610a779…`, 인정 파일 목록 `admitted.sha256`, 갈래 파일 `branches_16.txt` `4d067838…`(부팅마다 평가)·`branches_all.txt` `2b22b76a…`, 미리 정한 사용 `0ca47839…`(c1_pre_tx)·`955890c2…`(c1_post_tx), 판독지 `e70c31f2…`·파일 목록 `80313f04…`, 기대 `expect_c1.tsv` `d5501bf6…`·예측 `44011827…`, 부팅 스크립트 `boot/run_cycle.sh`(sha256 은 목록).
- **도구**: MC `kz` `94cba72b…`, `kz-dump` `32a96ae7…`, `chain_check.py`, `stop_eval.py`, `run_gi_c1.sh`, `space_guard.sh`(목록).

**문**(평가 절차 3 절; `run_cycle.sh gate` 가 기계적으로 씀): (1) 제어 기록 점검이 `check ok`, (2) 기록 풀 넘침이 없고(`overflow`·`pool_dropped` 0) 다른 손실은 종류와 수를 `gate.txt` 둘째 줄에 이름 붙임(그 필요는 기록에서 `unknown-loss`), (3) `chain confirmed`. `same_build` 는 문에 없다(쌍마다 기초 1).

**부팅 수의 규칙**(§12.10 4 항; 평가 절차 2 절; `run_cycle.sh` 의 `allowed` 가 지킴): 빌드마다 첫 부팅을 한다. 첫 부팅이 문을 지나면 그 빌드를 정확히 네 번 더(2–5) 부팅한다. 첫 부팅이 무엇을 보였든 같다. 지나지 못하면 원인을 `boot/<빌드>/addendum_1r.txt` 에 적어 커밋한 뒤 한 번만 다시 한다(1r). 1r 이 문을 지나면 그것을 첫 부팅으로 보고 네 번 더 한다. 이 읽기(평가 절차 13 절의 열린 점)는 lead 의 확인이 필요하다. 1r 도 지나지 못하면 그 빌드의 부팅은 거기서 끝난다. 기록을 본 뒤에 수를 올리거나 내리지 않는다. 한 부팅은 모두 세고, 문을 지나지 못한 부팅은 사유와 함께 나열한다. 차례는 c1_pre_tx 의 부팅들, 그다음 c1_post_tx 의 부팅들이다.

**더 한 네 부팅에서 읽는 것**은 둘뿐이다: 미리 정한 사용마다의 부팅별 분포(observed / not observed / undetermined 의 수), 그리고 쌍의 다른 순서가 보였는지(`other_order_seen_in_boots`). 시작 스크립트가 소비자(cf)를 생성자(bp)보다 먼저 적으므로 모든 부팅에서 순서가 같으리라 본다 [I]. 그 결과는 다른 순서에 대해 아무것도 보이지 않는다. 정적 요소와 입력이 빠진 항목은 부팅을 더 해도 바뀌지 않는다.

**설계 행은 지금의 규칙 아래에서 모두 미확정으로 기대한다** [I]: 16 갈래 모두에서 경로 A 행의 `use_cond` 가 `partial` 이라(`derived` 없음) `violation` 은 어느 부팅에서도 `observed` 가 될 수 없다(25.11; 정적 확인 `static_elements_16.tsv`). 막는 것: (c2) `flip: static-undetermined`(키가 정적으로 풀리지 않음; `Rc` 에서만 present), (c3)(i) 엔진을 지우는 채움이 다른 정의이고 writer 집합이 FIELD_STATUS DISABLED(`Rb` 에서도 not_established), (c3)(iii) 초기화 문맥의 열린 효과와 저장값 규칙의 전제 (6)(lead 판단 (a); `Re` 가 움직이지 않음), 부팅에서는 문맥 줄이 없는 행의 문맥 `"-"` 읽기(25.11), (c4) 생성자 키가 풀리지 않고 행위자가 추출되지 않음(`Rd` 에서 전제로만 present). 쌍 수준에서는 기초 4(성공 경로 줄 `missing`, lead 판단 (b))와 (다)(버퍼 줄 complete 0, lead 판단 (c))가 모든 갈래에서 빠져 `bound` 가 없고, (마)는 slot clear 가 막는다.

**부팅의 명령**(쓰기만 하고 돌리지 않았다): `boot/commands.txt` — 빌드마다 `run_cycle.sh boot|control|convert|kzt-text|chain|stop|gate|explain|expect <빌드> <N>` 아홉 단계를 N = 1 에, 문을 지나면 N = 2–5 에, 끝에 `summary`. 부팅 단계는 root 가 필요하다(`run_gi_c1.sh`). 출력은 모두 새 디렉터리(`obs1/c1cyc_<pre|post>_{boot,convert,explain}<N>`, 기록은 `boot/<빌드>/boot<N>/`)이고, 있는 디렉터리면 거부한다. **기대 파일은 어느 입력도 아니다**: 계획 생성, 부팅, 변환, 연결 점검, 멈춤 평가, 설명 스크립트의 인자에 기대 파일과 예측 파일이 없고, `expect` 단계만 그것을 읽는다(`boot/no_expect_input_check.txt`). `bash -n` 구문 점검 rc 0.

**공간**(부팅 단계의 몫, [I]): 앞 부팅의 출력은 부팅 3.6 MB, 변환 0.8 MB 였고(`obs1/c1pre_boot`, `obs1/c1pre_convert`), 16 갈래의 설명 출력은 정적 입력에서 26.9 MB 였다. 부팅 하나에 약 32 MB(부팅 기록이 든 설명 출력은 더 클 수 있음), 두 빌드에 최대 12 부팅이면 약 384 MB 로 지금 바닥 위의 여유(약 128 MB, 25.13)에 들지 않는다. 부팅 단계는 첫 부팅 전에 공간의 계획을 선언해야 한다(열린 점).

### 25.13 공간, 끝 점검, 계획과 다르게 한 것, 열린 점

- **공간 [M]**: 시작 541,810,688 B(`out/free_start.txt`), 옵션 끈 추출 넷을 xz 묶음으로 바꾼 뒤 713,269,248 B(25.1), 끝 440,836,096 B(`out/free_end.txt`, 09:57Z), 바닥 314,572,800 B 위의 여유 126,263,296 B. 감시 기록의 가장 낮은 값 393,605,120 B(`guard/s_explaincheck2_post.log`). 압축 허용은 그 넷에만 썼다. scratch 는 이 단계가 만든 것만 지웠다(정적 점검 출력 둘씩, 빈 디렉터리, 명령 파일).
- **끝 점검 [M]**: 원장 `ledger_check` 193 행 실패 0(`out/ledger_check_end.txt`), 등록부 `tools_check --deep` 57 행 실패 0(`out/tools_check_deep_end.txt`). 25.7 뒤로 도구와 obs1 출력은 바뀌지 않았다(새 출력은 이 기록 디렉터리의 작은 파일뿐). 커밋마다 이름 점검(`--cached`, 새 디렉터리 `--tree`)이 깨끗했다.
- **계획과 다르게 한 것**:
  1. 이 단계 안에서 도구 하나를 지었다: kzgcc 0.4 가 GCC 를 멈춰(25.4) kzgcc 0.5 를 새 디렉터리에 선언하고 지었다(25.5–25.6). 첫 파생 빌드는 시도 기록으로 남겼다.
  2. 비교기를 고쳤다(`decl_fh`·`effect_fh` 색인, 새 갈래 `treetok`·`pathlen`). 덧붙임 1·2 를 다시 하는 비교보다 먼저 커밋했다(25.6).
  3. 커밋의 차례는 이 단계의 지시대로 (1) 사용 목록, (2) 판독지, (3) 후보 목록과 갈래 파일, (4) 기대, (5) 부팅 선언이다. 평가 절차 12 절은 후보 목록을 사용 목록보다 먼저 둔다. 셋은 서로를 입력으로 쓰지 않고 계획에서 규칙으로 나오므로 내용은 차례에 달리지 않는다.
  4. 부팅마다 16 갈래만 평가한다. 부분 인정 갈래(512 갈래 파일)는 공간 때문에 같은 기록에서 자료 갈래 하나씩 나중에 평가한다(25.10).
  5. 평가 절차 5 절의 채운 판은 절차 문서가 아니라 25.10–25.11 과 기대 파일에 있다.
  6. (iii)의 기대는 `use_cond.caller_dependence.text` 로 적었다. 기대 대조 스크립트는 목록 칸에 들어가지 못한다(25.11).
  7. 큰 비교 출력은 `obs1/c1p2_compare` 에 두고 요약만 이 디렉터리에 남겼다.
  8. **(2026-11-19 덧붙임, `builds-01`)** kzgcc 0.5 빌드 뒤 점검 셋을 선언 25.5 (나)대로 하지 않았다: 플러그인 빌드 바로 뒤의 `ledger_check.sh` 없음, 등록부 점검이 `--deep` 없이 돎, 플러그인 첫 ctest 가 감시 밖. 그 뒤의 첫 점검(`out/ledger_check_after_builds.txt` 193 행, `out/tools_check_deep_after_builds.txt` 57 행 `--deep`, 모두 실패 0)은 지났다. 25.6 (가)의 덧붙임. [§26.1]
- **열린 점**:
  1. **문맥 `"-"` 읽기**(25.11): 동결한 판정 부분은 sidecar 문맥 줄이 없는 행의 사용에 문맥 `"-"` 를 주고, alternatives 줄은 문맥 1 을 이름 댄다. 그래서 부팅에서 고친 빌드의 행과 탐침 행의 (iii)이 `not_established` 가 되고 그 둘의 `expect_discriminate` 는 미달로 예측된다. 고치려면 판정 부분을 바꾸고 fixture·회귀·예행을 다시 하고 다시 동결한 뒤 이 선언을 새로 커밋해야 한다(첫 부팅 전). lead 의 판단.
  2. 다시 한 부팅 뒤의 네 번(평가 절차 13 절): 이 선언은 "1r 이 지나면 네 번 더"로 읽었다. lead 의 확인.
  3. 부팅의 공간: 최대 12 부팅에 약 384 MB [I], 여유 약 126 MB. 첫 부팅 전에 공간의 계획이 필요하다.
  4. `same_build`: build-id 를 읽는 방식은 cFS 가 적재한 모듈에서 아직 돈 적이 없다 [미확인](F10).
  5. 묶이지 않은 site: 242(`aggregate operand larger than 8 bytes`; 층 행 B119·B125 의 after-call), c1_post_tx 297(`wait-exit`, `no statement on line 1008 of the function`).
  6. lead 판단 (a) `Re` 와 전제 (6), (b) 성공 경로의 빈자리, (c) 버퍼가 complete 가 아님은 그대로 열려 있다. 읽는 사람은 정해지지 않았다(PI 결정). 원 이슈의 글은 이 환경에 없다 [미확인].
  7. A22·A26 의 생성(프로세스 주 스레드의 PSP 모듈 초기화)이 기록되는지 [I]. 기록되지 않으면 F4 가 어긋나고 시간 기준 행의 `expect_discriminate` 도 미달이다.
  8. kzllvm 에 같은 `out<k>` 집합 피연산자의 꼴이 있는지 보지 않았다. B′ 에는 소스 트리 토큰이 없다(D2 사본에 git 메타데이터가 없음; `treetok`).
  9. **(2026-11-19 덧붙임, `semantics-02`)** `osapi-idmap.c:640` 의 저장(site 264, 할당의 초기화)과 relations 의 key-clearing 이름(§12.11 (사), §15.7, §18.8, §19.5, §22.9 가 lead 의 판단으로 넘긴 것)이 이 목록에서 빠졌었다. 되살린다. lead 의 판단 §26.0 (파): 이 주기에는 판정 규칙 8.13 의 13 번대로 끝으로 보고, 다음 relations 개정에서 경로 조건으로 빈 레코드 조건을 낸다. 그때까지 25.11 (마)와 찾은 사용의 이름 상태는 이 저장에 기댄다(25.11 5.2 의 덧붙임). [§26.1]
  10. **(2026-11-19 덧붙임, `semantics-04`)** 고친 빌드의 A21 에서 재시도 루프 뒤의 실패 이벤트(`cf_cfdp.c:1014`·`:1016`)를 facts 가 놓쳐 계획의 need note 가 'no event send' 라고 적는다(25.11 5.3 의 덧붙임). 도구의 빈틈이고 다음 kz-static 빌드가 다룬다(§26.0 (하)). [§26.1]
- **다음 단계가 할 일**(차례): (1) 열린 점 1 의 lead 판단. 고치면 판정 부분의 고침, fixture 127, 2026-11-16 입력의 회귀, 두 빌드의 정적 예행, 다시 동결, `boot/declaration.tsv` 와 25.12 를 새로 커밋. (2) 열린 점 2·3 을 선언에 덧붙여 커밋. (3) `boot/commands.txt` 를 c1_pre_tx 에, 다음에 c1_post_tx 에 돌린다(root).

## 26. C1 주기: 부팅 전 고침 (2026-11-19, 첫 부팅 전)

### 26.0 lead 의 판단 (2026-11-19; 이 절의 다른 글과 어느 코드보다 먼저 커밋함)

근거: 25절의 끝 점검과 열린 것, 주기 준비 뒤의 세 검토(규칙·빌드·의미)와 그 검증(`records/closure-review-20261119/`; 아래의 `rules-NN`, `builds-NN`, `semantics-NN` 은 그 목록의 번호), 그리고 같은 날의 PI 결정(`Architecture/spec/PI_DECISIONS_RESOLVED_20261119.md`). 부팅은 아직 하지 않았다. 아래가 모두 커밋되기 전에는 첫 부팅을 하지 않는다.

**(가) PI 결정의 반영과 차례.** PI 결정 기록, 판정 규칙 8.14, 평가 절차 14절, 설명 스크립트 명세 11절을 이 차례로 커밋한 뒤에 코드를 고친다. 부팅 선언 `860c2cd` 는 이 글들보다 먼저 커밋되었다. 평가 절차 12절의 차례(규칙 글 → … → 부팅 선언)를 지키려고, 고친 동결과 고친 기대 뒤에 부팅 선언을 새로 커밋하고 그 선언이 `860c2cd` 를 대신한다. 이 어긋남은 평가 절차 14절에도 적는다.

**(나) 문맥 `-` (`rules-01`).** 동결한 판정 부분이 sidecar 에 문맥 줄이 없는 행의 사용을 문맥 `-` 로 두는 것은 판정 규칙 8.13 의 16번(문맥은 한 사용에 여러 호출 사슬이 닿을 때만 가른다)과 어긋나는 코드 결함이다. 읽기의 문제가 아니다. 고친다: 문맥 줄이 없는 행에서는 그 행의 대안 줄이 이름 붙인 문맥이 정확히 하나일 때 그 문맥을 쓰고, 하나가 아니면 문맥을 모름(16번)으로 둔다. 없는 문맥 번호를 만들지 않는다. 문맥 줄이 없는 한 문맥 행의 fixture(찾음, 찾지 못함, 세워지지 않음)를 더하고, fixture 전부, 2026-11-16 입력의 회귀, 실제 두 계획 위의 합성 사용 사건을 넣은 예행을 다시 한 뒤 다시 동결한다.

**(다) 항목이 없는 라이브러리 호출 (`rules-04`).** 계획의 줄이 이름 붙인 라이브러리 호출에 P0 항목도 의미 자료 항목도 없으면(`memchr`), 그 호출에 기대는 항목은 **모든 자료 갈래에서** 성립하지 않고 사유는 '항목 없음'이다. 규격(ISO C)의 글은 이름 붙인 전제로 따로 적을 뿐 어느 자료 상태에서도 근거가 아니다(판정 규칙 8.13 의 원칙 (2); PI 결정 ⑤). 그런 호출은 후보 목록(8.13 의 3번)에 넣지 않는다: 그 목록은 미대조 항목의 목록이고, 이것은 항목이 없는 것이다. 판정 규칙 8.14 에 적는다.

**(라) 찾은 사용의 이름 상태와 연결 (`semantics-01`).** 기록에 그 id 를 만든 세대가 없는데 조회가 찾은 사용은 이름 상태가 `unmet` 일 수 없다(찾았다는 사건과 어긋난다). 그때 이름 상태는 미정(사유: 기록에 그 id 의 생성이 없음, 또는 행의 생성자 site 가 완전하지 않음)이고 인스턴스 연결은 `unlinked` 다. 판정 부분을 그렇게 고친다. 기대는 규칙에서 다시 쓰고, 고친 판정 부분을 합성 기록에 돌린 예행과 맞대어, 어긋나는 줄마다 판정 부분의 결함인지 기대의 잘못인지를 가려 적는다. 기대를 예행 출력에서 베끼지 않는다.

**(마) 반증 점검과 평가의 스크립트 (`rules-02`).** 평가 절차 6–7절의 F1–F10, 대조 행의 s1–s4, expect_discriminate, 그리고 평가 절차 14절의 정밀도·계획 안의 재현율·미확정 분해를 계산하는 스크립트를 부팅 전에 쓰고, 검사마다 실패하는 fixture 를 붙이고, 동결해 sha256 을 동결 파일과 새 부팅 선언에 적고, 실행 스크립트의 설명 단계 뒤에 넣는다. 이 스크립트는 F1 을 빼고 기대 파일을 읽지 않는다. 기록을 쓰지 않고 Kairos 의 입력이 아니다.

**(바) 틀릴 수 있는 것과 아닌 것 (`rules-03`).** 평가 절차 14절에서 점검과 기대의 칸마다 '부팅으로 틀릴 수 있음' 또는 '정적·형식'으로 가른다. 반증 점검과 expect_safe 의 결과로는 앞의 것만 센다. 정적 입력이 미리 정하는 점검(s1, s2, s4, F2 의 bound 쪽, F6, F7)과 기대 파일에서 정적 출력을 되비추는 줄은 따로 센다.

**(사) 기대는 목록 항목 (`rules-05`).** (c1), (c2), (c4), (c3)(i)과 이름 결합 항목 (다)에 그 근거가 기대는 목록 항목(없으면 '없음')을 자료 갈래마다 적고, 형식 점검 16 을 그것까지 넓힌다. 다시 동결하는 김에 넣는다. 이 주기의 상태는 바뀌지 않으리라 본다 [I].

**(아) 부팅 전 다시 재기 (`builds-04`).** 실행 스크립트의 첫 단계로 부팅 선언의 모든 sha256·build-id 줄을 다시 재고, 하나라도 다르면 부팅하지 않는다. 설명 단계가 읽는 `reason_vocabulary.txt` 와 이름 공간 자료 다섯을 선언에 더한다.

**(자) 요약의 주·보조 표지.** 명세 11절의 요약 개정(갈래마다 주·보조 표지, 사용별 표, 쌍 종류)을 이번 동결에 함께 넣는다. 부팅 뒤에 요약을 고쳐 다시 내는 길(평가 절차 12절 6)은 쓰지 않는다.

**(차) 공간 감시 (`builds-06`).** `space_guard.sh` 는 명령이 끝난 뒤 잰 값이 바닥 아래이면 따로 판정(`below_floor_at_end`)과 0 이 아닌 rc 를 낸다. 등록부의 줄을 고친다. 결과 문서는 판정 대신 최저값을 인용한다.

**(카) 부팅을 위한 공간.** 부팅 수 규칙(평가 절차 2절)은 바꾸지 않는다. 공간은 바이트를 보존하는 압축으로 낸다(§12.10 2항의 허용; `f9ce046` 과 같은 방법): 부팅 단계가 읽지 않는 중간 추출 회차들을 xz tar 로 바꾸고, 파일마다 왕복 대조하고, 장부 줄을 고친다. 어느 회차를 바꿀지와 필요한 양(부팅 12 번의 [I] 와 여유 100 MB)을 먼저 커밋하고, 바꾼 뒤 잰다. 앞서 바이트가 같다고 적힌 `rf2_pre_relcheck`·`rf2_pre_order`·`rf2_pre_final_order` 는 다시 재어 같으면 `reclaim-20261118` 의 규약대로 걷는다. 하나뿐인 바이트를 지우지 않는다. 그래도 모자라면 부팅하지 않고 PI 에게 묻는다.

**(타) 다시 한 부팅의 읽기.** 평가 절차 2절 3 은 선언이 읽은 대로다: 다시 한 첫 부팅이 문을 지나면 그 빌드를 정확히 네 번 더 부팅한다(13절 첫 줄을 닫는다).

**(파) `osapi-idmap.c:640` 의 저장 (`semantics-02`).** 이 주기에는 판정 규칙 8.13 의 13번(빈 레코드 조건을 줄이 보이지 않는 slot clear 는 끝으로 본다)을 그대로 둔다. 보수적이고 규칙대로다. 열린 것으로 되살리고, 25.11 의 (마)와 찾은 사용의 이름 상태가 소스가 할당의 초기화로 보이는 저장(`osapi-idmap.c:626-640`)에 기댄다는 것을 적는다. 다음 relations 개정에서 경로 조건으로 빈 레코드 조건을 낸다.

**(하) 다음 kz-static 빌드로 미루는 도구의 한계.** `rules-06`(종료 효과의 `rests on` 이 감싼 함수 아래의 P0 미대조 항목을 적지 않음), `semantics-03`(strncpy 뒤의 NUL 종결 저장을 lifetime end 로 셈; C1 의 clear-slot 10 가운데 이름을 지우는 것은 2), `semantics-04`(고친 빌드의 재시도 뒤 실패 이벤트를 facts 가 놓침), `semantics-05`(HB-await 간선이 모든 writer 와 모든 대기 task 를 짝지음; C1 의 284 가운데 소스가 허락하는 것은 생성자→생성된 task 의 60), `semantics-06`(함수 안 정적 디스패치 표의 함수를 함수 포인터 대상으로 보지 않음). 검증은 이 주기의 계획된 행의 결과를 어느 것도 바꾸지 않는다고 했다. 해당 절(12.11 (라), 21.5, 22.6, 25.11)에 날짜 붙인 주를 단다.

**(거) 기록의 보완.** `builds-01`, `builds-02`, `builds-03`, `builds-05`, `builds-07` 은 해당 절에 이탈로 적고, `semantics-07` 은 25.11 의 근거를 고친다. `rules-07`, `rules-08` 은 이미 적혀 있어 손대지 않는다.

**(너) 바꾸지 않는 것.** 후보 목록, 갈래 파일, 미리 정한 사용, 판독지('채우지 않음: 읽는 사람 없음'), 파생 빌드, 계획은 그대로다. 기대 파일과 예측은 (나)·(다)·(라)에 따라 다시 쓰고, 부팅 선언은 다시 커밋한다.

**(더) PI 에게 남는 것.** 읽는 사람(첫 부팅 전에 정해지면 판독지와 파일 목록과 부팅 선언을 다시 커밋한다), 기초 2 가 프로세스의 시작 스레드가 만든 것에서 설 수 없는 문제, 읽기 `Re` 가 (iii)을 움직이게 하는 도구 빌드, 그리고 PI 결정 기록이 PI 확인으로 남긴 것.

### 26.1 검토 지적의 날짜 붙인 주 (2026-11-19; §26.0 (하)·(거), 부팅 없음)

**무엇.** §26.0 (하)·(거)가 이름 댄 닫기 검토의 지적마다, 그것이 닿는 절에 날짜 붙인 주 '(2026-11-19 덧붙임, <번호>)' 를 달았다. 이 절과 그 주의 `rules-NN`·`builds-NN`·`semantics-NN` 은 `../records/closure-review-20261119/findings.json` 의 번호다(§12 이전의 검토가 쓴 같은 꼴의 번호와 다르다). 앞의 글은 지우거나 고치지 않고 주만 더했다(25.11 5.2 의 기초 5 는 표의 칸 안에 주를 더함). 주에 든 수는 남긴 파일에서 다시 읽었고(`../records/closure-notes-20261119/`), 인용한 소스·출력의 줄은 모두 그 파일에서 다시 보았다. cFS 를 부팅하지 않았고, 코드·자료·계획·동결·기대·부팅 선언은 건드리지 않았다. push 하지 않았다.

| 번호 | 주를 단 곳 | 요지 |
|---|---|---|
| `builds-01` | 25.6 (가), 25.13 계획과 다르게 한 것 8 | kzgcc 0.5 빌드 뒤 점검 셋(바로 뒤의 원장 점검, 등록부의 `--deep`, 감시 아래의 플러그인 ctest)을 선언대로 하지 않음; 뒤의 첫 점검은 지남 |
| `builds-02` | 12.11 (바) | 측정이 비교 도중과 쌍둥이 비교가 멈춘 뒤에 돎; 읽은 출력은 뒤의 `twin_2`·`declared` 가 확인 |
| `builds-03` | 12.11 (바), 21.7 | 앞 단계의 rc 를 보기 전에 시작한 단계 |
| `builds-05` | 12.0 (나-1) 첫 ctest, 12.11 (바), 23.0 (마), 23.6 6 | 옛 바이너리로만 돈 시험 `tool_identity_schema_revision` |
| `builds-06` | 12.11 (마), 21.6, 23.5 | 선언한 필요량의 약 2.5 배를 쓴 scratch make; '모두 `completed`' 대신 가장 낮은 값 |
| `builds-07` | 12.11 (바) | lint 가 선언보다 자료 파일이 다섯 많은 트리에서 돎 |
| `rules-06` | 21.5 3, 22.6 | none found 9 줄의 `rests on: none` 이 감싼 함수 아래의 `abort` 를 빠뜨림 |
| `semantics-02` | 25.11 5.2·5.3, 25.13 열린 점 9 | `osapi-idmap.c:640` 은 할당의 초기화; 25.13 에서 빠졌던 열린 점을 되살림 |
| `semantics-03` | 21.5 3, 22.6 | C1 의 clear-slot 10 가운데 이름을 지우는 것은 2 |
| `semantics-04` | 25.11 5.3, 25.13 열린 점 10 | 고친 빌드 A21 의 재시도 뒤 실패 이벤트를 facts 가 놓침; 계획의 need note 는 도구의 빈틈 |
| `semantics-05` | 12.11 (라) | HB-await 284 가운데 소스가 허락하는 것은 생성자 → 만든 task 의 60 |
| `semantics-06` | 25.11 5.5 | A63 의 '뿌리가 닿지 않음' 은 도구의 빈틈(함수 안 정적 분배 표) |
| `semantics-07` | 25.11 5.2 기초 5 | 키 글의 한도는 64 가 아니라 20 |

`rules-07`, `rules-08` 은 이미 적혀 있어 손대지 않았다(§26.0 (거)). 같은 날 더한 날짜 붙인 주가 넷 더 있다: 8.7 의 '검증됨/전체 항목' 아래(분자는 적용된 항목의 수; 741·1,089·310, c1 의 일치 칸 112 `unverified`·46 `body-derived`; PI 결정 ①)와 8.8 의 '읽기 전용 효과' 아래(`strlen` 은 P0 선언 자료에 없음); `plan/results_w6_20261029.md` 의 머리(옛 판정 경로의 재분류, 자동 판정의 표에 넣지 않음, PI 결정 ③); `kairos/docs/semantics_coverage.md` 9 절(알려진 빠짐; 규칙 개정이 아닌 주); `kairos/docs/static_facts.md` 5x 제목 아래(그 형식은 선언된 kz-static `6f40dd60…` 이 냄). 저장소 최상위 README 는 고치지 않았다.

**점검.** `../records/closure-notes-20261119/check.sh`(bash, 읽기만; 행 번호·키·판정은 입력이 아님)를 공간 감시 아래 한 번 돌렸다: rc 0, 출력 `check.txt`(HEAD `3535b43`), 감시 기록 `guard_check.log`(dip 20,480 B). 그 앞에 같은 스크립트의 앞판을 저장소 밖에서 두 번 시험으로 돌려 출력의 꼴만 고쳤다(근거가 아님). `kairos/docs/` 의 두 문서는 도구 정체성의 경로 밖이고(`kairos/cmake/KzSourceIdentity.cmake`: 대상의 소스 디렉터리와 `include/`·`cmake/`·최상위 `CMakeLists.txt`), 스크립트는 주석에서만 그 이름을 댄다. 고친 트리에 MC 의 `kz_lint`(`3d4d5a8f…`)의 규칙 일곱을 감시 아래 돌려 모두 clean 이었다(`lint.txt`, `guard_lint.log`). 결과 문서와 W6 문서는 어느 스크립트도 읽지 않고 부팅 선언의 목록에도 없다. 커밋 전에 이름 점검(`--cached`, 새 디렉터리 `--tree`)과 금지어 grep 을 했다.

**측정** (`check.txt`):
- 감시 기록의 가장 낮은 값(A): 고침 빌드의 `df_*` 38 은 974,893,056 B; 그 scratch 의 017–029 열셋 948,453,376 B, 001–016 열여섯 305,602,560 B(`002_scratch-make.log`, 필요량 500,000,000 에 dip 1,235,963,904 B; 다섯 디렉터리에서 끝 값이 하한 아래인 유일한 기록); 닫기 kz-static 의 45 는 692,862,976 B; 런타임 27 은 679,084,032 B; 주기 준비 44 기록(끝 줄 50)은 393,605,120 B. 끝 줄의 판정은 모두 `completed` 다.
- 시간 겹침(B): 고침 빌드 다섯(빌드 뒤 `tools_check --deep` 과 실행 셋; 측정의 facts 보기와 비교 둘), 닫기 kz-static 셋(실행 `extract_final` 과 비교 셋), 런타임·주기 준비 0.
- 고침 빌드의 측정 파일: 다섯 02:03:56Z, 둘 02:04:10Z·02:04:28Z; 비교의 끝 02:03:26Z–02:04:14Z, 쌍둥이 rc 1 02:04:16Z, `twin_2`·`declared` 02:06:34Z·02:06:38Z(C). 주기 준비의 원장 점검 08:41:15Z·08:47:02Z·09:23:51Z·09:55:08Z, 깊지 않은 등록부 점검 09:10:12Z 하나(D).
- none found 9 줄, 모두 `rests on: none`, 효과 `terminal [CFE_PSP_Panic …]`, `layers` 에 `P0-unverified(abort, …)`(F). clear-slot c1 10/10, r2 6, c5 18/18(G). HB-await c1 284 = 짝 71 × 4(writer `main` 60, 넷 56 씩), r2 116 = 짝 29 × 4(H). framework 항목 741, 선언 1,089, `api` 이름 310, c1 일치 칸 158 = `unverified` 112 + `body-derived` 46(I). `strlen` 0 줄, `strcmp` 93 줄(J).

**공간.** 시작 384,884,736 B(13:00Z 무렵), 커밋 전 13:25:10Z 381,849,600 B(바닥 314,572,800 위 67,276,800 B). 다른 단계의 일로 계속 줄고 있다. 새로 남긴 것은 이 기록 디렉터리(26,481 B)뿐이다. scratch 는 이 단계의 시험 출력과 편집 스크립트만 만들었고 끝에 지운다.

**계획과 다르게 한 것.**
1. 주의 수를 남긴 파일에서 읽으려고 새 기록 디렉터리 `records/closure-notes-20261119/`(점검 스크립트, 출력, 감시 기록, README)를 더했다.
2. `builds-06` 의 주를 12.11 (마)에 더해 21.6·23.5 에도 달았다(검토가 그 둘을 이름 댐). 23.5 의 가장 낮은 값 679,084,032 B 는 이번에 센 것이다.
3. `semantics-05` 의 짝 15 는 검토의 소스 읽기를 따랐다. 앱을 다시 시작하는 경로(`cfe_es_apps.c:1165`)의 생성자는 세지 않았고 [I], r2 의 116 은 다시 세지 않았다.
4. `rules-06` 의 주는 대안 줄 자체가 아니라 그 행의 absence 줄이 효과 `CFE_PSP_Panic` 을 적는다고 읽었다(검토의 말 'the lines end in' 을 줄의 자리대로 고침).
5. `semantics-02`·`semantics-04` 의 열린 점은 25.13 의 열린 점에 번호를 이어 달았다(9, 10).

**열린 점.**
1. §26.0 (하)의 도구 한계 다섯(`rules-06`, `semantics-03`–`06`)은 다음 kz-static 빌드의 일이다. 그때 r2·c5 의 clear-slot 과 r2 의 HB-await 를 소스로 다시 센다.
2. `space_guard.sh` 의 끝 값 판정(§26.0 (차))은 아직 고치지 않았다. 그때까지 결과 문서는 판정 대신 가장 낮은 값을 인용한다.
3. 계획의 need note 글(`semantics-04`)과 site 264 의 계획(`semantics-02`)은 그대로다(§26.0 (너)).
4. OSAL 4.1.1 의 빠짐의 수는 남긴 파일로 다시 센 뒤에 `semantics_coverage.md` 9 절에 적는다 [미확인].

### 26.2 부팅을 위한 공간: 바이트를 보존하는 압축과 바이트가 같은 디렉터리의 회수 (2026-11-19; §26.0 (카), 부팅 없음)

**계획 (어느 압축이나 지우기보다 먼저 커밋함) [M][I].** 기록: `../records/closure-space-20261119/`. 이 절의 수는 그 디렉터리의 파일에서 읽었다.

- **목록**(`listing.tsv`, `listing.py`): obs1 아래 디렉터리 134, 블록 1,790,484,480 B. 그 가운데 아직 돌 단계가 읽거나, 견줄 기준이거나, 회수의 기준인 것은 34(549,044,224 B)다.
  - 부팅과 선언이 읽음: `cz_final_extract`, `cz_final_order`, `cz_plans_v4_ns2`, `c1p2_*` 여섯, `xr4_dry_inputs`.
  - 2026-11-16 입력의 회귀가 읽음: `c1pre_boot`, `c1pre_bprime`, `c1pre_derived`, `c1pre_explain`, `plangen`, `plangen_relcheck`.
  - 예행이 읽음: 위의 `cz_*` 셋, `rtb_bind2_*` 둘.
  - 동결본 출력이라 고친 판과 견줄 기준: `xr4_*` 열하나.
  - 회수 규약의 기준: `ll_final_order`, `order_rev1b`, `relrev7_check`, `revfix_order`, `revfix_relcheck`.
  - `/home/user/work` 의 다른 디렉터리 34 가운데 12 는 제외 종류(소스 트리, 도구 사슬, `kairos-build`, `preserve`, `llvm-project`)다. 나머지는 obs1 의 회차 차례 밖이라 이 규칙으로 고르지 않는다. 예를 들어 `redesign/step2a`(2,661,679,104 B)의 장부 목록은 파일을 다 이름 대지 않는다(`tests_8b` 파일 150, 목록 137 줄).
- **필요량 [I]**(`need.txt`; 항목마다 남긴 감시 기록이나 디렉터리 크기에서 읽음):
  - 부팅 하나 51,810,304 B. 내역: 부팅 2,736,128 B(2026-11-16 부팅의 dip). 변환과 글 864,256 B(`c1pre_convert`). 16 갈래의 설명 47,800,320 B. 기록 409,600 B.
  - 설명은 같은 부팅 기록에 동결본을 돌린 D1 여덟 갈래의 dip 23,900,160 B 의 두 배로 잡았다. 부팅 기록은 사용 사건을 싣기 때문이다. 정적 16 갈래의 dip 은 26,869,760 B 다.
  - 부팅 12 번: 621,723,648 B.
  - 코드와 동결 단계의 실행: 177,725,440 B(앞 실행의 감시 기록에서). 내역: fixture 의 scratch 72,712,192 B(끝에 지움), 회귀 23,900,160 B, 두 계획 위에서 두 차례로 하는 예행 56,123,392 B, 시험 dump 예행 24,989,696 B.
  - 여유: 100,000,000 B.
  - 합: 바닥 위 899,449,088 B, 목표 여유 1,214,021,888 B.
  - 25.12 의 [I](부팅 하나 약 32 MB)보다 큰 까닭은 설명 출력을 부팅 기록이 든 측정으로 잡은 데 있다. 정적 측정으로 잡으면 바닥 위 648,282,368 B 다.
  - 덤프는 기록 풀(`pool_bytes=4194304`, kzrt 0.5.0 시험 dump 의 `runx` 줄) 위로 자라지 않으리라 본다 [I].
  - 13:40Z 의 여유는 379,064,320 B 였다. 그래서 더 낼 것은 834,957,568 B 다.
- **고른 것**(`chosen.tsv`; 이 차례대로 하고, 여유가 목표에 닿으면 멈춘다):
  - R: `rf2_pre_final_order`, `rf2_pre_relcheck`, `rf2_pre_order`, 30,797,824 B. 바이트가 같은지 다시 쟀다(`reclaim/identity_summary_before.txt`): 세 쌍 모두 `identical-no-extra`, 파일 65, 30,641,951 B. 기준은 `ll_final_order`, `revfix_relcheck`, `revfix_order` 다.
  - tier 1: 지금 회차(`cz_*`) 앞의 추출 회차 18, 718,483,456 B. 곧 `rf2_final_extract`·`ll_final_extract`, 중간 옵션 회차 `rf_on_extract`·`sem_on_extract`·`objacc_on_extract`, 옵션 없는 회차와 그 앞의 기본 회차 일곱, 작은 옵션 회차 여섯이다.
  - tier 2: 같은 회차들의 order·relations 출력 14, 173,289,472 B.
  - 압축 비율은 `f9ce046` 넷 가운데 가장 큰 값 0.072207 로 기대했다. 모두 하면 기대 여유는 1,237,242,431 B 이고, 32 번째 줄에서 목표에 닿는다. 다른 단계의 쓰기로 여유가 계속 줄고 있어 모자랄 수 있다.
  - tier 2 는 26.0 (카)의 글('중간 추출 회차')보다 넓다. 이 단계의 지시('중간이거나 대신된 남긴 회차')를 따르되, R 과 tier 1 로 모자랄 때만 쓴다.
- **방법**: R 은 `reclaim-20261118` 의 규약을 따른다. 장부 행을 기준 디렉터리로 돌린 제안 장부를 지우기 전에 점검하고, `reclaim.sh` 가 지우기 바로 앞에 `diff -rq` 로 다시 견준다. tier 1·2 는 `f9ce046` 의 다섯 단계를 한 디렉터리씩 감시 아래에서 한다(`run_seq.sh`): 목록 대조, xz tar, 압축의 sha256 과 목록의 사본, 압축에서 흘린 파일마다의 왕복, 그다음에만 지우기. 그 뒤 장부 행을 고쳐 `ledger_check.sh` 로 점검한다. 장부의 규약은 README 에 적었다.

**결과 [M]** (`progress.tsv`, `archive/`, `reclaim/`, `ledger/`, `guard/`, `free_*.txt`). cFS 를 부팅하지 않았고 push 하지 않았다.

- **R**(`reclaim/reclaim.txt`): 세 디렉터리를 지웠다. 셋 모두 지우기 바로 앞의 `diff -rq` 에서 같았다. 여유 377,647,104 → 408,440,832 B(+30,793,728). 장부의 세 행은 지우기 전에 기준 디렉터리를 가리키게 고쳐 점검하고(193 행, 실패 0) 커밋했다(`698bdd9`).
- **tier 1**: 추출 회차 18 을 모두 압축했다. 원본 블록 718,483,456 B, 압축 47,814,552 B(디렉터리마다 113,744–5,133,268 B).
- **tier 2**: 10 줄째(줄 22–31)를 마친 뒤 목표에 닿아 멈췄다. 원본 143,192,064 B, 압축 2,067,680 B. 나머지 넷(줄 32–35: `order_rev1`, `rf2_relcheck`, `extfix_relcheck`, `rf2_order`)은 건드리지 않았다.
- **압축 28 개 모두 PASS**: 1 단계 목록 대조, 5 단계 지우기 전의 왕복(흘린 멤버 1,041 개, sha256 이 모두 목록과 같음). 압축 디렉터리 `/home/user/work/preserve/archives-20261119` 는 이제 32 개(앞의 넷 포함)이고 63,979,520 B 다.
- 압축마다 장부 행을 `parts` 로 고쳤다. 고친 제안 장부를 `ledger_check.sh` 로 점검(28 번 모두 193 행, 실패 0)한 뒤에만 장부를 바꿨다. 장부 머리에는 이 단계의 줄 하나를 더했다.
- **여유(df -B1 /)**: 시작 379,424,768 B(13:37:48Z). 압축 직전 408,137,728 B, tier 1 뒤 1,077,710,848 B, tier 2 뒤 1,217,945,600 B. 끝 1,217,867,776 B(13:56:56Z, `free_end.txt`)로 목표 1,214,021,888 B 보다 3,845,888 B 위다. **필요량을 채웠다.** 바닥 위는 903,294,976 B 다.
  - (2026-11-19 덧붙임, 26.5; `pre-builds-09`) '필요량을 채웠다'는 13:56:56Z 의 값이다. 그 뒤 26.3 의 실행이 약 392 MB 를 써서 다시 모자랐다(26.3 의 공간과 열린 점 1, 26.4 의 열린 점 1, 26.5 의 공간).
- 감시 기록의 가장 낮은 값은 377,647,104 B(`guard/g06_reclaim.log` 의 시작)이고, 압축 중의 가장 낮은 값은 402,989,056 B 다.
- **끝 점검**: `ledger_check.sh` 193 행 실패 0(`ledger/check_end.txt`). `tools_check.sh --deep` 57 행 실패 0(`tools_check_deep_end.txt`). 도구와 등록부는 바뀌지 않았다.
- 커밋마다 이름 점검(`--cached`, 새 디렉터리 `--tree`)과 금지어 grep 을 했다.
- 압축하거나 지운 31 디렉터리를 경로로 이름 대는 커밋된 스크립트는 109 곳이다(`scripts_naming_removed.txt`). 모두 앞 단계의 것이고 아직 돌 단계의 것은 없다. 그대로는 다시 돌지 않는다. 바이트는 압축이나 기준 디렉터리에 있다.

**계획과 다르게 한 것.**
1. 절 번호: 지시는 26.1 이었으나 26.1 은 앞 단계(`a52a99a`)가 이미 썼다. 그래서 26.2 로 적었다.
2. 차례: 지시는 압축 → `rf2_pre_*` 회수였다. 회수는 하나뿐인 바이트를 지우지 않고 압축도 필요 없으므로 먼저 했다(커밋한 `chosen.tsv` 의 차례).
3. 압축 스크립트의 첫 시도가 1 단계에서 멈췄다. `rf2` 회차의 목록은 경로를 `./` 없이 적는데, 스크립트가 두 꼴을 글자로 견줬기 때문이다. 지운 것은 없고 디렉터리도 그대로였다. 실패 기록은 `.trial1_fail` 이름으로 남겼다. 두 대조가 목록을 `./<경로>` 꼴로 읽게 고치고 자체 시험을 하나 더해(5/5) 다음 압축 전에 커밋했다(`ea19ba4`). `listing.py` 는 처음부터 두 꼴을 맞춰 읽었으므로 목록과 고른 것은 바뀌지 않았다.
4. 목록의 첫 실행(`guard/g01_listing.log`)은 이름이 `_on_extract` 로 끝나는 계획 디렉터리를 추출로, 점검 출력을 order 로 잘못 갈랐다. 둘째 실행이 같은 파일 이름에 덮어썼으므로 **첫 실행의 `listing.tsv` 는 남지 않았다**(감시 기록만 남음).
5. tier 2(order·relations 회차)를 썼다. 이것은 26.0 (카)의 글('중간 추출 회차')보다 넓고, 이 단계 지시의 '대신된 남긴 회차'에 기댄다. 열넷 가운데 열을 썼다.
6. 필요량은 25.12 의 [I] 보다 크다(설명 출력을 부팅 기록이 든 측정으로 잡음). 정적 측정이면 tier 1 과 R 로 채웠을 것이다.

**열린 점.**
1. 끝의 여유는 목표보다 3,845,888 B 만 위다. 다른 단계의 쓰기로 여유는 계속 준다(26.1 의 13:00Z 무렵 384,884,736 B 에서 이 단계 시작 13:37:48Z 379,424,768 B 로, 약 38 분에 5,459,968 B). 부팅 단계는 시작 때 다시 재야 한다(평가 절차 14.9). 목표 밑이면 `chosen.tsv` 의 줄 32–35(블록 29,097,408 B, 기대 약 27 MB)를 같은 방법으로 쓸 수 있다. 그 뒤로는 이 규칙으로 고를 디렉터리가 없다. 26.0 (카)에 따라 PI 에게 묻는다.
2. 부팅 하나의 필요량 [I] 은 2026-11-16 부팅(계획된 행 12)에서 왔다. 지금 계획은 계획된 행이 52 다. 첫 부팅의 감시 기록으로 다시 재어 남은 부팅의 필요량을 고친다.
3. 압축은 같은 디스크에만 있다(verified_copy 는 이 디스크 위의 왕복). remote 는 여전히 none 이다.
4. `/home/user/work` 의 obs1 밖 디렉터리(`redesign/step2a` 2.66 GB, `redesign/extract_baseline` 326 MB 등)는 이 규칙 밖이다. 장부 목록이 파일을 다 이름 대지 않는다. 그것을 쓰려면 목록을 먼저 만드는 글과 lead 의 판단이 필요하다.

### 26.3 설명 스크립트 명세 11절의 코드, fixture, 회귀, 예행 (2026-11-19; §26.0 (나)–(자)·(차), 부팅 없음)

**무엇 [M].** 기록: `../records/closure-explain-fix-20261119/`(README 가 파일을 적음). cFS 를 부팅하지 않았고 push 하지 않았다. 아무것도 동결하지 않았다(커밋한 코드는 소스 스냅샷이다). 차례는 다음과 같다.
- 명세 11.4 를 코드보다 먼저 커밋했다(`cee2ae9`). 코드를 쓰며 정한 것들이다: 이름 붙은 호출 파일을 만드는 스크립트, 주소 쓰임의 두 꼴, 기록에 더한 칸 넷, 반증 스크립트의 줄마다의 가름 규칙, 실행 스크립트의 시험 이음새, 감시의 rc 77.
- 그다음 코드와 fixture 를 함께 커밋했다(`dc879c6`).
- 그 뒤 아래의 실행을 모두 감시 아래 새 디렉터리에 돌렸다. 앞판의 부분 diff(재시작 전 `explain_judge.py` 의 75 줄)는 그대로 살렸다: 판의 이름, `text_of`, 갈래에 넘기는 호출 파일, `entry_state` 의 (b)–(d). 나머지는 새로 썼다.
  - (2026-11-19 덧붙임, 26.5; `pre-builds-05`) 이 문장은 g01·g02 를 덮지 않는다. g01(`guard/g01_named_calls.log`, 14:21:49Z)은 `../records/closure-c1prep-20261119/candidates/named_calls.tsv` 를 제자리에 썼는데, 그 규칙을 정한 명세 11.4(`cee2ae9`, 14:39:38Z)와 `named_calls.py` 의 커밋(`dc879c6`, 14:43:10Z)보다 먼저다. 계획과 다르게 한 것이다. g02(14:22:02Z)는 이미 커밋된 바꾸지 않은 `candidates.py`(`2a3ea84`, `5add2805…`)를 명세 11 의 2 대로 다시 돈 것이라 차례의 어긋남이 아니다. 커밋된 `named_calls.py`(`a88066dd…`)를 같은 인자로 다시 돌린 출력은 바이트까지 같다(`4dc7e78f…`; `../records/closure-prebootfix-20261119/out/named_calls_rerun_cmp.txt`).
- 판정 부분(`env/obs1/explain_judge.py`) — 명세 11 의 1, 3, 4, 5, 6:
  - 문맥 줄이 없는 행의 문맥은 그 행의 대안 줄이 이름 댄 문맥 id 가 정확히 하나일 때 그 id 다(`-` 도 줄이 적으면 그대로). 하나가 아니면 `unknown` 이다.
  - 항목이 없는 호출은 모든 갈래에서 `link_unlinked(name binding: no entry: <호출>)` 이고, 근거에 호출 파일의 줄을 단다.
  - 버퍼 주소 쓰임: `no entry …` 쓰임이 있으면 (다)는 모든 갈래와 `Rf` 에서 `missing` 이다. `<칸> unverified` 쓰임은 후보 줄에 기댄다(`depends_on`).
  - 찾은 사용에 열린 세대가 없으면 `undecided(no create of the found id in the record)` 또는 `undecided(the row's creator sites are not complete)` 이고 `unmet` 은 아니다.
  - (c1), (c2), (c4), (c3)(i), 모든 (c3)(iii), (다)에 `depends_on` 을 갈래마다 적는다.
  - 서식 검사 14 에 둘(문맥의 대안 줄, 찾은 사용의 `unmet`), 16 에 (a)–(d)를 더했다.
- 읽는 부분(`explain_read.py`) — 명세 11 의 2: `--named-calls` 를 더했다. `--candidates` 와 함께여야 하고, 어긋나면 rc 2 로 멈추고 기록을 쓰지 않는다.
- 이름 붙은 호출 파일 `../records/closure-c1prep-20261119/candidates/named_calls.tsv` 는 새 스크립트 `scripts/named_calls.py` 가 만들었다(`out/named_calls_run.txt`). 호출 넷이다:
  - `memchr` `length-call`: 기록 0(항목 없음).
  - `memset` `copy-call,fill-call`: 4, 미대조 2.
  - `strcmp` `compare-call`: 3, 미대조 1.
  - `strncpy` `copy-call`: 4, 미대조 2.
  - 바꾸지 않은 `candidates.py` 를 같은 입력에 다시 돌렸다(`obs1/xr5_candidates_regen`). 64 파일이 64 개 모두 커밋된 후보 목록·인정 파일과 바이트까지 같다(`out/candidates_regen_compare.txt`).
- 그 밖의 코드:
  - 요약(`explain_summary.py`; 7–9): 갈래마다 머리 줄 하나(`main (declared before the boot: D1/R0)` 은 `D1-R0` 에만), 사용 단위의 표, 쌍 종류 여덟 줄과 `unknown` 의 셋 나눔, 실패 징후의 두 나눔, `--check-summary` 를 넣었다.
  - 기대 대조(`explain_expect.py`; 12): 여덟째 칸 `class` 를 받는다. 기대 파일과 `expect_write.py` 는 이 단계에서 바꾸지 않았다.
  - 새 반증·평가 스크립트 `env/obs1/explain_eval.py`(13).
  - `boot/run_cycle.sh`: `verify` 단계, `eval` 단계, 깨끗한 `verify.txt` 가 있어야 도는 `boot`, 설명 단계의 호출 파일, 요약의 사용 목록을 더했다. `boot/commands.txt` 는 `verify` 를 빌드마다의 첫 명령이자 부팅마다 부팅 앞에 둔다. 시험은 `boot/test_run_cycle.sh` 다(14–15).
  - `env/tools/space_guard.sh`(26.0 (차)): 끝 값이 바닥 아래면 판정 `below_floor_at_end` 와 rc 77 을 낸다. 등록부 `records/TOOLS.tsv` 에 두 줄을 두었다(지금 판 `f67ac6ec…` in-place, 앞 판 `730bcc06…` gone(git 에 있음); `tools_check` 59 행 실패 0).

**시험 [M].**
- `env/obs1/tests/test_explain4.py`: **193 점검 모두 통과**(`out/fixtures.txt`). 앞의 127 에 66 을 더했다. 모두 합성 이름이고 설계 행의 꼴이 아니다.
  - (2026-11-19 덧붙임, 26.5; `pre-rules-05`) '모두 합성 이름'은 맞지 않았다. 항목이 없는 호출과 '기록이 있고 미대조가 아님'의 fixture 는 설계 행의 호출 `memchr` 를 설계 행의 칸(`length-call`)에 그대로 썼다. 26.5 에서 합성 이름(`zlen`, `zcmp`, `zput`)으로 바꾸고 `compare-call` 과 쉼표 목록의 경우를 더했다(206 점검).
  - 문맥 줄이 없는 한 문맥 행: 대안 줄이 found / none found(D2 에서 `held`, `derived`; D1 에서 `not_established`) / not established 인 셋, at-call 과 틀 사건이 없는 사용, 문맥 둘을 이름 댄 행(`unknown`), 대안 줄이 없는 행, `[context -]` 의 행.
  - 이름 붙은 호출 파일: 머리·꼬리·plan sha256·빠진 호출·파일 없음, 모두 rc 2.
  - 기록이 없는 호출: `D1`, `D2`, 부분 인정, `D2-all` 에서 모두 `no entry: …`. 기록이 있고 미대조가 아닌 호출은 `held`. 어느 파일에도 없는 항목은 '읽지 못함'.
  - 주소 쓰임: 항목 없음(`D2`, `D2-Rf` 에서 (다) `missing`), 미대조(`D2` 에서 `held` 와 `depends_on`; `D1` 과 그 종류를 뺀 부분 인정에서 `missing`).
  - 세대 없는 찾은 사용: 성공 경로 `missing`, 생성자 불완전, 생성 사건 없음 셋 모두 `undecided`, `unmet` 이 아님.
  - (c4)·(c1)(c2)·(c3)(i)의 `depends_on` 이 갈래마다 다른 상태로 감, 이름 대지 않는 행의 '없음'.
  - 서식 검사 14 의 둘과 16 (a)–(d)를 깬 기록 일곱(모두 그 검사에 물림).
  - 요약: 머리·사용 행·쌍 종류 줄, 그리고 깬 요약 넷(rc 3). 기대의 `class` 와 깬 파일 둘(rc 2).
- `env/obs1/tests/test_explain_eval.py`: **92 점검 모두 통과**(`out/eval_fixtures.txt`).
  - F1–F10, s1–s4 가 각각 한 번 이상 `fail` 한다. F6 만은 `not_evaluable` 이다(11.4 의 5: 실패하는 입력이 없음).
  - `expect_discriminate` 의 충족·미달·`not_counted`.
  - 정밀도: `undefined(no reader)`, `(no tool positive)`, `(T + F = 0)`, `U` 가 있어 확정하지 않은 값, 조건부 1/2.
  - 재현율: 1/1, 0/1, `undefined(k = 0)`, 옆 칸.
  - 줄마다의 가름(성공 경로 줄이 `held` 이면 F2 (a)·s1·s3·s4 와 (3)의 `expect_discriminate` 가 `boot`).
  - 열기·쓰기 감시와 그 음성 대조 둘: 기대 파일을 F1 밖에서 여는 사본, 출력 옆에 쓰는 사본을 잡는다.
- `boot/test_run_cycle.sh`: **7/7**(`out/run_cycle_tests.txt`). 경우는 다음과 같다:
  - 깨끗한 선언: `rows 6 ok 6 differs 0 missing 0`, 부팅 명령의 모의가 돎.
  - 바꾼 값: `differs`, rc 1, 부팅 명령을 부르지 않음.
  - 없는 파일: `missing`, rc 1.
  - verify 뒤에 바뀐 선언, `verify.txt` 없음: 둘 다 부팅 안 함.
  - 두 번째 verify: 앞 출력을 새 이름으로 남김.
- 단위 점검: `env/tools/tests/run_unit.sh` **20/20**(`out/unit_tools.txt`). 점검 1 의 감시는 rc 77 경우를 더했다. 점검 19 는 193, 새 점검 20 은 92 다. `env/redesign/step2a_tests_unit.sh` 7/7(`out/unit_step2a.txt`).
- 원장 `ledger_check` 205 행 실패 0(`out/ledger_check.txt`; 새 행 `OBS_xr5_*` 12). 커밋마다 이름 점검(`--cached`, 새 디렉터리 `--tree`)과 금지어 grep 을 했다.

**회귀: 2026-11-16 입력 [M]** (`obs1/xr5_regress_rev4fix`, `out/regress.log`). 24.3 과 같은 스크립트, 같은 입력, D1 여덟 갈래다. 갈래마다 45 기록이고 물림 0 이다. 동결한 판의 출력 `obs1/xr4_regress_rev4_2` 와 견줬다(`scripts/compare_records.py`, `out/compare_regress.txt`). **축 상태와 판정 값은 여덟 갈래 모두 45/45 같다.** 다른 필드 경로의 부류는 13 이고, 여덟 갈래에서 부류와 수가 똑같다. 그 차이는 모두 아래 고침에서 나온다.
1. `data_branch.admitted_kinds`, `data_branch.named_calls_sha256` 더함(45, 45): 11.4 의 2. 이 입력에는 호출 파일이 없어 `none` 이다.
2. `order.use_events[].context` `-` → `null`(177; `uses.tsv` 의 `context` 칸 177 줄도 같음): 명세 11 의 1. 이 relations 글(개정 14 줄이 없음)에는 대안 줄이 없다. 그래서 이름 댄 문맥 id 의 집합이 비어 `unknown` 이다. 예전의 `-` 가 `rules-01` 의 결함이다.
3. `use_cond.branch_iii[]` 의 `context` `-` → `null`(22), `why` `no alternatives line of context -` → `context unknown`(22), `uses` 더함(22): 2 와 같은 까닭이다. 22 는 사용 사건이 든 기록이다.
4. `use_cond.missing[]` 과 `tool_verdict.violation.undetermined_reasons[]` 의 `caller_dependence_unknown((i) not_established, (iii) not_established)` → `caller_dependence_unknown(context unknown)`(22): 하위 사유가 '문맥을 모름'을 먼저 고르는 줄(바뀌지 않음)이 3 을 따른 것이다.
5. `depends_on` 더함: `callee_check`, `reach`, `writers`, `branch_i`, 모든 `branch_iii` 항목(각 45). 명세 11 의 5 다. 후보 목록이 없는 실행이라 모두 빈 목록이다.
- 수명 끝의 `t_ms` 는 이 기록들에 수명 끝이 없어 차이가 없다. 스크립트 정체성 칸(`script.*`, `inputs.normal`)은 따로 셌다.

**예행 [M].**
- **(가) 두 C1 빌드의 정적 출력, 실행 없음, 16 갈래**(`obs1/xr5_dry_static_<빌드>`). 입력은 후보 목록, `branches_16.txt`, 호출 파일, 24.4 와 같은 정적 입력이다. 두 빌드 모두 rc 0, 갈래마다 46 기록, 물림 0, 입력 점검 refused 0 이다.
  - 경로 A 행의 요소 값(`scripts/elements.py`, `out/static_elements_16_fixed.tsv`; 첫 실행 `…trial1_root_none.tsv` 는 소비 뿌리 없음을 `none` 으로 적어 견줌이 틀렸다)을 동결한 판의 `../records/closure-c1prep-20261119/scratch/branches16/static_elements_16.tsv`(160 줄)와 견줬다. 다른 칸은 빌드마다 26, 모두 `D1` 갈래다.
  - (c4) `present` → `missing`: `D1-Rd`·`D1-all` 의 A20, A21, A22, A26 과 A63(빌드마다 10). `Rd` 는 (c4)를 전제로 세운다. 그런데 생성자 줄이 이름 댄 생성 API(`OS_CountSemCreate`·`OS_TimeBaseCreate`·A63 의 생성자)의 `agreement` 항목이 그 행의 `layers:` 목록과 후보 목록에 있다. `D1` 은 그 항목을 인정하지 않는다(명세 11 의 5; 그 절의 [I] 가 든 예와 같다).
  - A63 의 (c1) `present` → `missing`, `use_cond` `partial` → `not_derived`(`D1` 여덟 갈래, 빌드마다 16 칸): A63 의 flip 줄이 사용 callee `OS_FileOpenCheck` 를 이름 대고, 그 `agreement` 항목이 목록에 있다. 남는 요소가 없어 `not_derived` 다. 명세 11 의 5 의 [I] "`use_cond.status` 는 바뀌지 않으리라"는 A63 에서 맞지 않았다. **`violation` 은 160 줄 모두 `undetermined` 그대로다.** `D2` 갈래에는 다른 칸이 없다.
- **(나) 런타임 단계의 시험 dump**(`obs1/xr5_dry_rt_rt_objects`, `…_rt_format15`; `out/dry_rt_*.log`): 24.4 (나)와 같다. 읽는 부분 rc 0 이고, 판정 부분 rc 3 이다(그 dump 는 계획의 manifest 를 싣지 않아 모든 기록이 `build_mismatch`; 계약대로). 16 갈래 × 46 기록, 물림 0.
- **(다) 실제 두 계획 위의 합성 사용 사건**(시험 입력이며 증거가 아님; `scripts/synth_record.py`, `obs1/xr5_synth_<빌드>_<script|reverse>`, `out/synth_*.log`).
  - 생성기의 규칙은 그 머리글에 있다. 행위자는 쌍 파일이 이름 댄 뿌리마다 하나이고, 코어 뿌리를 먼저, 그다음 시작 스크립트의 앱을 그 차례나 그 반대로 둔다. 생성·사용 호출은 계획의 site 와 결합 파일의 묶인 site 에서만 낸다. 키 글은 relations 의 생성자 줄에서 온다. 결과는 같은 partition·같은 글의 생성이 먼저 끝났는지로 정한다. 행 번호·키·기대는 인자가 아니다.
  - 넷 모두 rc 0, 갈래마다 46 기록, 물림 0, 사용 523 이다(사건 1,132/1,130, 사용 호출 521, 생성 6).
  - 경로 A 행(`scripts/path_a_uses.py`, `out/synth_path_a_rows.txt`; `D1-R0`·`D2-R0`):
    - 문맥 줄이 없는 행의 사용은 문맥 `1` 을 받는다(A20, A22, A26, c1_post_tx 의 A21). c1_pre_tx 의 A21 은 틀로 정해진다(script 차례 1, reverse 차례 2).
    - c1_post_tx A21 의 (iii)은 `not_held`(found: retry)다. 평가 절차 14.11 이 예측한 수준 1 과 같은 꼴이다.
    - 찾은 사용은 모두 `undecided(the row's creator sites are not complete)` 이고 인스턴스 연결은 `unlinked` 다. `unmet` 은 하나도 없다.
    - 모든 쌍의 (나)는 `no entry: memchr`, (다)는 `no entry: strncpy, memchr, memcpy` 다.
    - 경로 A 의 모든 기록이 `violation undetermined` 다.
  - 첫 실행(`obs1/xr5_synth_c1_pre_tx_script_trial1`)은 쌍 파일의 뿌리 글 `-` 와 중괄호 집합에도 행위자를 만들었다(52 기록). 생성기를 고쳐 다시 돌렸고, 첫 출력은 남겼다.
- **(라) 반증·평가 스크립트를 여섯 예행 출력에 돌림**(`obs1/xr5_eval_dry`, 꼬리 `out/eval_dry_tails.txt`). 문 결과는 없고, 기대 파일은 시험 입력 `out/test_expect_c1_with_class.tsv` 다(커밋된 `expect_c1.tsv` 에 12 의 규칙으로 `class` 를 단 것: static 960, boot 1,184; 이 주기의 기대 파일이 아님).
  - 여섯 모두 `form ok` 다.
  - F1 의 `static` 어긋남은 빌드마다 26 이다. (가)의 26 칸과 같다. 곧 동결 전 판의 정적 값을 비춘 기대 줄이 고친 판과 갈리는 자리다. `boot` 의 F1 어긋남(정적 416; 합성 320–352)은 실행 없는 기록이나 합성 기록을 부팅 예측과 견준 것이라 아무것도 증명하지 않는다.
  - s1–s4 는 280 줄 모두 `static` 이고 `pass` 다.
  - `expect_discriminate` 는 모두 `static` 이다:
    - 정적 예행: c1_pre_tx 충족 16 / 센 64(A20; A21 은 `tier+control` 이라 `not_counted` 16), c1_post_tx 32/80(A20, A21).
    - 합성 c1_pre_tx script: 0/64. 그 합성 세계에서 A20 의 사용이 찾음이라 (3)을 요구하고, 세대가 없어 미달이다.
    - (2026-11-19 덧붙임, 26.5; `pre-rules-02`, `pre-rules-01`) 위의 수(16/64, 32/80, 0/64)와 s1–s4 의 280 은 16 갈래 × 행의 합이고 주 결과(`D1-R0`)가 아니다. 또 그 판(`f6b72fc4…`)은 찾은 사용마다 규칙 (3)을 붙여, 같은 정적 줄이 기록에 따라 충족과 미달로 갈렸다(평가 절차 14.13 의 1). 고친 판으로 같은 여섯 출력을 다시 평가한 주 결과는 26.5 에 있다.
  - F8 (b)·F10 은 `not_evaluable` 이다(연결된 쌍 없음, 문 결과 없음). 정밀도·재현율은 `undefined(no reader)` 다.
  - 첫 합성 평가는 출력 이름이 겹쳐 부팅 이름표가 `explain` 이었다. `…trial1_label_explain` 으로 남겼다.

**공간 [M].**
- 시작 1,213,014,016 B(14:21Z, `out/free_start.txt`), 끝 822,243,328 B(14:57:18Z, `out/free_end.txt`)다.
- 감시 기록은 38 이고 끝 줄은 42 다. 모두 `verdict=completed` 이고 `below_floor_at_end` 는 없다. 가장 낮은 값은 823,791,616 B 이고, 바닥 314,572,800 B 위로 509,219,216 B 다.
- 남긴 출력은 `obs1/xr5_*` 열둘, 약 367 MB(겉 크기)다. 그 가운데 합성 다섯이 239 MB 다. fixture 와 시험 출력은 scratch 에 두었다가 지웠다.
  - (2026-11-19 덧붙임, 26.5; `pre-builds-09`) 두 수는 남긴 파일에 없던 값이다. 26.5 에서 재어 남겼다: 겉 크기 367,485,823 B, 합성 다섯 238,996,026 B, 블록 385,576,960 B(`../records/closure-prebootfix-20261119/out/xr5_sizes.txt`).
- 26.2 가 잡은 '코드와 동결 단계의 실행' 177,725,440 B 보다 훨씬 많이 썼다. 16 갈래와 합성 예행 다섯 번 때문이다. 그래서 부팅 12 번의 목표 여유(1,214,021,888 B)에 약 392 MB 모자란다(열린 점 1).

**계획과 다르게 한 것.**
1. 절 번호: 지시는 26.2 였으나 26.1·26.2 는 앞 단계가 썼다. 그래서 26.3 으로 적었다.
2. 명세 11.4 를 코드보다 먼저 따로 커밋했다(머리의 규칙). 그 안의 정함 가운데 기록에 더한 칸 넷(`admitted_kinds`, `named_calls_sha256`, 수명 끝의 `t_ms`, 모든 (iii) 항목의 `depends_on`)은 11.1–11.3 이 적지 않은 것이다.
3. 첫 fixture 시험 하나(scratch, 약 46 MB)는 감시 없이 돌렸다. 그 뒤의 개발 시험(`guard/dev*.log`)과 모든 실행은 감시 아래였다.
4. 경로 B 의 fixture 세계에서 경로 A 행의 문맥 줄을 뺐다(11.4 의 8). 검사 14 에 더한 줄이 그 세계를 물렸기 때문이다.
5. F6 은 실패하는 fixture 가 없다(11.4 의 5). 지시는 '모든 점검을 한 번은 실패'였다.
6. `expect_write.py` 와 기대 파일의 다시 씀(12)은 하지 않았다. 다음 단계의 일이다(26.0 (나)·(다)·(라)).
7. 평가 출력이 부팅마다 약 1 MB 였다. 그래서 예행의 평가 출력은 기록 디렉터리에 두지 않고 `obs1/xr5_eval_dry` 로 옮겼다. 기록에는 꼬리와 sha256 목록만 둔다. 명세 11 의 15 는 부팅의 `eval.txt` 를 '작은 글'로 기록 디렉터리에 둔다(열린 점 3).
8. 첫 합성 생성기와 첫 요소 비교, 첫 합성 평가의 출력은 고친 뒤에도 새 이름으로 남겼다.

**열린 점.**
1. **공간.** 여유 822 MB 로는 부팅 12 번의 목표에 약 392 MB 모자란다. 이 단계의 `obs1/xr5_*`(JSON 기록이 대부분)를 26.2 의 방법(목록 대조, xz tar, 파일마다의 왕복, 장부 행을 `parts` 로)으로 바꾸면 되찾을 수 있으리라 본다 [I]. 고를 것과 양을 먼저 커밋해야 한다(26.0 (카)). 부팅 전에 lead 가 정한다.
2. 남은 차례(평가 절차 14.0): 규칙에서 다시 쓰는 기대와 예측(12, 기대 쓰는 스크립트의 `class` 칸 포함), 다시 하는 동결(판정 부분 sha256 은 이 단계의 커밋 판 기준), 새 부팅 선언(새 줄: 어휘 파일, 이름 공간 자료 다섯, `named_calls.tsv` 와 그 스크립트, `explain_eval.py`, 새 동결 파일).
3. `eval.txt` 의 크기와 자리(약 1 MB × 부팅 12): 명세 11 의 15 를 고칠지 lead 가 정한다.
4. 명세 11 의 5 의 [I] 가 A63 에서 틀렸다(`D1` 의 `use_cond` `not_derived`). 평가 절차 14.6 의 표는 이 단계의 출력에서 어긋나지 않았다(s1–s4·F5·F6 `static`, F2 (b)·F7 `format`, F3·F4·F8 (b)·F9·F10 `boot`). 그러나 다시 동결한 판으로 다시 확인해야 한다(14.6 머리).
5. 글 커밋 셋(`b071f02`, `adfdb51`, `3535b43`)의 결과 소절은 아직 없다(그 단계의 열린 점). 그 단계의 수는 남긴 파일이 없어 이 절에 옮기지 않았다. (2026-11-19 덧붙임: 26.6 이 닫는다.)
6. 합성 생성기의 한계 [I]. 사용의 키 글은 그 행의 첫 생성자 글이다. 수명 끝·효과·정지 사건은 내지 않는다. 생성자 뿌리가 쌍 파일에 없으면 생성이 없다. 그래서 합성 예행은 코드 길만 보이고, 부팅 결과의 예측이 아니다.

### 26.4 다시 하는 동결, 규칙에서 다시 쓴 기대, 새 부팅 선언 (2026-11-19; §26.0 (가)·(나)·(다)·(라)·(바)·(사)·(아)·(타), 부팅 없음)

**무엇 [M].** 기록: `../records/closure-preboot-20261119/`(README 가 파일을 적음). cFS 를 부팅하지 않았고 push 하지 않았다. 출력을 쓰는 실행은 모두 감시 아래 돌렸다(감시 기록 35, 모두 `verdict=completed`, `below_floor_at_end` 없음). 커밋의 차례는 다음과 같다.
1. `aaadfba` 다시 하는 동결(새 동결 파일).
2. `3acbea8` 평가 절차 14.12(다시 쓰는 기대와 새 부팅 선언보다 먼저): F1 의 가름 둘, 기대를 다시 쓰는 법, 합성 기록과 맞댈 때의 셋째 종류.
3. `38142cb` 규칙에서 다시 쓴 기대와 예측(어느 실행과도 맞대기 전).
4. `46ea311` 고친 판정 부분의 합성 기록과의 맞댐, 어긋난 줄마다의 가름.
5. `549dc94` 새 부팅 선언 `boot/declaration2.tsv`(옛 선언 `860c2cd` 를 대신함), 실행 스크립트·시험·명령의 고침.
6. 이 커밋: verify 한 번의 출력, 끝 점검, 이 절.

**(1) 다시 하는 동결 [M].** 새 동결 파일은 `../records/closure-preboot-20261119/freeze.tsv`(`70bbf7f5…`)다. 앞의 `../records/closure-explain-rev4-20261119/freeze.tsv`(`733feef7…`)는 덮어쓰지 않았다. 원본 커밋은 `dc879c6` 이고 그 뒤로 이 파일들은 바뀌지 않았다(`git status` 깨끗함).

| 부분 | 파일 | sha256 | 앞 동결과 |
|---|---|---|---|
| 읽는 부분 | `env/obs1/explain_read.py` | `ef20a760…` | 바뀜(명세 11 의 2: `--named-calls`) |
| 판정 부분 | `env/obs1/explain_judge.py` | `1d0b4ae0…` | 바뀜(11 의 1, 3, 4, 5, 6) |
| 구동부 | `env/obs1/explain.py` | `af9d4395…` | 같음 |
| 갈래 생성 | `env/obs1/explain_branches.py` | `a722e4af…` | 같음 |
| 요약 | `env/obs1/explain_summary.py` | `5e19b419…` | 바뀜(7–9, 11) |
| 기대 대조 | `env/obs1/explain_expect.py` | `a9a9fc3f…` | 바뀜(12: `class` 칸) |
| 반증·평가 | `env/obs1/explain_eval.py` | `f6b72fc4…` | 새로(13) |
| fixture | `env/obs1/tests/test_explain4.py`, `tests/test_explain_eval.py` | `804326bf…`, `5e153435…` | 바뀜, 새로 |
| 개정 3 | `env/obs1/explain_rev3.py` | `acfd4271…` | 같음(회귀의 견줌 기준) |

- 동결 전 점검: 동결한 바이트 위에서 fixture 를 다시 돌렸다. `test_explain4.py` 193/193(`out/fixtures.txt`), `test_explain_eval.py` 92/92(`out/eval_fixtures.txt`). 앞뒤의 sha256 이 같다(`out/freeze_sha_before.txt`, `…_after.txt`).
- 회귀(2026-11-16 입력), 16 갈래 정적 예행, 시험 dump, 합성 사용 사건은 26.3 이 같은 판으로 돌렸다. 그 기록의 `script.judge_sha256` 은 `1d0b4ae0…`, `reader_sha256` 은 `ef20a760…` 다. 그래서 다시 돌리지 않았다.

**(2) 규칙에서 다시 쓴 기대 [I][M].** 기대를 쓰는 법은 평가 절차 14.12 의 2 로 먼저 커밋했다. 새 파일은 옛 파일 옆에 두었다(`expect_c1.tsv` `d5501bf6…` 와 `predictions.tsv` `44011827…` 는 그대로).
- `scripts/expect_write2.py`(`6947e9cb…`). 입력은 다섯이다.
  - 25.11 의 커밋된 바탕 `scratch/branches16/static_elements_16.tsv`(8.14 앞의 규칙이 정적 입력에 낸 요소 값).
  - 두 빌드의 relations 글(행 블록의 `layers:`·`flip:`·`absence:`·`creator:`·`holder:`·대안 줄·성공 경로 줄).
  - 후보 목록과 갈래 파일이 이름 댄 인정 파일.
  - 손으로 쓴 예측 `expect/predictions2.tsv`(`4ed9a362…`, 68 줄, 줄마다 근거).
  - 미리 정한 사용의 목록.
  - 행 번호·키·값은 상수가 아니다. 고친 판정 부분의 출력은 읽지 않는다.
- 건 규칙.
  - 판정 규칙 8.14 의 6 (다)와 명세 11 의 5: 요소의 출처 줄이 이름 댄, 행의 `layers:` 목록의 함수를 후보 목록에서 찾는다. 그 갈래가 인정하지 않으면 present·`held` 를 내린다.
  - 8.14 의 6 (가): 문맥 줄이 없는 행의 문맥은 대안 줄의 하나뿐인 id 다.
  - 8.14 의 2 의 결합으로 `use_cond` 를 다시 센다. `derived` 가 나오면 스크립트가 멈춘다(나오지 않았다).
  - 예측 쪽: 8.14 의 5(`memchr` 항목 없음: 조건 (나)의 의미는 `D2` 에서도 빠짐), 6 (가), 6 (나)(찾은 사용은 `undecided(the row's creator sites are not complete)`, 사용과 행의 연결 `unlinked`; 명세 11 의 4 의 차례).
- 출력 `expect/expect_c1_2.tsv`(`444f7615…`): 2,208 줄(16 갈래 × 두 빌드의 경로 A 행 다섯 × 칸). `class` 는 `static` 1,280, `boot` 928, `format` 0 이다. `static` 은 여섯 칸 × 160 = 960 에 14.12 의 1 이 더한 `use_cond.caller_dependence.text` 160 과 `link.status` 160 이다. 빈 기록 디렉터리에 돌린 형식 점검은 rc 0, `no_record 2208` 이다(`expect/form_check2_no_records.txt`). 요소 값은 `expect/elements_by_rule_2.tsv`(160 줄)에 갈래마다의 `depends_on` 과 함께 있다.
- **바뀐 것**(`expect/changes_2.tsv`; 같음 1,676, 바뀜 468, 더함 64):

| 칸 | 줄 | 바뀜 | 규칙 |
|---|---|---|---|
| `use_cond.writers.status` (c4) | 20 | `D1-Rd`·`D1-all` 의 행 다섯 × 두 빌드: present → missing | 8.14 의 6 (다): `creator:` 줄이 `OS_CountSemCreate`·`OS_TimeBaseCreate`·`OS_mv` 를 이름 대고, 그 `agreement` 항목이 후보 목록에 있어 `D1` 이 인정하지 않음 |
| `use_cond.callee_check.status` (c1) | 16 | A63, `D1` 여덟 갈래 × 두 빌드: present → missing | 같은 규칙: A63 의 `flip:` 줄이 `OS_FileOpenCheck` 를 이름 댐 |
| `use_cond.status` | 16 | A63, `D1` 여덟 갈래: `partial` → `not_derived` | 남는 요소가 없음(8.14 의 2) |
| `use_cond.caller_dependence.text` | 48 | 두 빌드의 A20: (iii) `not_established` → `not_held`(32); 고친 빌드의 A21: 같음(16) | 8.14 의 6 (가): 대안 줄의 문맥 1 이 `found: …` |
| `use[0].context` | 112 | `-` → `1` | 8.14 의 6 (가): 문맥 줄이 없는 행(결함 빌드 A20·A22·A26, 고친 빌드 A20·A21·A22·A26) |
| `link.status` | 64 | A22·A26: `established` → `unlinked` | 8.14 의 6 (나): 세대가 없음(성공 경로 줄 `missing`) |
| `use[0].name_state` | 64 | A22·A26: `undecided(a slot clear not shown to be no lifetime end)` → `undecided(the row's creator sites are not complete)` | 8.14 의 6 (나), 명세 11 의 4 |
| `use[0].link_result` | 64 | A22·A26: `linked` → `unlinked` | 8.13 의 14번(열린 세대에만) |
| 쌍의 `missing_items` | 64 | `D2` 여덟 갈래의 A20·A21 생성 자리 둘: `na_semantics` 가 더해짐 | 8.14 의 5 |
| 더함 | 64 | A20 의 `use[1].context`, `use[1].link_result` | 둘째 조회도 같은 규칙으로 적음 |

- 이 바뀜으로 요소 값은 빌드마다 26 칸이 25.11 의 바탕과 다르다. 26.3 이 고친 판정 부분의 정적 예행에서 본 26 칸과 같은 자리다. 기대를 커밋한 **뒤에** 규칙으로 낸 값과 고친 판정 부분의 정적 표를 칸마다 견줬다(`compare/elements_vs_fixed_static.txt`): 960 칸, 다른 칸 0. 기대는 바꾸지 않았다.
- 명세 11 의 5 의 [I] 'use_cond 는 바뀌지 않으리라'는 A63 에서 맞지 않는다(26.3 열린 점 4 와 같다). `violation` 은 160 줄 모두 `undetermined` 다.
- **행마다의 기대 (D1-R0, 두 빌드)** [I]:
  - A21 (CF 의 조회): 결과 absent. 문맥은 결함 빌드에서 틀로 1, 고친 빌드에서 1(대안 줄). 이름 상태 `undecided(the row's creator sites are not complete)`, 연결 `unlinked`. (iii)은 결함 빌드 `not_established`, 고친 빌드 `not_held`(`found: retry`). 쌍의 빠진 항목: BP 자리 {기초 4, (다), (마), (나) 의미}, SCH_LAB 자리 + 기초 6. 결과 `differs_from_use_key,unlinked` / `differs_from_use_key`.
  - A20 (탐침 행): 사용 둘 모두 absent, 문맥 1. (i)·(iii) 모두 `not_held`. 쌍은 A21 과 같은 꼴에 기초 6.
  - A22·A26 (시간 기준 행): found, 문맥 1, 이름 상태 `undecided(the row's creator sites are not complete)`, 사용·행의 연결 `unlinked`.
  - A63 (파일 이름 행): 사건 없음, `target_not_reached`, `D1` 에서 `not_derived`.
  - `D2` 의 쌍 항목은 `D1` 과 같다(`memchr` 항목 없음).

**(3) 고친 판정 부분과의 맞댐 [M]** (평가 절차 14.12 의 3). 동결한 `explain_expect.py` 에 `expect_c1_2.tsv` 를 주고 26.3 의 합성 출력 넷(`/home/user/work/redesign/obs1/xr5_synth_<빌드>_<script|reverse>/explain`)과 정적 예행 둘(`…/xr5_dry_static_<빌드>`)에 돌렸다(`compare/*.txt`). 출력마다 한 빌드만 담으므로 다른 빌드의 1,104 줄은 `no_record` 다.
- 합성 넷: `static` 줄은 빌드마다 640/640 모두 같다. `boot` 줄은 같음 256–272, 어긋남 192–208 이다.
- 정적 예행 둘: `static` 줄 640/640 같음. `boot` 줄은 사용 사건이 없어 448 이 어긋난다. 정적 예행은 26.0 (라)의 맞댐이 아니라 가르지 않았다.
- **합성 넷의 어긋난 줄 784 의 가름**(`scripts/classify_compare.py`, `compare/classify_synth_*.tsv`; 줄마다 기록의 칸을 근거로 적음): **판정 부분의 결함 0, 기대의 잘못 0, 합성 입력 784**. 그래서 판정 부분도 기대도 고치지 않았다. 출력마다의 내역은 다음과 같다.
  - (2026-11-19 덧붙임, 26.5; `pre-builds-03`) '기대의 잘못 0'은 시험의 결과가 아니다. 그 가름 스크립트에는 기대의 잘못을 내는 길이 없었고, T1·T3·T4 는 기대 값을 읽지 않았으며 T5 는 항목의 차이만 읽었다. 가름은 생성기의 꼴에서 왔다. 그러므로 이 784 줄은 기대의 잘못인지 시험되지 않았다 [I]. reverse 차례의 줄은 모두, script 차례의 줄은 아래 끝 목록의 것이 그렇다. 부팅 뒤의 읽기는 평가 절차 14.13 의 7 이다(F1 이 어긋나면 그대로 보고하고 기대 파일을 다시 쓰지 않음). T4 의 '생성 사건 없음' 갈래는 기록을 보지 않았다. 기록을 보게 고친 `classify_compare2.py` 로 다시 가른 수는 같다(T4 의 줄은 모두 '부재 사용이 없음', '생성 없음' 0, 가르지 못함 0; `../records/closure-prebootfix-20261119/compare/classify2_*.tsv`).
  - T1 64: A20 의 `use[1]` 이 기록에 없다. 생성기는 사용 호출 site 쌍마다 호출 하나를 쓴다(루프의 둘째 바퀴가 없음).
  - T2 16: 기대 absent 가 found 다.
    - script 차례의 A20: 생성기가 사용의 키 글을 행의 첫 생성자 줄의 글(`SCH_LAB`)로 준다. 그 글의 생성이 앞서 끝났다.
    - reverse 차례의 A21: BP 가 CF 보다 먼저 돈다.
    - 둘 다 이름 상태는 `undecided`, 인스턴스 연결은 `unlinked` 다(`unmet` 아님; 8.14 의 6 (나)대로).
  - T3 16(결함 빌드 reverse 만): A21 의 문맥이 2 다. 생성기가 reverse 에서 사용을 마지막 문맥 틀 안에 둔다.
  - T4 64: 찾은 사용에는 쌍이 없다(쌍은 부재 사용에만 맺음, 8.2).
  - T5 32: 빠진 항목의 차이가 모두 기록의 항목 근거로 설명된다.
    - (마)가 `held` 다: 기록에 수명 끝 사건이 0 이다. 생성기는 쓰지 않는다.
    - reverse 에서 기초 6 이 `held` 다: 생성이 사용 뒤다.
    - 기초 5·(나) 한도가 빠진다: 생성 하나의 키 글이 기록되지 않았다. 생성기의 생성자 키 글 규칙 때문이다.
  - T6 16: 결과에 `differs_from_use_key` 가 없다. 그 자리의 생성 키 글이 사용의 글과 같거나 기록되지 않았다(`order.publish_events`).
- 합성 세계가 시험하지 못한 기대 줄도 있다 [I]. (마)의 `missing`, A20 의 둘째 사용, BP 의 둘째 생성의 키 글(`BP1_tsem`), 시작 스크립트 차례에서의 A20 의 absent 다. 부팅 기록에서 처음 맞댄다.
- 동결한 `explain_eval.py` 도 새 파일의 `class` 칸을 읽는다. 결함 빌드 script 합성에서 F1 은 `boot` 지남 272·어긋남 192, `static` 지남 640 이다(`out/eval_synth_*_tail.txt`; 출력 약 1 MB 는 scratch 에 쓰고 머리·꼬리·sha256 만 남김).

**(4) 후보 목록, 갈래 파일, 미리 정한 사용, 판독지 [M].** 모두 그대로다. 옛 선언의 줄로 다시 재어 sha256 이 같다(`declaration/remeasure_860c2cd_2.tsv`). 26.0 의 규칙 가운데 이것들을 바꾸는 것은 없다.
- (다): 항목이 없는 호출은 후보 목록에 넣지 않는다. `memchr` 는 처음부터 후보 목록에 없었다(`closure-c1prep-20261119/out/candidates_named_check.txt`). '항목 없음'은 새 입력 `named_calls.tsv` 가 나른다. 바꾸지 않은 `candidates.py` 를 다시 돌린 64 파일이 바이트까지 같았다(26.3).
- (나): 문맥의 고침은 기록의 문맥 id 를 바꾼다. 사용 목록의 단위는 바꾸지 않는다. 목록의 `-` 는 평가 절차 5.1 의 '문맥 줄이 없는 행의 사용 하나'다. 요약은 그것을 기록의 어느 문맥과도 맞춘다(명세 11.4 의 3).
- (라)·(사): 판정 부분의 출력만 바꾼다.
- 판독지: 'not filled: no reader' 그대로다. `reader_files.tsv` 가 가리는 판정 규칙의 sha256(`71c5dfee…`)은 8.14 앞의 파일이다. 판독자를 정할 때 다시 커밋한다(평가 절차 14.5).

**(5) 새 부팅 선언 [D].** `boot/declaration2.tsv`(`9758074a…`, 행 130)가 `860c2cd` 의 `boot/declaration.tsv`(그대로 둠)를 대신한다.
- 옛 선언의 행 110 을 다시 쟀다(`declaration/remeasure_860c2cd_2.tsv`). 103 이 같다. 설치된 ELF 42 줄의 sha256 과 GNU build-id, plan-id 둘, 계획, sidecar, 결합 파일, B′ 기초, 주기의 파일, 도구가 그 안에 있다. 7 이 커밋된 고침으로 바뀌었다: `run_cycle.sh`, 읽는 부분, 판정 부분, 요약, 기대 대조, fixture, `space_guard.sh`(`declaration/changes_vs_860c2cd.tsv`). 처음 잰 것(`remeasure_860c2cd.tsv`)은 `run_cycle.sh` 를 고치기 전이고, 그것도 남겼다.
- 더한 행은 20 이다.
  - 동결: 반증·평가 스크립트와 그 fixture, 새 동결 파일.
  - 설명 단계의 입력: `reason_vocabulary.txt`, 두 sidecar 의 `input namespace-data` 줄이 이름 댄 이름 공간 자료 다섯.
  - 계획 입력: `framework_api_roles.txt`.
  - 주기의 파일: `named_calls.tsv` 와 그 스크립트, `expect_c1_2.tsv`·`predictions2.tsv`·`elements_by_rule_2.tsv`·`expect_write2.py`·바탕 `static_elements_16.tsv`, `commands.txt`, `test_run_cycle.sh`, `no_expect_input_check2.txt`.
- 선언의 머리에 적은 것.
  - 문(평가 절차 3절).
  - 부팅 수의 규칙과 다시 한 부팅의 읽기(26.0 (타), 14.10: 1r 이 문을 지나면 네 번 더).
  - 더 한 부팅에서 읽는 둘(사용마다의 부팅별 분포, `other_order_seen_in_boots`).
  - 주 결과 `D1/R0` 와 보조 갈래(나머지 15 와 부분 인정 496).
  - 기대하는 결과와 그 구조적 까닭: (c2)·(c4)는 정적으로 정해지지 않는다. (c3)(i)은 어느 갈래에서도 `held` 가 아니고, (iii)은 모든 문맥에서 `not_held` 이거나 `not_established` 다. 쌍에서는 기초 4(성공 경로 줄 0/14)와 (다)가 모든 갈래에서 빠진다. 그래서 어느 설계 행도 `observed` 에 닿을 수 없다. 이 기대는 양성도 음성도 아니다.
  - 지금 잰 공간과 필요량(아래).
  - 명령 `boot/commands.txt`.
- 실행 스크립트(제자리에서 고침; 앞판은 git 에 있음). `run_cycle.sh` 는 `declaration2.tsv` 를 재고, `eval`·`expect` 단계는 `expect_c1_2.tsv` 를 읽는다. 기록의 이름표도 이 절을 가리킨다. `bash -n` 깨끗하다. `boot/test_run_cycle.sh` 는 꾸민 선언의 이름만 고쳤고 7/7 이다(`out/run_cycle_tests.txt`). `boot/commands.txt` 는 머리만 고쳤다. 명령은 그대로이고 빌드마다 첫 명령이자 부팅마다 부팅 앞의 명령이 `verify` 다. 쓰기만 했고 돌리지 않았다.
- 기대 파일이 입력이 아님의 점검을 다시 했다(`boot/no_expect_input_check2.txt`, 명세 11 의 15). 기대 파일과 예측을 이름 대는 단계는 `eval`(F1 에만: `explain_eval.py:328`)과 `expect` 둘이다. 설명 단계의 인자와 두 계획의 `args.txt` 에는 없다.
- **verify 한 번** [M]: `run_cycle.sh verify <빌드> 0` 을 두 빌드에 돌렸다. 둘 다 rc 0, `rows 130 ok 130 differs 0 missing 0` 이다(`boot/c1_pre_tx/boot0/verify.txt`, `boot/c1_post_tx/boot0/verify.txt`; 감시 기록 `guard/verify-*.log`). N = 0 은 부팅 번호가 아니다. 부팅 단계는 0 을 받지 않는다. 부팅들은 제 N 으로 verify 를 다시 돈다.

**공간 [M].**
- 시작 819,277,824 B(15:10:36Z, `out/free_start.txt`), 끝 811,319,296 B(15:33:13Z, `out/free_end.txt`)다. 바닥 위로 496,746,496 B 다.
- 감시 기록의 가장 낮은 값은 707,485,696 B(`guard/evalfx.log`)다. fixture 의 scratch 107,872,256 B 는 끝에 지웠다.
- 새로 남긴 것은 기록 디렉터리의 글뿐이다(비교 출력 여섯 약 1.5 MB 포함). obs1 에 새 출력은 없다.
- **필요량과 견줌**(선언의 머리; 26.2 `need.txt` [I]): 부팅 하나 51,810,304 B × 12 = 621,723,648 B 에 여유 100,000,000 B 를 더하면, 첫 부팅 전에 1,036,296,448 B 가 비어 있어야 한다. 15:27:40Z 에 잰 값은 811,941,888 B 로 224,354,560 B 모자란다. 26.0 (카)에 따라 그 필요량이 채워지거나 PI 가 정하기 전에는 부팅하지 않는다. 설명 출력을 정적 측정으로 잡으면(26.2 `need.txt` 의 `beside` 줄 648,282,368 B 에서 코드 단계의 몫 177,725,440 B 를 뺌) 부팅 12 번과 여유에 바닥 위 470,556,928 B, 곧 785,129,728 B 가 필요하고 지금 값은 그 위다. 그러나 선언한 필요량은 큰 쪽이다(26.2).

**끝 점검 [M].** `ledger_check` 205 행 실패 0(`out/ledger_check_end.txt`), `tools_check` 59 행 실패 0(`out/tools_check_end.txt`). 이 단계는 등록부의 도구와 obs1 의 남긴 출력을 바꾸지 않았다. 커밋마다 이름 점검(`--cached`, 새 디렉터리 `--tree`)과 금지어 grep 을 했다.

**계획과 다르게 한 것.**
1. 절 번호: 지시는 26.3 이었다. 26.3 은 코드 단계가 썼다. 그래서 26.4 로 적었다.
2. 평가 절차 14.12 를 더했다(지시 밖의 글 커밋). 두 가지다.
   - 14.6 의 F1 줄을 14.6 머리의 가름 규칙으로 고쳤다. 그 결과 `link.status` 와 `use_cond.caller_dependence.text` 가 이 주기에 정적이다. 명세 11 의 12 의 '출처로 가름'으로는 `boot` 다(320 줄).
   - 합성 기록과 맞댈 때 '합성 입력' 종류를 두었다. 26.0 (라)의 두 종류 밖이므로 맞대기 전에 커밋했다.
3. 요소의 8.14 앞 값은 25.11 의 커밋된 바탕에서 읽었다. 그 바탕은 동결 앞 판정 부분의 정적 실행 출력이다(25.11 이 그렇게 만듦). 8.14 가 바꾼 것만 규칙으로 걸었다. 앞 판의 8.13 구현이 틀렸다면 그 틀림은 이 기대에도 남는다 [I].
4. 기대를 쓰는 스크립트는 새 이름(`expect_write2.py`)으로 두었다. 명세 11.3 은 `expect_write.py` 를 '다시 적는다'고 했다. 옛 스크립트와 파일은 그대로다.
5. 예측에 A20 의 `use[1].context`·`use[1].link_result` 를 더했다(64 줄).
6. 동결 파일은 새 디렉터리에 두었다(지시의 둘째 안).
7. 고친 판정 부분의 정적 표와의 견줌(0 칸 다름)은 기대를 커밋한 뒤에 했다. 기대는 바꾸지 않았다.
8. 가름 스크립트의 앞 시도 둘을 시도 이름으로 남겼다.
   - `*.trial1_*`: None 의 정렬로 둘이 멈췄다. 둘은 고친 스크립트로 다시 했다.
   - `*.trial2_t6_basis5.*`: 결과 시험이 키 글을 기초 5 의 근거에서 읽었다. 그런데 `held` 인 항목은 키 글을 싣지 않는다.
9. `run_cycle.sh`, `test_run_cycle.sh`, `commands.txt` 는 제자리에서 고쳤다(26.3 의 관행). `declaration.tsv`, `expect_c1.tsv`, `predictions.tsv`, `no_expect_input_check.txt` 는 그대로 두었다.
10. verify 를 N = 0 으로 돌렸다. 부팅의 `boot1/verify.txt` 와 섞이지 않게 하려는 것이다. `boot/<빌드>/boot0/about.txt` 에 그 뜻을 적었다.
11. 선언에 `framework_api_roles.txt` 를 더했다(계획 입력; 지시의 목록 밖).
12. 반증·평가 스크립트의 합성 출력 둘은 scratch 에 쓰고 지웠다. 머리·수·sha256 만 남겼다.

**열린 점.**
1. **공간**: 첫 부팅 전에 224,354,560 B 가 모자란다. 압축할 것과 양을 먼저 커밋하고 26.2 의 방법으로 내거나(예: 26.3 의 `obs1/xr5_*`; 26.3 열린 점 1), PI 에게 묻는다(26.0 (카)). 부팅 하나의 필요량 [I] 은 2026-11-16 부팅에서 왔다. 첫 부팅의 감시 기록으로 고친다(26.2 열린 점 2).
2. 합성 세계가 시험하지 못한 줄((마), A20 의 둘째 사용, BP 둘째 생성의 키 글, 시작 스크립트 차례의 A20)은 첫 부팅 기록에서 처음 맞댄다 [I].
3. 찾은 사용의 인스턴스 연결 사유 `link_unlinked(no create of the id in the record)` 는 세대가 없다는 뜻이다. 생성 사건이 기록되어도 그 성공 경로 줄이 `held` 가 아니면 같은 글이 나온다. 명세 11 의 4 가 이 글을 그대로 두었으므로 결함이 아니고, 상태(`unlinked`)도 맞다. 글을 고치면 판정 부분이 바뀐다. 다음 개정의 일로 남긴다.
4. 25.13 의 열린 점 가운데 아직 열린 것.
   - 4: `same_build` 의 build-id 읽기는 cFS 가 적재한 모듈에서 처음이다 [미확인](F10).
   - 5: 묶이지 않은 site 242, c1_post_tx 297.
   - 6: lead 판단 (a)–(c)의 셋은 그대로 열려 있다. 읽는 사람이 없고, 원 이슈의 글은 이 환경에 없다 [미확인].
   - 7: A22·A26 의 생성이 기록되는가 [I]. 이제 F4 에만 달리고, 시간 기준 행의 `expect_discriminate` 는 어느 쪽이든 미달이다.
   - 8: kzllvm 의 집합 피연산자 꼴, B′ 의 `treetok`.
   - 9: `osapi-idmap.c:640`(26.0 (파)).
   - 10: `semantics-04`.
   - 1(문맥 `-`)은 26.3 의 코드와 이 동결로 닫혔다. 2(다시 한 부팅의 읽기)는 26.0 (타)와 14.10 으로 닫혔다. 3(공간)은 26.2 로 계획했으나 위 1 로 다시 열렸다.
5. 26.3 의 열린 점 가운데 아직 열린 것.
   - 3: `eval.txt` 의 크기(약 1 MB × 부팅 12)와 자리. 지금 실행 스크립트는 기록 디렉터리에 쓴다.
   - 5: 글 커밋 셋(`b071f02`, `adfdb51`, `3535b43`)의 결과 소절이 없다. (2026-11-19 덧붙임: 26.6 이 닫는다.)
   - 6: 합성 생성기의 한계.
6. PI 에게 남은 것(26.0 (더)).
   - `R0` 결합과 (ii)의 자리(판정 규칙 8.14 의 2).
   - 읽는 사람. 정해지면 판독지·파일 목록·이 선언을 다시 커밋한다.
   - 기초 2 가 프로세스의 시작 스레드가 만든 것에서 설 수 없는 문제.
   - `Re` 가 (iii)을 움직이게 하는 도구 빌드.
   - PI 결정 기록이 PI 확인으로 남긴 것.
   - PI 결정 기록(`ac5b70a`)이 인용한 판정 규칙의 줄 번호는 지금 파일에서 2 뒤에 있다.
7. 다음 단계의 차례: 공간의 계획과 압축(또는 PI 의 결정). 그다음 `boot/commands.txt` 를 c1_pre_tx, 이어서 c1_post_tx 에 돌린다(root). 첫 명령은 `verify` 다.

### 26.5 다시 동결한 판의 검토 뒤 고침: `expect_discriminate`, 갈래마다의 꼬리, 손실 표지, 부팅 선언 3 (2026-11-19; 검토 `pre-…`, 부팅 없음)

**무엇 [M].** 다시 동결한 판과 부팅 선언 2(26.4)를 첫 부팅 전에 다시 본 검토와 그 검증이 22 지적을 냈다. 모두 진짜였고 14 가 부팅 전의 것이었다(`../records/closure-review-preboot-20261119/findings.tsv`; 아래의 `pre-…` 는 그 번호, `rules-NN` 등과 다름). 기록: `../records/closure-prebootfix-20261119/`(README 가 파일을 적음). cFS 를 부팅하지 않았고 push 하지 않았다. 커밋의 차례는 다음과 같다(글이 코드보다 먼저).
1. `65c6d88` 검토 기록. 지적마다 이 기록의 말로 줄인 요지를 적었다. 검토와 검증의 글 전체는 옮기지 않았다(이 저장소에 없는 문서를 인용한 곳이 있음).
2. `2b08b29` 평가 절차 14.13, 설명 스크립트 명세 11.5, 판정 규칙 8.14 의 6 아래 날짜 붙인 주.
3. `6846ef7` PI 결정 기록의 날짜 붙인 정정.
4. `7db20a3` 명세 11.5 의 1 의 같은 날 고침(코드 커밋 전; 개발 시험에서 바로잡음, 아래 계획과 다르게 한 것 4).
5. `ef6d38f` 코드와 fixture, 새 예측 파일, 이 단계의 실행 스크립트와 개발 시험.
6. `603adb8` 감시 아래의 실행.
7. `906e2d6` 세 번째 동결.
8. `13cf028` 부팅 선언 3.
9. 이 커밋: verify(N = 0) 두 빌드, 끝 점검, 이 절과 26.6, 26.2–26.4 의 날짜 붙인 주.

**지적마다 한 것.**

| 번호 | 한 것 | 다시 돈 것과 그 결과 [M] |
|---|---|---|
| `pre-rules-01`, `pre-texts-01` | 평가 절차 14.13 의 1–2: 규칙 (3)은 (1)·(2)의 요구가 없는 대조 행에만 건다(7절의 표를 되살림). `class` 는 정적 입력에서 셈한다. 14.6 의 `expect_discriminate` 줄, 14.12 의 1 끝 문장, 명세 11.4 의 5 의 그 문장을 대신한다. 반증·평가 스크립트 개정 2 | fixture 여섯 경우(`ED-found` 는 충족·`static` 으로 바뀜, `ED-found-heldsp`, `ED-3-found`, `ED-3-boot`, `ED-3-boot-unmet`, `ED-ctx` 의 두 점검). 동결 판 `f6b72fc4` 에서는 어긋난다(`out/dev_evfx_old_eval.txt`). 남긴 출력 여섯에 다시 돌린 주 결과(`D1-R0`): c1_pre_tx 충족 1 / 센 4(A20; A21 은 `not_counted`), c1_post_tx 2/5(A20, A21). 모든 줄이 `static` 이고 정적 예행, script, reverse 기록에서 같다(`out/eval_*_tail.txt`). 앞 판에서는 같은 줄이 기록에 따라 충족과 미달로 갈렸다 |
| `pre-rules-02` | 14.13 의 3: 꼬리를 갈래마다 쓴다. `D1-R0` 가 먼저 오고 `main`, 나머지는 자료 갈래와 읽기를 이름 댄 `auxiliary` 다. 합친 수는 없다. `count` 줄에 갈래 칸을 더했다 | fixture `tail-branch`(`D2-R0` 에서만 어긋남 → main 은 0, auxiliary 는 1)와 `base`. 26.3 (라)에 날짜 붙인 주 |
| `pre-texts-02` | 14.13 의 4: F3(경로 A 가 아니거나 부재 사용이 없는 기록), F4(찾은 사용이 없음), F9(사용 사건이 없음)는 `not_evaluable` | fixture `F3-pathB`, `F3-foundonly`, `noevents`, `base` 의 F4. 합성 c1_pre_tx script 의 주 결과에서 `boot` 반증은 지남 22 / 34(어긋남 12 는 F1 의 합성 입력 줄), 셈할 수 없음 180 이다. 앞 판은 16 갈래를 합쳐 '지남 1,760 / 1,952' 로 셌다(`../records/closure-preboot-20261119/out/eval_synth_c1_pre_tx_script.log`) |
| `pre-rules-07` | 14.13 의 5: 이 주기의 F3 은 약하다(site 264, `osapi-idmap.c:640`, OSAL 의 모든 할당에서 섬; 직접 확인). F3 의 `note` 가 수명 끝이나 slot clear 만으로 면한 생성을 적는다 | fixture `F3-slot`, `F3-end-note` |
| `pre-rules-05` | 명세 11.5 의 2: 항목이 없는 호출의 fixture 를 합성 이름으로 바꾸었다(`zlen`; `compare-call` 의 `zcmp`; 쉼표 목록의 `zput`). 판정 부분은 이 항목으로 바뀌지 않는다. `fill-call` 은 읽는 항목이 없어 두지 않았다 | `test_explain4.py` 206/206. 26.3 에 날짜 붙인 주 |
| `pre-semantics-01` | 명세 11.5 의 1: `no-build-id`·`object-table` 은 손실 표지가 아니다. 수명 끝의 불완전으로 `unlinked` 인 찾은 사용의 사유는 원인을 따른다. 실행 스크립트의 `control`·`gate` 가 손실 기록을 종류와 수로 이름 댄다. 예측의 전제를 `expect/predictions3.tsv` 에 적었다(근거 36 줄; 값은 그대로) | 새 fixture 다섯 경우 가운데 넷(다섯 점검)이 동결 판정 부분 `1d0b4ae0` 에서 어긋난다(`out/dev_fx_old_judge.txt`). 2026-11-16 회귀, 정적 예행 둘, 시험 dump 둘(`rt_objects` 는 `no-build-id` 두 줄을 실음), 합성 기록 넷: 남긴 출력과 다른 칸이 없다(스크립트 정체성과 이름표만). 합성 c1_pre_tx script 에 손실 한 줄을 더한 기록: `no-build-id` 는 같고, `after-stop` 은 이름 상태 `undecided(lifetime ends not complete: loss markers in the record)`, 찾은 사용의 사유 `record_insufficient(loss marker)`, (마) `missing` 이고 기대 대조의 어긋남이 192 → 240(갈래마다 셋 더)이다(`compare/loss_*`). `expect_write2.py` 를 `predictions3.tsv` 로 다시 돌린 기대 파일과 요소 표는 바이트까지 같다 |
| `pre-builds-02` | 부팅 선언 3: ELF 가 아닌 설치 파일 20(빌드마다 시작 스크립트 둘과 표 여덟), `ws/exe` 의 파일 수(줄 종류 `count`), `hash_instance.sh`·`persist_snapshot.sh`, 설치 목록 둘 | 20 줄 모두 빌드의 `install_inventory.tsv` 와 커밋된 원장 목록과 같다(`out/declaration3_installed_crosscheck.txt`). verify 159/159 |
| `pre-builds-07` | 부팅 단계: 선언과 `run_cycle.sh` 가 커밋되었고 고친 것이 없음을 보고, verify 를 그 자리에서 다시 돈다 | `test_run_cycle.sh` 11/11. 새 경우는 verify 뒤에 바뀐 선언된 파일, 커밋되지 않은 선언, `count` 줄, 손실 기록의 이름이다 |
| `pre-builds-03` | 14.13 의 7: 시험되지 않은 기대 줄은 부팅 뒤 어긋나도 그대로 보고하고 기대를 다시 쓰지 않는다. 26.4 (3)에 날짜 붙인 주. `classify_compare2.py` 는 T4 가 기록을 본다 | 다시 가른 수는 같다. T4 의 줄은 출력마다 64 이고 모두 '부재 사용이 없음'이다. '생성 없음' 0, 가르지 못함 0 |
| `pre-builds-05` | 26.3 에 날짜 붙인 계획과 다르게 한 것 | 커밋된 `named_calls.py` 를 다시 돌린 출력이 바이트까지 같다(`4dc7e78f…`) |
| `pre-builds-08` | 동결한 요약(`5e19b419…`)을 남긴 합성 출력 둘을 두 부팅으로 삼아 돌렸다 | 두 빌드 모두 rc 0, `summary branches 16 uses 6`(c1_pre_tx)·`uses 5`(c1_post_tx) `form ok`, `--check-summary` rc 0, sha256 앞뒤 같음 |
| `pre-texts-04` | 14.13 의 6: 수준 1 의 정적 부분과 부팅 부분. PI 결정 기록 ③ 아래 주와 §5 의 9 | 없음(글) |
| `pre-texts-08`, `pre-builds-10` | 명세 11.5 의 4–5, 14.13 의 9·11: 가름 규칙, `expect_write2.py`, 선언 파일의 이름(`declaration2.tsv` → `declaration3.tsv`), 14.7 의 입력. 26.0 의 라틴 글자 대응은 아래 | 없음(글) |
| `pre-texts-07` | 판정 규칙 8.14 의 6 아래 주, 명세 11.5 의 6, 14.13 의 10 | 없음(글) |
| `pre-texts-06`, `-10`, `-11`, `-12` | PI 결정 기록: §2 A9 와 ①의 표시, 45 가운데 44(A21 은 git 에도; `cmp` 같음), 시나리오 자료의 수를 뺌, §3 의 11(HB-await 284) | 없음(글) |
| `pre-builds-09`, `pre-texts-13` | 26.6(글 단계의 소절). `obs1/xr5_*` 의 크기를 남김. 26.2·26.3 에 날짜 붙인 주 | `out/xr5_sizes.txt`: 겉 크기 367,485,823 B, 합성 다섯 238,996,026 B, 블록 385,576,960 B |

(2026-11-19 덧붙임, 26.9; `fix-judge-05`) 위 `pre-builds-07` 의 시험 11/11 은 부팅 단계의 커밋 점검을 선언에 대해서만 시험했다. `run_cycle.sh` 쪽(커밋되지 않은 고침)과 `count` 줄의 `missing` 갈래는 시험되지 않았다(검증의 변이: 그 둘을 뺀 스크립트도 11/11). 잘못된 부팅이 열리지는 않았다: 선언 3 의 `run_cycle.sh` 줄이 sha256 으로 두 번 재지고, 없는 디렉터리는 어느 이름표로든 rc 1 이다. 두 경우는 26.9 의 시험에 더했다.

26.0 의 표지: 영어 기록(선언, 동결 파일, README, 스크립트 머리, 커밋 글)이 26.0 을 라틴 글자로 인용한 것은 차례의 대응이다. (a)=(가), (b)=(나), (c)=(다), (d)=(라), (e)=(마), (f)=(바), (g)=(사), (h)=(아), (i)=(자), (j)=(차), (k)=(카), (l)=(타), (m)=(파), (n)=(하), (o)=(거), (p)=(너), (q)=(더)다. 이 단계의 새 영어 기록은 26.0 을 제 표지로 인용한다.

**시험 [M]** (커밋한 코드 `ef6d38f` 위, 감시 아래): `test_explain4.py` 206/206(`out/fixtures.txt`), `test_explain_eval.py` 116/116(`out/eval_fixtures.txt`), `boot/test_run_cycle.sh` 11/11(`out/run_cycle_tests.txt`), `env/tools/tests/run_unit.sh` 20/20, `env/redesign/step2a_tests_unit.sh` 7/7(`out/unit_*.txt`). 개발 시험은 `out/dev_*.txt` 다.

**다시 돈 것 [M]** (판정 부분이 바뀌었으므로 명세 11.5 의 8; 출력은 scratch 에 쓰고 비교·sha256·꼬리를 남긴 뒤 지움).
- 2026-11-16 입력의 회귀(D1 여덟 갈래 × 45 기록)를 `obs1/xr5_regress_rev4fix` 와 견줬다. 축 상태와 판정이 45/45 같고, 다른 칸은 스크립트 정체성뿐이다(`compare/regress.txt`).
- 정적 예행 두 빌드(16 갈래 × 46)와 시험 dump 둘(16 갈래 × 46, 판정 부분 rc 3 은 계약대로)도 같다(`compare/static_*.txt`, `compare/rt_*.txt`).
- 합성 기록 넷을 다시 설명했다. 다른 칸은 기록 이름표와 스크립트 정체성뿐이다. `expect_c1_2.tsv` 와의 대조는 26.4 와 같다: 같음 912 / 896, 어긋남 192 / 208(`compare/synth_*`).
- 손실 한 줄을 더한 둘, 반증·평가 스크립트 개정 2 를 남긴 출력 여섯에, 요약 둘, 이름 붙은 호출 파일, 기대 파일의 다시 만듦, 가름 넷: 위 표.

**세 번째 동결 [M]** (`../records/closure-prebootfix-20261119/freeze.tsv`, `253b2f28…`; 앞의 `70bbf7f5…`·`733feef7…` 는 그대로). 원본 커밋은 `ef6d38f` 다.

| 부분 | sha256 | 앞 동결(`70bbf7f5…`)과 |
|---|---|---|
| 판정 부분 `explain_judge.py` | `644acc11…` | 바뀜(11.5 의 1) |
| 반증·평가 `explain_eval.py` | `03b9c95d…` | 바뀜(11.5 의 3, 개정 2) |
| fixture `test_explain4.py`, `test_explain_eval.py` | `e4a7dda6…`, `b050b238…` | 바뀜 |
| 읽는 부분, 구동부, 갈래 생성, 요약, 기대 대조, 개정 3 | `ef20a760…`, `af9d4395…`, `a722e4af…`, `5e19b419…`, `a9a9fc3f…`, `acfd4271…` | 같음 |

- 동결한 바이트 위에서 fixture 를 다시 돌렸다: 206/206, 116/116(`out/freeze_*fixtures.txt`). 앞뒤의 sha256 이 같다.

**부팅 선언 3 [D]** (`boot/declaration3.tsv`, `a6d242be…`, 행 159). `declaration2.tsv`(`9758074a…`, 그대로 둠)를 대신한다.
- 옛 행 130 을 다시 쟀다(`declaration/remeasure_declaration2.tsv`): 같음 121, plan-id 둘은 그대로 옮김, 바뀜 7. 바뀐 것은 `run_cycle.sh`, 판정 부분, fixture, 반증·평가, 그 fixture, `commands.txt`, `test_run_cycle.sh` 이고 모두 `ef6d38f` 의 것이다(`declaration/changes_vs_declaration2.tsv`).
- 더한 행은 29 다: 세 번째 동결, 설치 파일 20, 파일 수 둘(31, 31), 도구 둘, 설치 목록 둘, `predictions3.tsv`, `no_expect_input_check3.txt`.
- 머리에 적은 것: 손실 기록을 이름 대는 문, 갈래마다의 평가, 수준 1 의 두 부분, 기대하는 `expect_discriminate`, 지금 잰 공간.
- 기대 파일이 입력이 아님의 점검을 다시 했다(`boot/no_expect_input_check3.txt`). 예외는 `eval`(F1)과 `expect` 둘이다.
- **verify 한 번** [M]: `run_cycle.sh verify <빌드> 0` 을 두 빌드에 돌렸다. 둘 다 rc 0, `rows 159 ok 159 differs 0 missing 0` 이다(`boot/<빌드>/boot0/verify.txt`). 26.4 의 출력은 새 이름(`verify.20261119T175158.471662223Z.txt`, `verify.20261119T175200.491719483Z.txt`)으로 남았다.

**공간 [M].**
- 실행 시작 768,425,984 B(17:44:04Z, `out/free_start.txt`), 선언 때 763,105,280 B(17:50:50Z, `out/free_declaration.txt`), 끝 762,650,624 B(17:52:13Z, `out/free_end.txt`).
- 감시 기록 46 개는 모두 `verdict=completed` 다. 가장 낮은 값은 647,077,888 B(`guard/unit-tools.log`)로 바닥 위 332,505,088 B 다. 가장 큰 dip 는 117,207,040 B 다.
- 새로 남긴 것은 기록 디렉터리의 글(약 3.8 MB)뿐이다. obs1 에 새 출력은 없고 원장의 행도 더하지 않았다. scratch 의 출력은 비교 뒤에 지웠다.
- **필요량과 견줌**: 첫 부팅 전에 1,036,296,448 B 가 비어 있어야 한다(26.4). 17:50:50Z 의 763,105,280 B 는 273,191,168 B 모자란다. 26.0 (카)에 따라 부팅하지 않는다.

**끝 점검 [M].** `ledger_check` 205 행 실패 0(`out/ledger_check_end.txt`), `tools_check` 59 행 실패 0(`out/tools_check_end.txt`). 첫 호출은 경로를 잘못 주어 rc 127 이었고, `*.trial1_path.*` 로 남겼다. 커밋마다 이름 점검(`--cached`, 새 디렉터리 `--tree`)과 금지어 grep 을 했다.

**계획과 다르게 한 것.**
1. 절 번호: 지시는 26.4 였다. 26.4 는 앞 단계가 썼다. 그래서 26.5 로 적었다. 글 단계의 소절은 26.6 이다(시간의 차례 밖).
2. 검토 기록은 지적의 요지만 이 기록의 말로 적었다. 글 전체는 옮기지 않았다.
3. `expect_discriminate` 는 지적이 든 두 길 가운데 '규칙 (3)을 (1)·(2)가 없는 행에만' 을 골랐다(14.13 의 1). 7절의 표를 되살리는 쪽이다. 부팅 전에 골랐고 lead 의 확인을 기다린다.
4. 명세 11.5 의 1 끝 문장('앞의 셋은 지금의 바이트에서 어긋난다')은 틀렸다. 개발 시험에서 사건 손실 한 줄의 이름 상태는 앞 판도 맞게 냈다. 그래서 코드 커밋 전에 같은 날 고침으로 바로잡았다(`7db20a3`). 앞 판에서 어긋나는 것은 다섯 점검이다.
5. 예측은 `predictions2.tsv` 를 고치지 않고 새 파일 `predictions3.tsv` 로 두었다. 기대 파일은 바뀌지 않았다(다시 만들어 바이트가 같음).
6. 큰 출력(설명 기록, 평가 출력)은 scratch 에 쓰고 비교 뒤 지웠다. 남은 공간이 부팅의 필요량에 모자라기 때문이다. 비교, sha256 목록, 꼬리는 남겼다.
7. 시도 출력은 새 이름으로 남겼다: 개발 시험 일곱, `du` 옵션을 잘못 준 크기 목록(마지막 줄 없음), 경로를 잘못 준 끝 점검 둘.
8. 남긴 verify 출력의 이름을 나노초까지 적게 고쳤다. 지적 밖의 고침이다. 부팅 단계가 verify 를 다시 돌리면 한 초 안에 두 번 돌 수 있고, 그러면 앞 출력이 덮어써졌을 것이다.
9. 시험 dump 의 다시 돎은 26.3 이 남긴 dump 글과 대신 파일(run.meta, chain.txt, 시작 스크립트)을 읽었다. MC 의 `kz-rt-dump` 와 `chain_check.py` 는 다시 돌리지 않았다. 읽는 부분의 중간 글은 남긴 것과 바이트까지 같았다.
10. 판정 부분의 판 이름(`script.judge`)을 바꾸었다. 그래서 모든 기록의 그 칸이 다르다(스크립트 정체성으로 따로 셈).
11. 14.13 의 3 이 '26.4 가 인용한' 합친 수로 든 예(32/80, 280)는 26.3 (라)의 것이다. 26.4 가 인용한 합친 수는 F1 의 지남 272·어긋남 192·정적 640 이다.

**열린 점.**
1. **공간**: 첫 부팅 전에 273,191,168 B 가 모자란다. 압축할 것과 양을 먼저 커밋하고 26.2 의 방법으로 내거나(예: `obs1/xr5_*`, 겉 크기 367,485,823 B), PI 에게 묻는다(26.0 (카)).
2. lead 가 확인할 것: `expect_discriminate` 의 규칙 (3)의 자리(14.13 의 1, 위 계획과 다르게 한 것 3).
3. PI 에게 여쭐 것에 더함: 수준 1 을 정적 부분과 부팅 부분으로 나눈 보고로 주장해도 되는지(PI 결정 기록 §5 의 9), ①에서 뗀 P0 선언 자료(§2 A9).
4. `fill-call` 을 읽는 판정 항목이 없다. 판정 규칙 8.14 의 5 가 그것을 이름 대므로 항목이 필요한지는 다음 개정에서 정한다.
5. 합성 맞댐이 시험하지 못한 기대 줄(784 와 26.4 (3)의 끝 목록)은 첫 부팅에서 처음 맞댄다. 어긋나면 14.13 의 7 대로 보고한다 [I].
6. F3 은 relations 개정이 빈 레코드 조건을 낼 때까지 약하다(14.13 의 5; 26.0 (파)).
7. `eval.txt` 는 부팅마다 약 1 MB 다(남긴 출력 여섯의 sha256 과 줄 수는 `out/eval_*_tail.txt`). 그 자리는 26.3 의 열린 점 3 그대로다.
8. 25.13·26.3·26.4 의 열린 점 가운데 아직 열린 것은 26.4 의 열린 점 4–6 그대로다(글 단계의 소절은 26.6 이 닫음).
9. 다음 단계의 차례: 공간의 계획과 압축(또는 PI 의 결정). 그다음 `boot/commands.txt` 를 c1_pre_tx, 이어서 c1_post_tx 에 돌린다(root). 첫 명령은 `verify` 이고, 부팅 단계가 그것을 다시 돈다.

### 26.6 글 단계의 결과 소절: 검토 기록, PI 결정 기록, 판정 규칙 8.14, 평가 절차 14, 명세 11 (2026-11-19, 12:10Z–12:55Z 의 커밋; 이 절은 그 뒤에 씀, 부팅 없음)

**무엇.** 26.0(`0208770`)과 함께 이 묶음의 글 단계였던 다섯 커밋이다. 그때 결과 소절을 쓰지 않았다(26.3 의 열린 점 5, 26.4 의 열린 점 5 의 5). 이 절이 그 자리를 채운다. 커밋의 차례대로 적는다.
1. `9288cc7`(12:10Z) 주기 준비 뒤 세 검토와 그 검증의 기록(`../records/closure-review-20261119/`). `counts.txt` 에 따르면 지적은 22(blocking 4, major 13, minor 5)다. 진짜 20 가운데 부팅 전 5(`rules-01`, `rules-02`, `rules-04`, `builds-04`, `semantics-01`)이고, 진짜 아님은 2(`rules-07`, `rules-08`; 이미 적혀 있음)다.
2. `ac5b70a`(12:29Z) PI 결정 기록(`Architecture/spec/PI_DECISIONS_RESOLVED_20261119.md`): 결정 ①–⑥, lead 가 고친 적용(§2 A1–A8), 기준 커밋 뒤의 사실(§3), 적용의 차례(§4), PI 가 확인할 것(§5), lead 가 맡은 열린 것(§6).
3. `b071f02`(12:44Z) 판정 규칙 8.14 의 1–8: `R0` 의 이름과 결합, (ii)의 자리, 요구 순서의 읽기, 주 결과, 항목이 없는 호출, 코드가 규칙에 못 미친 셋, 읽기 갈래의 답, 바꾸지 않는 것.
4. `adfdb51`(12:48Z) 평가 절차 14.0–14.11.
5. `3535b43`(12:55Z) 명세 11절의 1–16 과 11.3 의 표.

**시험.** 모두 글 커밋이라 돌린 시험이 없다. 커밋마다 이름 점검과 금지어 grep 을 했다고 그 단계가 보고했으나, 그 출력은 남지 않았다.

**측정과 공간.** 남은 값은 PI 결정 기록 §5 의 6 에 적힌 것 하나다: 12:29Z 의 388,460,544 B [M](바닥 위 73,887,744 B). 그 단계의 다른 측정은 남긴 파일이 없어 수를 적지 않는다.

**계획과 다르게 한 것.**
1. 차례의 어긋남: 부팅 선언 `860c2cd` 가 이 글들보다 먼저 커밋되었다(26.0 (가), 평가 절차 14.0). 그 뒤 새 부팅 선언 2(`549dc94`)와 3(`13cf028`)이 대신했다.
2. PI 결정 기록이 인용한 판정 규칙의 줄 번호는 `ac5b70a` 의 것이다. 8.14 를 더한 뒤로 지금 파일에서 2 뒤에 있다(26.4 의 열린 점 6).
3. 이 절은 시간의 차례 밖에서 26.5 뒤에 쓴다.

**열린 점.** 이 글들이 PI 에게 남긴 것은 26.0 (더)와 PI 결정 기록 §5 그대로다(26.5 의 열린 점 3 이 하나를 더함). 이 절로 26.3 의 열린 점 5 와 26.4 의 열린 점 5 의 5 는 닫는다.

### 26.7 lead 의 판단: 부팅을 여는 조건 (2026-11-19; 이 절 뒤의 어느 작업보다 먼저 커밋함)

근거: 26.5 의 고침과 그 열린 것, 26.6, 평가 절차 14.13, 부팅 선언 3(`boot/declaration3.tsv`).

**(가) `expect_discriminate` 의 고른 길.** 26.5 가 고른 길을 확인한다: 규칙 (3)(찾은 사용마다 열린 세대 위의 인스턴스 연결)은 (1)·(2)의 요구가 없는 대조 행에만 건다(평가 절차 14.13 의 1). 부팅 전에 커밋한 평가 절차 7절의 표로 돌아가는 길이고, 결과를 보고 고른 것이 아니다. 이 주기에 그 판정은 정적 입력에서 정해지므로 '정적·형식' 칸에 두고 그렇게 보고한다.

**(나) 공간.** 부팅 선언 3 이 계산한 필요(빈 공간 1,036,296,448 B)를 26.2 의 방법으로 채운다. 이번 묶음이 만든 예행과 합성 기록의 디렉터리(`obs1/xr5_*`)와, 부팅 단계가 읽지 않는 이번 묶음의 다른 중간 출력을 xz tar 로 바꾸고, 파일마다 왕복 대조하고, 장부 줄을 고친다. 바꿀 목록과 필요를 먼저 커밋한다. 하나뿐인 바이트를 지우지 않는다. 그래도 모자라면 부팅하지 않고 PI 에게 묻는다.

**(다) `eval.txt` 의 자리.** 부팅마다 약 1 MB 인 평가 출력 전문은 남긴 출력 디렉터리에 두고 sha256 과 장부 줄을 단다. 저장소의 기록에는 그 갈래별 수(머리와 꼬리)만 커밋한다.

**(라) 부팅을 여는 조건.** 26.5 의 고침(판정 부분, 평가 스크립트 개정 2, 실행 스크립트, 부팅 선언 3, 예측 3, 평가 절차 14.13, 명세 11.5)을 따로 검토하고 검증해, 부팅 전에 고칠 것이 없으면 부팅한다. 있으면 글 먼저, 코드와 fixture, 다시 동결, 새 부팅 선언의 차례로 고친 뒤 부팅한다. 부팅은 평가 절차 2절과 가장 새 부팅 선언대로 한다: `c1_pre_tx` 먼저, 다음 `c1_post_tx`; 첫 명령은 verify; 부팅 수 규칙과 26.0 (타)의 다시 한 부팅의 읽기; 문을 지나지 못한 부팅은 사유를 덧붙임으로 커밋한 뒤에 다시 한다. 첫 부팅 뒤에는 읽는 부분만 고칠 수 있다(평가 절차 12절).

**(마) 부팅 뒤의 보고.** 결과 27절에 평가 절차 10절과 14절의 틀로 낸다: 주 결과 `D1/R0` 의 표를 먼저 두고 보조 갈래의 표에는 바꾼 조건을 적는다; 부팅별 분포로 내고 하나로 합치지 않는다; 반증 점검은 '부팅으로 틀릴 수 있음'과 '정적·형식'을 따로 센다; 기대와 맞지 않는 줄은 평가 절차 14.13 의 7 대로 있는 그대로 적는다; 정밀도와 계획 안의 재현율은 정의되지 않음(읽는 사람 없음)으로 적는다; 쌍 종류와 실패 징후는 쌍 종류마다 가른다; `unordered` 는 '이 개정이 간선을 찾지 못함'으로 적는다; 탐지나 주장의 말을 쓰지 않는다.

**(바) PI 에게 남는 것.** PI 결정 기록 5절의 항목(9 와 A9 를 포함)은 그대로 남는다. 부팅은 그 답을 기다리지 않는다: 주 결과는 부팅 전에 선언되었고 PI 의 뒤 답은 함께 보고할 보조 갈래만 고른다(평가 절차 14).

### 26.8 부팅을 위한 공간 2: 26.3 의 남긴 출력(`obs1/xr5_*`)을 xz tar 로 (2026-11-19; §26.7 (나), 부팅 없음)

**계획 (어느 압축보다 먼저 커밋함) [M][I].** 기록: `../records/closure-space2-20261119/`. 이 절의 수는 그 디렉터리의 파일에서 읽었다.

- **시작 여유**: 758,841,344 B(18:08:36Z, `free_start.txt`).
- **필요량**(`need.txt`): 부팅 선언 3 의 머리에서 읽었다. 첫 부팅 전에 빈 공간 1,036,296,448 B 가 있어야 한다. 바닥 314,572,800 B, 부팅 12 번 621,723,648 B(하나 51,810,304 B; 26.2 `need.txt` [I]), 여유 100,000,000 B 의 합이다. 18:10:54Z 의 758,681,600 B 는 277,614,848 B 모자란다.
- **목록**(`listing.tsv`, `listing.py`): 26.3–26.5 가 obs1 에 남긴 디렉터리를 두 길로 찾았다. 26.2 를 닫은 커밋 `c7b9d90` 뒤에 더한 장부 행의 root, 그리고 26.2 끝(13:57Z) 뒤에 바뀐 obs1 의 디렉터리다. 두 길 모두 `xr5_*` 열둘이다. 26.4 와 26.5 는 obs1 에 출력을 남기지 않았다(두 절의 공간).
  - 열둘 모두 바꿀 수 있다. 장부 행이 하나씩이고 `sha256c`·local `yes` 다. 목록이 디스크의 정규 파일을 빠짐없이, 더함 없이 이름 대고 sha256 이 모두 같다. 링크나 다른 형의 항목이 없다. 다른 장부 행이 가리키지 않는다.
  - 부팅 단계의 파일(`boot/run_cycle.sh`, `boot/commands.txt`, `boot/declaration3.tsv`, `boot/test_run_cycle.sh`)과 동결 파일 셋은 `xr5_` 를 적지 않는다. 부팅 단계가 obs1 에서 읽는 것은 `cz_plans_v4_ns2`, `c1p2_*`, `cz_final_extract`, `cz_final_order`, `xr4_dry_inputs` 다.
  - 이름 대는 커밋된 스크립트는 둘이고 끝난 단계의 것이다. 26.3 의 `closure-explain-fix-20261119/scripts/runs.sh` 와 26.5 의 `closure-prebootfix-20261119/scripts/runs.sh` 다. 아직 돌 비교가 아니다.
  - 합: 파일 7,677, 겉 크기 367,485,823 B, 블록 385,576,960 B.
- **차례**(`chosen.tsv`): 줄마다 그 앞에서 여유가 필요량에 닿았는지 보고, 닿았으면 멈춘다.
  - 1 합성 생성기의 첫 시도(`xr5_synth_c1_pre_tx_script_trial1`, 대신됨). 2 동결 전 스냅숏의 평가 출력(`xr5_eval_dry`; 26.5 의 다시 한 평가가 대신함). 3–6 합성 기록 넷(시험 입력, 증거가 아님). 7–8 시험 dump 예행 둘. 9–10 정적 예행 둘. 11 2026-11-16 입력의 회귀. 12 후보 다시 돎(커밋된 디렉터리와 바이트가 같음; 얻는 것이 가장 작음).
  - 26.7 (라)의 검토가 고칠 것을 찾아 다시 돌 때 먼저 견줄 것을 뒤에 두었다.
  - 기대 [I]: 26.2 의 압축 28 개에서 가장 큰 비율 0.072835 를 썼다. 그러면 8 번째 줄 뒤에 1,041,642,153 B 로 필요량에 닿고, 모두 하면 1,116,175,213 B 다. 내용이 JSON 설명 기록이라 비율은 짐작이다. 실제 값은 돌며 잰다.
  - 멈춘 뒤의 줄은 목록에 남긴다. 뒤 단계가 시작 때 잰 값이 필요량 밑이면(평가 절차 14.9) 같은 방법으로 쓸 수 있다.
- **방법**: 26.2 의 압축과 같다. `archive.sh`, `run_seq.sh`, `ledger_rewrite.py` 는 26.2 의 것을 옮겼다. 바꾼 것은 머리글, 층 이름 `A`, 필요량 줄 이름, 이 단계의 기록 디렉터리, 장부 머리 줄이다. 자체 시험은 다섯 모두 통과했다(`selftest.txt`).
  - 한 디렉터리씩 감시 아래에서 한다: 목록 대조, xz tar, 압축의 sha256 과 목록의 사본, 압축에서 흘린 파일마다의 왕복, 그다음에만 지우기.
  - 그 뒤 장부 행을 `parts` 로 고친 제안 장부를 `ledger_check.sh` 로 점검하고(rc 0), 그다음에만 장부를 바꾼다. 장부 머리에는 이 단계의 줄 하나를 첫 고침 때 더한다.
  - 압축은 `/home/user/work/preserve/archives-20261119/<이름>` 에 둔다.
- **잃는 것**: 압축은 바이트·mode·소유자·시각을 보존한다. 지운 경로를 그대로 읽는 위의 두 스크립트는 그대로는 다시 돌지 않는다. 26.7 (라)의 검토 뒤에 회귀·예행·합성 맞댐을 다시 하면, 견줄 디렉터리를 압축에서 scratch 로 풀고 `MEMBERS.sha256` 로 점검한 뒤 읽는다.
- 시작 점검: `ledger_check` 205 행 실패 0(`ledger/check_start.txt`).

**결과 [M]** (`progress.tsv`, `archive/`, `ledger/`, `guard/`, `free_end.txt`). cFS 를 부팅하지 않았고 push 하지 않았다.

- **압축**: `chosen.tsv` 의 1–8 줄을 했다(`run_seq.sh 1 1`, 이어서 `run_seq.sh 2 12`). 파일 5,649, 원본 블록 305,188,864 B, 압축 1,949,632 B(디렉터리마다 96,184–367,868 B)다.
  - 압축 바이트 / 블록은 0.004468–0.013920 이다. 기대 [I] 의 0.072835 보다 훨씬 작다.
- **압축 여덟 모두 PASS**: 1 단계 목록 대조와 5 단계 지우기 전의 왕복이다. 흘린 멤버 5,649 개의 sha256 이 모두 목록과 같고, 멤버 이름이 디스크의 파일과 같았다. 압축은 `/home/user/work/preserve/archives-20261119/<이름>` 에 있고, 그 디렉터리는 이제 40 개, 66,793,472 B 다.
- **장부**: 압축마다 행을 `parts` 로 고쳤다. 고친 제안 장부를 `ledger_check.sh` 로 점검한 뒤에만 장부를 바꿨다(여덟 번 모두 205 행, 실패 0). 장부 머리에 이 단계의 줄 하나를 더했다(`c367219`).
- **멈춤**: 9 번째 줄 앞에서 여유 1,059,622,912 B 가 필요량 1,036,296,448 B 위라 멈췄다. 9–12 줄(정적 예행 둘, 회귀, 후보 다시 돎; 블록 80,388,096 B)은 건드리지 않았다. 7 번째 줄 뒤의 1,035,550,720 B 는 필요량에 745,728 B 모자랐다.
- **여유(df -B1 /)**: 시작 758,841,344 B(18:08:36Z), 첫 압축의 감시 시작 757,628,928 B(18:14:43Z), 여덟째 압축의 감시 끝 1,059,663,872 B(18:16:06Z). 끝은 1,059,168,256 B(18:20:17Z, `free_end.txt`)다. **필요량을 채웠다.** 필요량보다 22,871,808 B 위이고, 바닥 위로 744,595,456 B 다.
- 감시 기록 12 개는 모두 `verdict=completed` 다. 가장 낮은 값은 757,006,336 B(`guard/a_1_xr5_synth_c1_pre_tx_script_trial1.log`)로 바닥 위 442,433,536 B 다.
- **끝 점검**: `ledger_check.sh` 205 행 실패 0(`ledger/check_end.txt`). `tools_check.sh --deep` 59 행 실패 0(`tools_check_deep_end.txt`). 도구와 등록부는 바뀌지 않았다.
- 커밋마다 이름 점검(`--cached`, 새 디렉터리 `--tree`)과 금지어 grep 을 했다.
- `xr5_` 를 이름 대는 커밋된 스크립트의 줄은 42 이다(`scripts_naming_archived.txt`). 모두 26.3 과 26.5 의 `runs.sh` 에 있고, 남은 넷을 이름 대는 줄도 그 안에 있다. 아직 돌 단계의 것은 없다.

**계획과 다르게 한 것.**
1. 자체 시험의 감시 기록 첫 줄(명령줄)의 scratch 경로를 돈 뒤에 `<scratch>` 로 고쳐 적었다. 앞 단계 기록과 같은 꼴이다. 다른 줄은 그대로다.
2. 장부는 ASCII 로만 적혀 있어, 머리 줄에 26.7 (나)를 `26.7 item (b), space` 로 적었다.

**열린 점.**
1. 끝의 여유는 필요량보다 22,871,808 B 만 위다. 이 단계 동안에도 다른 쓰기로 여유가 줄었다: 18:08:36Z–18:14:43Z 에 1,212,416 B, 18:16:06Z–18:20:17Z 에 495,616 B. 이 빠르기면 몇 시간 안에 필요량 밑으로 내려갈 수 있다 [I].
   - 26.7 (라)의 검토와 부팅 단계는 시작 때 다시 잰다(평가 절차 14.9). 밑이면 `run_seq.sh 9 12` 로 남은 넷을 같은 방법으로 쓸 수 있다. 블록 80,388,096 B 이고, 이 단계의 비율이면 거의 다 나온다 [I]. 새 목록은 필요 없다(멈춘 뒤의 줄은 커밋된 목록에 남음).
   - 그 뒤로는 이 규칙으로 고를 디렉터리가 없다. 26.7 (나)에 따라 PI 에게 묻는다.
2. 26.7 (라)의 검토가 고칠 것을 찾아 회귀·예행·합성 맞댐을 다시 하면, 압축한 여덟 가운데 견줄 것을 scratch 로 풀고 `MEMBERS.sha256` 로 점검한 뒤 읽어야 한다. 가장 큰 것의 블록은 53,350,400 B 다. 그 공간은 위 1 의 여유에서 나온다.
3. 압축은 같은 디스크에만 있다. remote 는 여전히 none 이다(26.2 열린 점 3).
4. 부팅 하나의 필요량 [I] 은 그대로다. 첫 부팅의 감시 기록으로 고친다(26.2 열린 점 2).
   - (2026-11-19 덧붙임, 26.9; `fix-decl-03`) 위 '필요량을 채웠다'는 선언 3 의 값(부팅 하나 51,810,304 B)에 기댄다. 남긴 측정으로 다시 적은 값(54,484,992 B; 평가와 기대 대조의 출력, 16 갈래 × 46 기록의 꼴)으로는 필요가 1,068,392,704 B 이고, 이 절 끝의 1,059,168,256 B 는 그에 못 미쳤다. 26.9 가 남은 9–12 줄을 쓴다.
5. 다음 단계의 차례(26.7 (라)): 26.5 의 고침을 따로 검토·검증하고, 고칠 것이 없으면 평가 절차 2절과 부팅 선언 3 대로 부팅한다(`c1_pre_tx` 먼저, 첫 명령은 `verify`).

### 26.9 26.5 고침의 따로 한 검토와 부팅 전 고침: 평가 출력의 자리, 한 번만 쓰는 출력, 부팅마다의 공간, 부팅 선언 4 (2026-11-19; §26.7 (라), 부팅 없음)

**무엇과 차례 (계획; 어느 코드나 압축보다 먼저 커밋함) [M][I].** lead 의 판단 26.7 (라)대로 26.5 의 고침을 따로 검토하고 검증했다. 지적 18 이 모두 진짜였고 10 이 부팅 전의 것이었다(`../records/closure-review-bootfix-20261119/findings.tsv`; 아래의 `fix-…` 는 그 번호). 판정 부분, 반증·평가 스크립트, 기대 파일, 동결은 바꾸지 않는다(부팅 전의 지적 가운데 그것들에 닿는 것이 없다). 실행 스크립트, 그 시험, 명령이 바뀌므로 새 부팅 선언 4 가 선언 3 을 대신한다. 차례는 평가 절차 14.14 의 9 다.
1. 이 커밋: 검토 기록, 평가 절차 14.14, 명세 11.6, 이 절의 계획과 날짜 붙인 주, 공간 단계의 계획(`../records/closure-space3-20261119/`), 26.5·26.8 의 날짜 붙인 주.
2. 공간 단계의 압축(26.8 의 목록 9–12 줄).
3. `run_cycle.sh`, `test_run_cycle.sh`, `commands.txt` 의 고침.
4. 감시 아래의 시험.
5. 부팅 선언 4 와 기대 입력 점검 4.
6. verify(N = 0) 두 빌드, 끝 점검, 이 절의 결과.

**지적마다 할 것 (부팅 전, 10).**
- `fix-judge-01`, `fix-decl-08`: 평가 출력 전문은 작업 디렉터리의 `c1cyc_<s>_eval<N>/eval.txt`, 기록 디렉터리에는 머리와 갈래마다의 꼬리(`eval_tail.txt`)만; 새 단계 `keep` 이 sha256 목록과 장부 행을 단다(14.14 의 1). 26.7 (다)를 실행 스크립트가 한다. 명세 11 의 15 의 '작은 글, 기록 디렉터리'를 대신한다.
- `fix-decl-05`: `control`·`kzt-text`·`chain`·`stop`·`gate` 는 출력을 한 번만 쓴다(14.14 의 2).
- `fix-decl-12`: 부팅하지 않은 부팅 단계의 규칙(14.14 의 3).
- `fix-judge-02`, `fix-decl-01`, `fix-decl-02`, `fix-decl-03`: 부팅 하나의 필요를 남긴 측정에서 다시 적고(54,484,992 B [I]; `../records/closure-space3-20261119/need.txt`), 부팅마다 남은 부팅의 수로 필요를 정하며, 부팅 단계가 스스로 재어 못 미치면 부팅하지 않는다(14.14 의 4). 지금 잰 공간(19:25:13Z 1,035,726,848 B, 감시 기록 `g01_need.log` 의 끝 값)은 첫 필요 1,068,392,704 B 에 32 MB 남짓 모자라므로, 26.8 이 남긴 9–12 줄을 같은 방법으로 쓴다(아래 공간 계획).
- `fix-decl-10`: `explain` 과 `summary` 앞의 다시 재기, `sheet_boot.tsv` 의 거절(14.14 의 6).
- `fix-decl-11`: 센 항목은 디렉터리가 아닌 모든 항목(14.14 의 7).

**새 선언에 함께 넣는 것 (부팅 전이 아니던 지적; 선언 4 를 어차피 만들므로).**
- `fix-judge-05`: 시험 두 경우(`run_cycle.sh` 의 커밋되지 않은 고침, 센 디렉터리가 없음).
- `fix-judge-06`: `verify_at_boot.txt` 를 쓰기 전에 앞의 것을 새 이름으로(14.14 의 2).
- `fix-judge-07`: `fix-decl-11` 과 같다.
- `fix-judge-08`: `gate` 단계의 주석을 고친다.
- `fix-decl-06`: 선언 4 의 머리가 `predictions3.tsv` 를 근거로 적고, '`expect` 단계와 `eval` 의 F1 만 읽음'을 `expect_c1_2.tsv` 에만 단다.
- `fix-decl-09`: 선언의 고정 파일(14.14 의 5).

**날짜 붙인 주 (2026-11-19, 부팅 전; 결과 27 이 따름).**
- (`fix-judge-03`) 동결한 반증·평가 스크립트(`03b9c95d…`)는 단위마다의 `note` 를 600 글자에서 표시 없이 자른다(`explain_eval.py` 의 `emit`). F3 지남의 `note` 는 수명 끝으로 면한 생성의 목록 뒤에 'slot clear only (weak: judgement rules 8.13 item 13)' 부분을 쓴다. 그래서 같은 종류·글의 끝난 생성이 많은 단위에서는 약함의 표지가 잘릴 수 있다. 검증은 합성 세계에서 그것을 다시 보였다(id 끝으로 면한 생성 11 과 slot clear 만으로 면한 생성 1: `note` 가 정확히 600 글자, 'weak' 없음). 이 주기에 그럴 가능성은 작다 [I]. 동결은 다시 열지 않는다. 결과 27 은 부팅마다 남긴 `eval.txt` 에서 `note` 가 정확히 600 글자인 F3 줄의 수를 세어 적고(예: `awk -F'\t' '$1=="F3" && length($10)==600'`), 그런 줄이 있으면 그 단위의 약한 생성을 기록의 `publish_events`·`lifetime_ends` 에서 이름 대거나 'weak mark not shown' 이라 적으며, 그 지남을 강한 지남으로 보고하지 않는다. 다음 평가 스크립트 개정에서 수를 먼저 쓰고 목록을 자르며 잘림을 '(cut)' 으로 적고, 면한 생성이 많은 fixture 를 더한다.
- (`fix-judge-04`) 다음 넷은 fixture 가 없다. (가) (1)의 요구가 있고 대안 줄의 문맥이 둘 이상이며 sidecar 문맥 줄이 없는 대조 행(기대 `boot`), (나) (2)만 있고 sidecar 문맥 줄이 있는 대조 행(기대 `static`), (다) 경로 A 기록의 찾은 사용에 `namespace-entry` 줄이 없음(기대 `unlinked`, `plan_missing(need not planned)`, 까닭 'no namespace-entry line'), (라) 찾은 사용에 손실 표지와 묶이지 않은 수명 끝 site 가 함께 있음(기대 `record_insufficient(site not bound in the build)`; `lifetime_complete` 가 묶이지 않은 site 를 먼저 봄). 코드는 지금 글을 따른다(검증의 탐침). 이 주기의 대조 행은 넷 가운데 어느 꼴에도 닿지 않는다(`plan_state = control` 인 행에 sidecar 문맥 줄이 없고(문맥 줄 둘은 `tier+control` 이라 세지 않는 결함 빌드의 A21 의 것), (2)만의 행이 없으며, 경로 A 행 넷은 두 빌드의 `plan.side` 에 `namespace-entry` 줄이 있다). 결과 27 은 fixture 수 옆에 이 넷을 시험되지 않은 꼴로 적는다. 다음 동결에서 fixture 로 더한다.
- (`fix-judge-08`) 부팅 선언 3 의 `run_cycle.sh`(`aa3d535c…`) 171–172 줄의 주석('이름 붙은 손실이 닿는 필요는 `unknown-loss`')은 `ef6d38f` 뒤로 맞지 않는다. 다스리는 규칙은 동결한 판정 부분(`explain_judge.py` 의 `NONEVENT_LOSS` 와 `loss_any`)과 선언 3 의 머리다: `no-build-id`·`object-table` 의 이름 붙은 손실은 손실 표지가 아니고 어느 필요도 `unknown-loss` 로 만들지 않는다. 그 밖의 이름 붙은 손실은 지금대로 손실 표지다. 결과 27 은 그 규칙을 판정 부분과 선언의 머리로 인용하고 주석으로 인용하지 않는다. 고친 `run_cycle.sh` 는 주석을 고친다.
- (`fix-decl-06`) 선언 3 의 39 줄 '(expect_c1_2.tsv; predictions2.tsv)' 의 `predictions2.tsv` 는 지금 `expect/predictions3.tsv`(`d8a83e6a…`, 값은 `predictions2.tsv` 와 같음)가 담는 값이다. 16–18 줄의 'read only by the expect step and by F1 of the eval step' 은 `expect/expect_c1_2.tsv` 하나를 두고 한 말이다. 어느 단계도 예측 파일을 읽지 않는다(`boot/no_expect_input_check3.txt`). 선언 4 의 머리가 그렇게 적는다.
- (`fix-decl-09`) 결과 27 은 빌드마다 부팅이 끝난 뒤 남긴 모든 `verify*.txt` 의 `declaration sha256=` 머리와 `boot/declaration_pin.txt` 가 같음을 보이고, 첫 부팅 뒤로 선언 파일의 커밋이 없음을 `git log` 로 보인다. 다르면 찾은 대로 적고 12절의 지위를 준다.

**공간 계획 (어느 압축보다 먼저 커밋함) [M][I].** 기록: `../records/closure-space3-20261119/`. 수는 그 디렉터리의 파일에서 읽었다.
- 시작 여유: 1,035,780,096 B(19:24:39Z, `free_start.txt`). 이 단계의 첫 측정 1,036,095,488 B(19:21:05Z, `../records/closure-review-bootfix-20261119/out/free_start.txt`)는 이미 선언 3 의 필요 1,036,296,448 B 에도 200,960 B 모자랐다. 26.8 끝(18:20:17Z) 뒤로 다른 쓰기와 이 검토의 scratch 로 23,072,768 B 가 줄었다.
- 필요(`need3.py`, `need.txt`; 감시 기록 `guard/g01_need.log`): 부팅 하나 54,484,992 B(부분은 14.14 의 4; 선언 3 보다 2,674,688 B 큼), `c1_pre_tx` 첫 부팅 앞 1,068,392,704 B, `c1_post_tx` 첫 부팅 앞 741,482,752 B. 19:25:13Z 의 1,035,739,136 B 는 32,653,568 B 모자란다.
- 목록(`chosen.tsv`): 26.8 의 `chosen.tsv` 9–12 줄을 그대로 옮겼다(정적 예행 둘, 2026-11-16 입력의 회귀, 후보 다시 돎; 블록 80,388,096 B). 다시 보았다: 장부 행이 하나씩이고 링크나 다른 형의 항목이 없으며, 부팅 단계의 파일·`env/obs1`·동결 파일 셋이 `xr5_` 를 적지 않는다(`git grep` 0 줄).
- 이 단계는 네 줄을 모두 쓴다. 줄 앞에서 멈추지 않는다. 까닭: 지금 모자라고, 멈춤 규칙으로는 10 번째 줄 뒤에 남는 여유(약 24 MB [I])가 26.8 끝부터 이 단계 시작까지 다른 쓰기로 준 양(23 MB)과 비슷하며, 아직 돌 어느 단계도 이 넷을 읽지 않는다. 판정 부분이 다시 바뀌어 회귀·예행을 다시 해야 하면 압축에서 scratch 로 풀어 `MEMBERS.sha256` 로 점검한 뒤 읽는다(26.8 열린 점 2 와 같음).
- 방법: 26.8 과 같다. `archive.sh`, `selftest.sh` 는 머리글만, `run_seq.sh` 는 머리글과 줄 앞의 멈춤 한 줄을 뺀 것만, `ledger_rewrite.py` 는 기록 디렉터리·절·장부 머리 줄(넷째 쓰임, 셋째 쓰임 줄 아래)만 바꿨다. 자체 시험 다섯 모두 통과(`selftest.txt`, 감시 기록 `guard/g02_selftest.log`). 시작 점검 `ledger_check` 205 행 실패 0(`ledger/check_start.txt`; 장부 sha256 `6763efd1…`).

**결과 [M]** (`../records/closure-review-bootfix-20261119/`, `../records/closure-space3-20261119/`; README 와 `commands.txt` 가 파일과 차례를 적음). cFS 를 부팅하지 않았고 push 하지 않았다. 커밋의 차례: `1522f83` 검토 기록·글·계획 → `8d94350` 압축 → `adfc67d` 실행 스크립트·시험·명령 → `66397fc` 시험 한 경우 더함 → `4152b47` 부팅 선언 4 → 이 커밋(verify, 끝 점검, 이 결과).

| 번호 | 한 것 | 다시 돈 것과 그 결과 |
|---|---|---|
| `fix-judge-01`, `fix-decl-08` | `eval` 단계: 전문은 `<작업>/c1cyc_<s>_eval<N>/eval.txt`(그 디렉터리가 있으면 rc 2), 기록 디렉터리에는 `eval_tail.txt`(26.5 의 뽑는 법)만; 기록 디렉터리의 `eval.txt`·`eval_tail.txt` 가 있어도 rc 2; 새 단계 `keep`(목록, 장부 행, 제안 장부의 `ledger_check.sh`, 실패한 시도는 `*.failed.*` 로 남고 다시 돎); 명세 11 의 15 의 '작은 글, 기록 디렉터리'를 14.14 의 1 이 대신함 | 시험 20(전문의 자리, 꼬리의 줄, 두 번째 `eval` rc 2), 22(목록과 행, 빈·없는 디렉터리는 행 없음, 두 번째 `keep` rc 2), 25(제안 장부의 점검 실패: rc 1, 장부 그대로, 다시 돎). 변이 `eval_into_record_dir`, `keep_no_ledger_check` 가 시험에서 어긋남 |
| `fix-decl-05` | `control`, `kzt-text`, `chain`, `stop`, `gate` 는 출력이 있으면 rc 2(14.14 의 2). 옮겨 남기는 길은 쓰지 않음 | 시험 14(FAIL 문이 두 번째 `gate` 뒤에도 FAIL, 출력의 sha256 그대로, `gate*` 파일 하나). 변이 `gate_no_refusal`, `control_no_refusal` 이 어긋남 |
| `fix-decl-12` | 부팅하지 않음: 명령이 0 이 아닌 rc 로 끝나고 `$RUN` 이 없으면 빈 디렉터리를 지우고 `boot.notbooted.<UTC ns>.txt` 를 남기며 고정 파일을 쓰지 않음(14.14 의 3). `convert`·`explain`·`eval` 의 시작 전 거절(rc 75)도 디렉터리를 남기지 않음 | 시험 16(rc 3 → 디렉터리 없음, 기록 하나, 고정 없음; 같은 N 이 다시 돎), 23(`convert` 가 두 번 rc 75, 디렉터리 없음, 거절 기록 둘). 변이 `not_booted_rule_off`, `convert_refusal_keeps_dir` 가 어긋남 |
| `fix-judge-02`, `fix-decl-01`, `fix-decl-02`, `fix-decl-03` | 부팅 하나 54,484,992 B [I](`need.txt`); 부팅 단계가 남은 부팅의 수로 필요를 재고 `space_at_boot.txt` 를 남기며 못 미치면 rc 1(14.14 의 4); 이미 한 부팅이 남긴 바이트가 더 크면 그것을 씀; 선언 4 의 `need` 줄; 26.8 목록 9–12 줄의 압축 | 시험 17(`c1_pre_tx` 1 의 필요 14,000,000 = 1M + 1M + 12 × 1M, 1 B 모자라면 부팅 안 함, 같으면 부팅; `c1_post_tx` 1 은 6 ×), 18(측정 3,035,136 B 가 선언 1,000,000 B 보다 커서 씀). 변이 `no_space_check` 가 어긋남. 압축 넷 모두 왕복 PASS(멤버 2,028), 장부 넷 번 점검 205 행 실패 0, 여유 1,034,665,984 → 1,114,177,536 B |
| `fix-decl-10` | `explain` 이 같은 N 의 verify 를 다시(`verify_at_explain.txt`), `summary` 가 N = 5 의 verify 를 다시(`verify_at_summary.txt`, `boot5/gate.txt` 필요); `eval` 은 언제나 `--sheets-boot none`, `sheet_boot.tsv` 가 있으면 rc 2 | 시험 20(`sheet_boot.tsv` 가 있으면 rc 2, 평가 디렉터리 없음; 꼬리에 `sheets-boot none`), 21(선언된 파일이 바뀌면 `explain` rc 1, 설명 출력 없음), 24(`summary` 는 `boot5/gate.txt` 없이 rc 2, 바뀐 파일로 rc 1). 변이 `eval_sheet_accepted`, `explain_no_reverify` 가 어긋남 |
| `fix-decl-11`, `fix-judge-07` | `count` 줄은 `find ! -type d`(14.14 의 7) | 시험 13(심볼릭 링크 하나로 `differs`). 변이 `count_regular_files_only` 가 어긋남. 두 빌드의 값은 31 그대로(정규 파일이 아닌 항목 0, `out/free_start.txt` 의 시점에 셈; `commands.txt`), 선언 4 의 verify 에서 `31 31 ok` |
| `fix-judge-05` | 시험 두 경우 | 시험 11(`run_cycle.sh` 의 커밋되지 않은 고침: 부팅 안 함), 12(센 디렉터리 없음: `missing`). 검증이 쓴 변이 둘(`git_check_declaration_only`, `count_missing_as_zero`)이 이제 어긋남. 26.5 의 표 아래 날짜 붙인 주 |
| `fix-judge-06` | 다시 쓰는 기록(`verify.txt`, `verify_at_*`, `space_at_boot.txt`, `explain.txt`, `boot.txt`, `eval_run.txt`)은 쓰기 전에 앞의 것을 `<이름>.<UTC ns>.<확장자>` 로 | 시험 15(중첩 verify 가 거절한 부팅을 다시 함: `verify_at_boot` 기록 둘, 앞의 것에 `differs 1`). 변이 `verify_at_boot_overwritten` 이 어긋남 |
| `fix-decl-09` | 고정 파일 `boot/declaration_pin.txt`(주기의 첫 부팅이 씀; 그 뒤의 부팅·`explain`·`eval`·`summary` 는 sha256 이 같아야 돎; 고정 없이 도는 부팅은 `c1_pre_tx` 1 뿐) | 시험 19(커밋된 선언 고침 뒤의 부팅 2 거절, 고정 없는 `c1_post_tx` 1 거절). 변이 `no_pin_check` 가 어긋남 |
| `fix-judge-08` | `gate` 의 주석을 판정 부분의 `NONEVENT_LOSS` 로 고침; 결과 27 은 판정 부분과 선언 머리를 인용 | 없음(주석) |
| `fix-decl-06` | 선언 4 의 머리: 값은 `predictions3.tsv` 의 것, 단계가 읽는 기대 파일은 `expect_c1_2.tsv` 하나(`eval` 의 F1 과 `expect`) | `boot/no_expect_input_check4.txt`: 기대 파일을 이름 대는 단계는 `eval)`·`expect)` 둘, 설명 단계 0, 두 계획의 `args.txt` 0 |
| `fix-judge-03`, `fix-judge-04` | 위 날짜 붙인 주(결과 27 이 따름). 동결은 그대로 | 없음 |

**시험 [M]** (모두 감시 아래; 출력은 scratch 에 쓰고 결과와 sha256 을 남긴 뒤 지움).
- `boot/test_run_cycle.sh`: 개발 실행 1(커밋 전)은 24/25 였다. 부팅하지 않음의 경우가 뒤 실행이 덮어쓴 `boot.out` 을 읽은 시험의 잘못이었고, 코드는 맞게 했다(`out/dev_run_cycle_tests1.txt`; 시험을 고쳐 `adfc67d` 에 넣음). `adfc67d` 위에서 25/25(`out/run_cycle_tests.txt`).
- 변이 실행 1(`scripts/mutants.sh`; 한 변이마다 고침 하나를 한 줄로 뺀 `run_cycle.sh` 에 바뀌지 않은 시험): 13/14 가 시험에서 어긋났다. `keep` 의 장부 점검을 뺀 변이는 통과했다(그 변이는 장부도 지웠음; 시험에 점검이 실패하는 경우가 없었다). 고정의 변이는 뒤의 모든 부팅을 거절해 다른 까닭으로 어긋났다(`out/mutants.trial1.txt`).
- 그래서 시험 25 를 더하고 고정의 경우가 고정 문구 자체를 보게 했다(`66397fc`; 개발 실행 2 26/26, `out/dev_run_cycle_tests2.txt`). 두 변이는 점검만 빼는 한 줄 고침으로 좁혔다.
- `66397fc` 위에서 26/26(`out/run_cycle_tests_final.txt`; 두 스크립트의 sha256 앞뒤 같음), 변이 14/14 가 시험에서 어긋남(`out/mutants.txt`). `bash -n` 깨끗함.

**부팅 선언 4 [D]** (`boot/declaration4.tsv`, `b268e2d2…`, 줄 165; `4152b47`). `declaration3.tsv`(`a6d242be…`)는 그대로다.
- 옛 줄 159 를 다시 쟀다(`declaration/remeasure_declaration3.tsv`): 같음 154, plan-id 둘은 옮김, 바뀜 3(`run_cycle.sh` `2508f144…`, `commands.txt` `365ce243…`, `test_run_cycle.sh` `f76b63f9…`; `declaration/changes_vs_declaration3.tsv` 가 커밋을 적음). `count` 줄 둘은 새 셈(디렉터리가 아닌 항목)으로도 31 이라 같다.
- 더한 줄 6: `need` 셋(바닥 314,572,800, 여유 100,000,000, 부팅 하나 54,484,992; `need.txt` 에서 읽음), `env/tools/ledger_check.sh`, `records/closure-space3-20261119/need.txt`, `boot/no_expect_input_check4.txt`.
- **verify 한 번**: `run_cycle.sh verify <빌드> 0` 을 두 빌드에 돌렸다. 둘 다 rc 0, `rows 162 ok 162 differs 0 missing 0`, `need` 줄은 값으로 적힘(`boot/<빌드>/boot0/verify.txt`; 26.5 의 출력은 `verify.20261119T194536.428774735Z.txt`, `verify.20261119T194538.455749636Z.txt` 로 남음). 고정 파일은 아직 없다(부팅 없음).

**공간 [M].**
- 압축(`../records/closure-space3-20261119/progress.tsv`): 9–12 줄 넷, 파일 2,028, 블록 80,388,096 B 가 xz tar 379,628 B(줄마다 4,088–128,092 B)가 되었다. 압축 디렉터리는 이제 44 개, 67,502,080 B 다. 장부 sha256 `6d9cc76a…`.
- 여유: 시작 1,036,095,488 B(19:21:05Z), 압축 앞 1,034,665,984 B(19:29:23Z), 압축 뒤 1,114,177,536 B, 선언 때 1,113,055,232 B(19:44:35Z), 끝 1,112,498,176 B(19:48:04Z, `out/free_end.txt`). **`c1_pre_tx` 첫 부팅 앞의 필요 1,068,392,704 B 보다 44,105,472 B 위**이고, 바닥 위 797,925,376 B 다.
- 감시 기록 16 개(이 기록 10, 공간 단계 6)는 모두 `verdict=completed` 다. 가장 낮은 값은 1,034,330,112 B(`../records/closure-space3-20261119/guard/a_9_xr5_dry_static_c1_pre_tx.log`)이고, 가장 큰 dip 는 시험의 16,683,008 B 다.

**끝 점검 [M].** `ledger_check` 205 행 실패 0(`out/ledger_check_end.txt`), `tools_check --deep` 59 행 실패 0(`out/tools_check_deep_end.txt`). 커밋마다 금지어 grep 과 이름 점검(`--cached`, 새 디렉터리 `--tree`)을 했다.

**계획과 다르게 한 것.**
1. 공간 단계는 26.8 의 디렉터리를 쓰지 않고 새 기록 디렉터리 `closure-space3-20261119` 를 두었다. 26.8 의 `run_seq.sh` 는 제 `need.txt`(선언 3 의 값)로 멈추므로, 다시 적은 필요로는 쓸 수 없었다. 스크립트는 옮겨 머리글과 멈춤 한 줄만 바꿨다.
2. 9–12 줄을 멈춤 없이 넷 모두 썼다(위 공간 계획의 까닭). 줄 앞의 멈춤 규칙(26.8)을 이 단계에서 쓰지 않은 것은 계획에 먼저 적었다.
3. 부팅 하나의 필요에 verify 출력 셋, 평가의 머리·꼬리, `keep` 의 목록을 넣었다. 지적이 든 부분(설명, 평가, 기대 대조) 밖의 것이다. 새 실행 스크립트가 부팅마다 남기는 것이기 때문이다.
4. 지적 밖의 고침: 시작 전 거절(rc 75)을 부팅 밖의 `convert`·`explain`·`eval` 에도 같은 규칙으로 다뤘다(그러지 않으면 그 번호의 단계가 다시 돌지 못함). `keep` 의 실패한 시도를 `*.failed.*` 로 남겨 다시 돌게 했다.
5. 부팅 전이 아니던 `fix-judge-05`, `-06`, `-07`, `-08`, `fix-decl-06`, `-09` 는 선언 4 를 어차피 만들므로 코드에 넣었다. `fix-judge-03`, `-04` 는 동결을 다시 열지 않으려고 글로만 두었다.
6. 시도 출력은 새 이름으로 남겼다: 개발 실행 둘(`out/dev_run_cycle_tests{1,2}.txt`), 변이 실행 1(`out/mutants.trial1.txt`, 감시 기록 `guard/mutants.trial1.log`).
7. 감시 기록 첫 줄의 scratch 경로는 돈 뒤에 `<scratch>` 로 고쳐 적었다(앞 단계와 같은 꼴).

**열린 점.**
1. 부팅 하나의 필요는 [I] 다. 첫 부팅 뒤로는 부팅 단계가 이미 한 부팅이 남긴 바이트로 스스로 고친다. 끝의 여유는 첫 필요보다 44 MB 남짓 위이고 다른 쓰기로 계속 준다 [I](26.8 열린 점 1 의 빠르기면 몇 시간). 부팅 단계가 거절하면 남은 압축 줄이 없으므로 PI 에게 묻는다(26.0 (카), 26.7 (나)).
2. 압축은 같은 디스크에만 있다(26.2 열린 점 3).
3. 결과 27 이 할 것: 부팅마다 `space_at_boot.txt` 와 감시 기록의 dip, 부팅하지 않은 단계가 있으면 그 까닭; F3 `note` 의 600 글자 줄 수(`fix-judge-03`); 시험되지 않은 꼴 넷(`fix-judge-04`); 모든 `verify*.txt` 의 선언 sha256 과 고정 파일(`fix-decl-09`); 부팅마다 `run.meta` 의 `other=` 와 `inst_files.sha256.other`(정규 파일이 아닌 항목; 이제 verify 도 셈); 고정 파일, 목록, 꼬리, 장부를 함께 커밋.
4. 다음 단계의 차례(26.7 (라)): 부팅 선언 4 와 `boot/commands.txt` 대로 `c1_pre_tx` 먼저 부팅한다(root; 첫 명령은 `verify`, 부팅마다 열두 단계, 끝에 `summary`). 그다음 `c1_post_tx`.

## 27. C1 주기: 부팅과 그 평가 (2026-11-19)

근거: lead 의 판단 26.7 (라)·(마), 평가 절차 2·3절과 14.14, 부팅 선언 4(`../records/closure-c1prep-20261119/boot/declaration4.tsv`, `b268e2d2…`), 명령 `boot/commands.txt`(`365ce243…`), 실행 스크립트 `boot/run_cycle.sh`(`2508f144…`). 읽기와 보고의 틀(26.7 (마), 평가 절차 10절과 14절)은 두 빌드의 부팅이 끝난 뒤의 절이 한다. 빌드마다의 소절(27.1 부터)은 스크립트가 낸 수만 적는다.

### 27.1 `c1_pre_tx` 의 부팅 (2026-11-19)

**차례 (첫 부팅 전에 커밋함) [D].**
- 부팅은 `boot/commands.txt` 의 줄을 그대로, 저장소 뿌리에서, 차례대로 돌린다. 부팅마다 열두 단계(verify, boot, control, convert, kzt-text, chain, stop, gate, explain, eval, expect, keep)이고, 단계마다 rc 를 본 뒤 다음으로 간다. 감시는 실행 스크립트가 거는 단계(boot, convert, explain, eval)에 걸린다. 실행 스크립트, 선언, 판정 부분, 기대는 바꾸지 않는다.
- 부팅 수(평가 절차 2절, 14.10; 선언 4 의 머리): 첫 부팅이 문(3절; `boot/c1_pre_tx/boot1/gate.txt`)을 지나면 무엇을 보였든 정확히 네 번 더(N = 2–5) 하고 `summary` 를 돌린다. 지나지 못하면 그 사유를 `boot/c1_pre_tx/addendum_1r.txt` 에 적어 커밋한 뒤 1r 을 한 번 한다. 1r 이 지나면 2–5 와 `summary`, 지나지 못하면 이 빌드의 부팅은 거기서 끝난다. 기록을 본 뒤에 수를 올리거나 내리지 않는다. 한 모든 부팅을 세고, 문을 지나지 못한 부팅은 사유와 함께 적는다. 부팅하지 않음(14.14 의 3)은 부팅으로 세지 않고, 까닭을 없앤 뒤 같은 N 으로 다시 하며 그 일을 여기 적는다.
- 공간: 첫 부팅 앞의 필요는 1,068,392,704 B 다(선언 4). 이 단계의 첫 측정은 1,110,761,472 B(19:53:02Z, `../records/closure-boots-20261119/free_start.txt`), 42,368,768 B 위다. 부팅 단계가 부팅마다 스스로 잰다(`space_at_boot.txt`). 못 미쳐 거절하면 남은 압축 줄이 없으므로 부팅하지 않고 PI 에게 묻는다(26.0 (카), 26.7 (나)).
- 부팅마다 커밋하는 것: 실행 스크립트의 기록 디렉터리 `boot/c1_pre_tx/boot<N>/` 전부(작은 출력, 제어 점검, 연결 사슬, 문, 평가의 머리와 갈래마다의 꼬리, 기대 대조, `keep` 의 sha256 목록, 감시 기록), 장부, 첫 부팅 뒤의 `boot/declaration_pin.txt`, 그리고 이 단계의 기록 `../records/closure-boots-20261119/c1_pre_tx/`(단계마다의 시각·rc·여유 `steps.tsv`, 부팅마다 뽑은 수 `boot-<N>/`; README 가 파일을 적음). 큰 출력(작업 디렉터리와 덤프, 변환, 설명 기록, `eval.txt`)은 `/home/user/work/redesign/obs1/c1cyc_pre_{boot,convert,explain,eval}<N>/` 에 두고 `keep` 단계가 장부 행을 단다.
- 부팅마다 아래에 날짜 붙인 한 줄을 더한다.

**부팅 [M].**
- **부팅 1** (2026-11-19, 19:57:19Z–19:58:45Z; 기록 `boot/c1_pre_tx/boot1/`, 뽑은 수 `../records/closure-boots-20261119/c1_pre_tx/boot-1/index.txt`; 단계마다 rc 0, `steps.tsv`). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 1,109,737,472 B, need 1,068,392,704 B, 41,344,768 B 위; `space_at_boot.txt`). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 100,268 B, `kz-rt-dump exit=0`, instance files 31, other 0. 고정 파일 `boot/declaration_pin.txt`(`b268e2d2…`, 19:57:38Z). control: `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,103 줄. convert rc 0(events 1103, losses 0). chain `confirmed`(16 links; `same_build` 는 `unconfirmed(object not in the build record)`, 7 objects 가운데 confirmed 1; 문에 들지 않음). stop: applications 5, reached inside window 5. **gate PASS**(losses: fields none; records none). explain rc 0: 16 갈래 모두 기록 46, refused 0, uses 181. eval rc 0(본문 7,095 줄, 파일 7,602 줄, form ok): 주 갈래 D1-R0 의 반증 점검 boot pass 31 / 160(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe boot 0 / 0, static 20 / 20; expect_discriminate met 1 / counted 4(unmet 3, not_counted 1; static 5). `note` 가 600 글자인 F3 줄 0 / 752. expect rc 0: match 1,040, mismatch 64, no_record 1,104(boot: 400, 64, 464; static: 640, 0, 640). keep: 장부 행 4, `ledger_check` 209 행 실패 0. 이 부팅이 남긴 블록 60,403,712 B(기록 663,552, 부팅 2,764,800, 변환 741,376, 설명 55,001,088, 평가 1,232,896; `large_outputs.tsv`)로 선언 값 54,484,992 B 보다 크다. 다음 부팅의 공간 점검은 그 큰 값을 쓴다(14.14 의 4). 감시 기록의 가장 낮은 값 1,049,698,304 B(eval), 가장 큰 dip 55,009,280 B(explain). 문을 지났으므로 부팅 수 규칙대로 N = 2–5 를 한다.
- **부팅 2** (2026-11-19, 19:59:57Z–20:00:47Z; 기록 `boot/c1_pre_tx/boot2/`, 뽑은 수 `../records/closure-boots-20261119/c1_pre_tx/boot-2/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 1,048,117,248 B, need 1,018,609,920 B = 바닥 + 여유 + 10 × 60,403,712 B(부팅 1 의 측정), 29,507,328 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 100,268 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,103 줄. convert rc 0(events 1103, losses 0). chain `confirmed`(16 links; `same_build` 은 부팅 1 과 같은 글). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 181). eval rc 0(본문 7,079 줄, 파일 7,586 줄): D1-R0 반증 점검 boot pass 31 / 159(fail 128, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 1 / counted 4(unmet 3, not_counted 1; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 213 행 실패 0. 남긴 블록 60,420,096 B. 감시 최저 988,200,960 B(eval), 가장 큰 dip 55,029,760 B(explain).
- **부팅 3** (2026-11-19, 20:01:38Z–20:02:13Z; 기록 `boot/c1_pre_tx/boot3/`, 뽑은 수 `../records/closure-boots-20261119/c1_pre_tx/boot-3/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 986,775,552 B, need 958,353,664 B = 바닥 + 여유 + 9 × 60,420,096 B(부팅 2 의 측정), 28,421,888 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 100,268 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,103 줄. convert rc 0(events 1103, losses 0). chain `confirmed`(16 links). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 181). eval rc 0(본문 7,095 줄, 파일 7,602 줄): D1-R0 반증 점검 boot pass 31 / 160(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 1 / counted 4(unmet 3, not_counted 1; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 217 행 실패 0. 남긴 블록 60,444,672 B. 감시 최저 926,777,344 B(eval), 가장 큰 dip 55,050,240 B(explain).
- **부팅 4** (2026-11-19, 20:02:57Z–20:03:33Z; 기록 `boot/c1_pre_tx/boot4/`, 뽑은 수 `../records/closure-boots-20261119/c1_pre_tx/boot-4/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 925,376,512 B, need 898,130,176 B = 바닥 + 여유 + 8 × 60,444,672 B(부팅 3 의 측정), 27,246,336 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 100,268 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,103 줄. convert rc 0(events 1103, losses 0). chain `confirmed`(16 links). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 181). eval rc 0(본문 7,095 줄, 파일 7,602 줄): D1-R0 반증 점검 boot pass 31 / 160(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 1 / counted 4(unmet 3, not_counted 1; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 221 행 실패 0. 남긴 블록 60,424,192 B. 감시 최저 865,472,512 B(eval), 가장 큰 dip 55,029,760 B(explain).
- **부팅 5** (2026-11-19, 20:04:54Z–20:05:45Z; 기록 `boot/c1_pre_tx/boot5/`, 뽑은 수 `../records/closure-boots-20261119/c1_pre_tx/boot-5/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 864,006,144 B, need 837,685,504 B = 바닥 + 여유 + 7 × 60,444,672 B(부팅 3 의 측정), 26,320,640 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 100,268 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,103 줄. convert rc 0(events 1103, losses 0). chain `confirmed`(16 links). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 181). eval rc 0(본문 7,095 줄, 파일 7,602 줄): D1-R0 반증 점검 boot pass 31 / 160(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 1 / counted 4(unmet 3, not_counted 1; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 225 행 실패 0. 남긴 블록 60,375,040 B. 감시 최저 804,147,200 B(eval), 가장 큰 dip 54,984,704 B(explain).
- **summary** (2026-11-19, 20:06:46Z–20:06:51Z; rc 0): `boot5/gate.txt` 있음, N = 5 의 verify 다시 `rows 162 ok 162 differs 0 missing 0`(`boot/c1_pre_tx/verify_at_summary.txt`), 문을 지난 부팅 다섯(boot1–boot5)으로 `boot/c1_pre_tx/summary.txt`(1,489 줄): `summary branches 16 uses 6 form ok`.

**부팅의 목록 [M]** (`../records/closure-boots-20261119/c1_pre_tx/steps.tsv`: 단계 61, rc 가 0 이 아닌 단계 0). `c1_pre_tx` 에서 한 부팅은 다섯이고 모두 문을 지났다. 문을 지나지 못한 부팅, 다시 한 부팅(1r), 부팅하지 않음(14.14 의 3)은 없다. 부팅 수 규칙대로 첫 부팅 뒤 정확히 넷을 더 했다.

| N | 시각(UTC) | 공간: free / need (B) | gate | D1-R0 반증 점검 boot pass / 셈 (fail) | 기대 대조 match / mismatch / no_record | 남긴 블록 (B) |
|---|---|---|---|---|---|---|
| 1 | 19:57:19–19:58:45 | 1,109,737,472 / 1,068,392,704 | PASS | 31 / 160 (129) | 1,040 / 64 / 1,104 | 60,403,712 |
| 2 | 19:59:57–20:00:47 | 1,048,117,248 / 1,018,609,920 | PASS | 31 / 159 (128) | 1,040 / 64 / 1,104 | 60,420,096 |
| 3 | 20:01:38–20:02:13 | 986,775,552 / 958,353,664 | PASS | 31 / 160 (129) | 1,040 / 64 / 1,104 | 60,444,672 |
| 4 | 20:02:57–20:03:33 | 925,376,512 / 898,130,176 | PASS | 31 / 160 (129) | 1,040 / 64 / 1,104 | 60,424,192 |
| 5 | 20:04:54–20:05:45 | 864,006,144 / 837,685,504 | PASS | 31 / 160 (129) | 1,040 / 64 / 1,104 | 60,375,040 |

표의 수는 스크립트가 쓴 그대로다(`space_at_boot.txt`, `gate.txt`, `eval_tail.txt`, `expect_compare.txt`, `large_outputs.tsv`). 읽기는 두 빌드의 부팅 뒤의 절이 평가 절차 10절·14절과 14.13 의 7 대로 한다.

**선언의 고정 (26.9 `fix-decl-09`) [M]** (`../records/closure-boots-20261119/c1_pre_tx/pin_check.txt`). `boot/declaration_pin.txt` 는 `b268e2d2…`(19:57:38Z, `c1_pre_tx` 1 의 부팅 단계)다. `c1_pre_tx` 의 `boot1`–`boot5` 에서 선언 줄이 있는 `verify*.txt` 16 개가 모두 그 값을 적는다. `verify_at_boot.txt`, `verify_at_explain.txt`, `verify_at_summary.txt` 는 다시 돈 verify 의 표준 출력 한 줄(`rows 162 ok 162 differs 0 missing 0`)만 담는다. 그 verify 가 쓴 `verify.txt` 는 다음 다시 재기 때 `verify.<UTC ns>.txt` 로 옮겨 남는다. 첫 부팅 앞의 커밋 `99f43ee` 뒤로 `declaration4.tsv`·`run_cycle.sh` 의 커밋은 0, 커밋되지 않은 고침도 0 이다.

**공간 [M].** 시작 1,110,761,472 B(19:53:02Z). 부팅마다의 판정은 위 표대로 모두 `met` 이고, 위로 남은 것은 41,344,768 B 에서 26,320,640 B 로 줄었다. 부팅 단계는 부팅 2 부터 이미 한 부팅이 남긴 가장 큰 값(60,403,712 → 60,420,096 → 60,444,672 B)을 썼다(14.14 의 4). 감시 기록 20 개는 모두 `verdict=completed` 이고, 가장 낮은 값은 804,147,200 B(부팅 5 의 eval), 가장 큰 dip 은 55,050,240 B(부팅 3 의 explain)다. 끝 802,512,896 B(20:09:50Z, `c1_pre_tx/free_end.txt`). 같은 규칙으로 `c1_post_tx` 첫 부팅 앞의 필요는 314,572,800 + 100,000,000 + 6 × 60,444,672 = 777,240,832 B 이고, 끝의 여유는 그보다 25,272,064 B 위다. 그 부팅 단계가 스스로 다시 잰다.

**끝 점검 [M].** `ledger_check` 225 행 실패 0(`c1_pre_tx/ledger_check_end.txt`; 부팅마다 `keep` 이 넷씩 더해 205 → 225), `tools_check --deep` 59 행 실패 0(`c1_pre_tx/tools_check_deep_end.txt`). 커밋마다 금지어 grep 과 이름 점검을 했다.

**계획과 다르게 한 것.**
1. 과제가 적은 기록 자리 `records/closure-boots-20261119/c1_pre_tx/boot-<N>/` 에는 이 단계가 쓴 것(단계의 출력, 뽑은 수, 큰 출력의 sha256 목록의 sha256 과 장부 행, 실행 스크립트 파일의 sha256)만 두었다. 실행 스크립트의 기록(작은 출력, 제어 점검, 연결 사슬, 문, 평가의 머리와 꼬리, 기대 대조, `keep` 의 목록)은 선언된 실행 스크립트가 쓰는 자리 `boot/c1_pre_tx/boot<N>/` 에 그대로 두고 커밋했다. 실행 스크립트는 첫 부팅 뒤에 바꿀 수 없다.
2. `summary` 단계가 N = 5 의 verify 를 다시 돌려 `boot5/verify.txt` 를 다시 썼다. 앞의 것은 `boot5/verify.20261119T200646.331211208Z.txt` 로 남았다(같은 선언 sha256). `boot-5/runner_files.sha256` 은 그 앞의 상태를 적는다. 두 파일은 `summary` 와 함께 커밋했다.
3. `eval` 의 표준 출력이 적는 줄 수(본문, 예: 7,095)와 `eval_tail.txt` 머리의 줄 수(파일 전체, 예: 7,602)는 세는 대상이 다르다. 둘 다 그대로 적었다.

**열린 점.**
1. `c1_post_tx` 의 공간: 끝의 여유는 그 첫 부팅의 필요보다 25,272,064 B 위이고, 다른 쓰기로 계속 준다 [I](이 단계 앞 19:48Z–19:53Z 에 약 1.7 MB). 다음 단계는 곧 시작하는 것이 좋다. 부팅 단계가 거절하면 남은 압축 줄이 없으므로 PI 에게 묻는다(26.0 (카), 26.7 (나)).
2. 부팅 하나의 필요는 이제 측정값이다: 가장 큰 60,444,672 B, 선언 값 54,484,992 B 보다 5,959,680 B 크다. 거의 모두 16 갈래의 설명 기록(부팅마다 블록 54,976,512–55,042,048 B; `large_outputs.tsv`)이다.
3. 26.9 가 결과 27 에 남긴 것 가운데 이 단계가 낸 것: F3 `note` 가 600 글자인 줄은 부팅마다 0 / 752(`fix-judge-03`), 선언 고정의 대조(`fix-decl-09`, 위), 부팅마다 `run.meta` 의 `other=0` 과 `inst_files.sha256.other` 0 줄. 시험되지 않은 꼴 넷(`fix-judge-04`)과 읽기는 보고의 절이 적는다.
4. D1-R0 의 boot 반증 점검 fail(128–129)과 기대 대조의 mismatch 64(모두 boot 종류)는 스크립트가 낸 그대로다. 줄마다의 까닭은 보고의 절이 평가 절차 14.13 의 7 대로 있는 그대로 적는다.
5. 압축과 남긴 출력은 같은 디스크에만 있다(26.2 열린 점 3).

### 27.2 `c1_post_tx` 의 부팅 (2026-11-19)

**차례 (첫 부팅 전에 커밋함) [D].**
- 27.1 의 차례와 같다. 빌드만 `c1_post_tx` 다(평가 절차 2절, 26.7 (라): `c1_pre_tx` 먼저, 다음 `c1_post_tx`). 부팅은 `boot/commands.txt` 의 줄을 그대로, 저장소 뿌리에서, 차례대로 돌린다. 부팅마다 열두 단계(verify, boot, control, convert, kzt-text, chain, stop, gate, explain, eval, expect, keep)이고, 단계마다 rc 를 본 뒤 다음으로 간다. 감시는 실행 스크립트가 거는 단계(boot, convert, explain, eval)에 걸린다. 단계는 `99f43ee` 의 `step.sh`, 뽑은 수는 같은 커밋의 `extract.sh` 로 낸다. 두 스크립트는 이미 `c1_post_tx` 를 받으므로 고치지 않는다. 실행 스크립트, 선언, 판정 부분, 기대는 바꾸지 않는다(평가 절차 12절).
- 부팅 수(평가 절차 2절, 14.10; 선언 4 의 머리): 첫 부팅이 문(3절; `boot/c1_post_tx/boot1/gate.txt`)을 지나면 무엇을 보였든 정확히 네 번 더(N = 2–5) 하고 `summary` 를 돌린다. 지나지 못하면 그 사유를 `boot/c1_post_tx/addendum_1r.txt` 에 적어 커밋한 뒤 1r 을 한 번 한다. 1r 이 지나면 2–5 와 `summary`, 지나지 못하면 이 빌드의 부팅은 거기서 끝난다. 기록을 본 뒤에 수를 올리거나 내리지 않는다. 한 모든 부팅을 세고, 문을 지나지 못한 부팅은 사유와 함께 적는다. 부팅하지 않음(14.14 의 3)은 부팅으로 세지 않고, 까닭을 없앤 뒤 같은 N 으로 다시 하며 그 일을 여기 적는다.
- 공간: 실행 스크립트의 규칙(14.14 의 4)으로 첫 부팅 앞의 필요는 314,572,800 + 100,000,000 + 6 × 60,444,672 = 777,240,832 B 다(부팅 하나의 값은 이미 한 부팅이 남긴 가장 큰 값, `c1_pre_tx` 부팅 3). 이 단계의 첫 측정은 801,075,200 B(20:14:43Z, `../records/closure-boots-20261119/c1_post_tx/free_start.txt`), 23,834,368 B 위다. 시작의 장부 점검은 225 행 실패 0(`c1_post_tx/ledger_check_start.txt`). 부팅 단계가 부팅마다 스스로 잰다(`space_at_boot.txt`). 못 미쳐 거절하면 남은 압축 줄이 없으므로 부팅하지 않고 PI 에게 묻는다(26.0 (카), 26.7 (나)).
- 부팅마다 커밋하는 것: 실행 스크립트의 기록 디렉터리 `boot/c1_post_tx/boot<N>/` 전부, 장부, 이 단계의 기록 `../records/closure-boots-20261119/c1_post_tx/`(`steps.tsv`, 부팅마다 뽑은 수 `boot-<N>/`). 큰 출력은 `/home/user/work/redesign/obs1/c1cyc_post_{boot,convert,explain,eval}<N>/` 에 두고 `keep` 단계가 장부 행을 단다. `boot/declaration_pin.txt` 는 `c1_pre_tx` 1 이 쓴 그대로이고 이 빌드의 부팅 단계는 그것을 읽기만 한다.
- 부팅마다 아래에 날짜 붙인 한 줄을 더한다.

**부팅 [M].**
- **부팅 1** (2026-11-19, 20:16:08Z–20:17:36Z; 기록 `boot/c1_post_tx/boot1/`, 뽑은 수 `../records/closure-boots-20261119/c1_post_tx/boot-1/index.txt`; 단계마다 rc 0, `c1_post_tx/steps.tsv`). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 800,350,208 B, need 777,240,832 B = 바닥 + 여유 + 6 × 60,444,672 B(`c1_pre_tx` 부팅 3 의 측정), 23,109,376 B 위; `space_at_boot.txt`). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 106,812 B, `kz-rt-dump exit=0`, instance files 31, other 0. 고정 파일은 읽기만 했다(`b268e2d2…`, 바뀌지 않음). control: `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,177 줄. convert rc 0(events 1177, losses 0). chain `confirmed`(16 links; `same_build` 는 `unconfirmed(object not in the build record)`, 7 objects 가운데 confirmed 1; 문에 들지 않음). stop: applications 5, reached inside window 5. **gate PASS**(losses: fields none; records none). explain rc 0: 16 갈래 모두 기록 46, refused 0, uses 178. eval rc 0(본문 7,111 줄, 파일 7,586 줄, form ok): 주 갈래 D1-R0 의 반증 점검 boot pass 32 / 161(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe boot 0 / 0, static 20 / 20; expect_discriminate met 2 / counted 5(unmet 3, not_counted 0; static 5). `note` 가 600 글자인 F3 줄 0 / 752. expect rc 0: match 1,040, mismatch 64, no_record 1,104(boot: 400, 64, 464; static: 640, 0, 640). keep: 장부 행 4, `ledger_check` 229 행 실패 0. 이 부팅이 남긴 블록 60,477,440 B(기록 667,648, 부팅 2,781,184, 변환 765,952, 설명 55,013,376, 평가 1,249,280; `large_outputs.tsv`)로 이제까지 가장 크다. 다음 부팅의 공간 점검은 그 값을 쓴다(14.14 의 4). 감시 기록의 가장 낮은 값 740,261,888 B(eval), 가장 큰 dip 55,021,568 B(explain). 문을 지났으므로 부팅 수 규칙대로 N = 2–5 를 한다.
- **부팅 2** (2026-11-19, 20:18:27Z–20:18:57Z; 기록 `boot/c1_post_tx/boot2/`, 뽑은 수 `../records/closure-boots-20261119/c1_post_tx/boot-2/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 738,713,600 B, need 656,482,560 B = 바닥 + 여유 + 4 × 60,477,440 B(부팅 1 의 측정), 82,231,040 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 105,932 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,167 줄. convert rc 0(events 1167, losses 0). chain `confirmed`(16 links; `same_build` 은 부팅 1 과 같은 글). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 176). eval rc 0(본문 7,111 줄, 파일 7,586 줄): D1-R0 반증 점검 boot pass 32 / 161(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 2 / counted 5(unmet 3, not_counted 0; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 233 행 실패 0. 남긴 블록 60,477,440 B. 감시 최저 678,768,640 B(eval), 가장 큰 dip 55,021,568 B(explain).
- **부팅 3** (2026-11-19, 20:19:29Z–20:19:59Z; 기록 `boot/c1_post_tx/boot3/`, 뽑은 수 `../records/closure-boots-20261119/c1_post_tx/boot-3/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 677,363,712 B, need 596,005,120 B = 바닥 + 여유 + 3 × 60,477,440 B(부팅 1 의 측정), 81,358,592 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 106,460 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,173 줄. convert rc 0(events 1173, losses 0). chain `confirmed`(16 links). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 178). eval rc 0(본문 7,111 줄, 파일 7,586 줄): D1-R0 반증 점검 boot pass 32 / 161(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 2 / counted 5(unmet 3, not_counted 0; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 237 행 실패 0. 남긴 블록 60,477,440 B. 감시 최저 617,418,752 B(eval), 가장 큰 dip 55,021,568 B(explain).
- **부팅 4** (2026-11-19, 20:20:41Z–20:21:11Z; 기록 `boot/c1_post_tx/boot4/`, 뽑은 수 `../records/closure-boots-20261119/c1_post_tx/boot-4/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 615,985,152 B, need 535,527,680 B = 바닥 + 여유 + 2 × 60,477,440 B(부팅 1 의 측정), 80,457,472 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 106,460 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,173 줄. convert rc 0(events 1173, losses 0). chain `confirmed`(16 links). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 178). eval rc 0(본문 7,111 줄, 파일 7,586 줄): D1-R0 반증 점검 boot pass 32 / 161(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 2 / counted 5(unmet 3, not_counted 0; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 241 행 실패 0. 남긴 블록 60,477,440 B. 감시 최저 556,036,096 B(eval), 가장 큰 dip 55,025,664 B(explain).
- **부팅 5** (2026-11-19, 20:21:33Z–20:22:04Z; 기록 `boot/c1_post_tx/boot5/`, 뽑은 수 `../records/closure-boots-20261119/c1_post_tx/boot-5/index.txt`; 단계마다 rc 0). verify `rows 162 ok 162 differs 0 missing 0`. 공간 `verdict met`(free 554,569,728 B, need 475,050,240 B = 바닥 + 여유 + 1 × 60,477,440 B(부팅 1 의 측정), 79,519,488 B 위). boot rc 0: tx·rx `operational_at_5s=yes`, `stop=ci-es-restart-poweron`, `tx_exit=0 rx_exit=0`, 덤프 111,036 B, instance files 31, other 0. control `check ok (0 failures)`, 손실 필드·손실 기록 없음, pool overflow 0, `ev` 1,225 줄. convert rc 0(events 1225, losses 0). chain `confirmed`(16 links). stop 5 / 5. **gate PASS**(losses none). explain rc 0(16 갈래 × 기록 46, refused 0, uses 192). eval rc 0(본문 7,111 줄, 파일 7,586 줄): D1-R0 반증 점검 boot pass 32 / 161(fail 129, not evaluable 120), static 137 / 137, format 51 / 51; expect_safe static 20 / 20(boot 0 / 0); expect_discriminate met 2 / counted 5(unmet 3, not_counted 0; static 5). F3 600 글자 0 / 752. expect: match 1,040, mismatch 64, no_record 1,104(boot 400, 64, 464; static 640, 0, 640). keep 4 행, 장부 245 행 실패 0. 남긴 블록 60,903,424 B(기록 671,744, 부팅 2,793,472, 변환 786,432, 설명 55,402,496, 평가 1,249,280). 감시 최저 494,198,784 B(eval), 가장 큰 dip 55,410,688 B(explain). 선언된 부팅 다섯(1 과 정확히 넷 더)을 다 했으므로 `summary` 를 돌린다.

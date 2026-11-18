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

### 7.8 열린 것

- **본문 없는 함수의 읽기 전용 효과**: 더해진 주소 탈출의 다수는 글을 읽기만 하는 것으로 보이는 호출(`strcmp`, `strncmp`, `strlen`, `OS_strnlen`, `printf`, 가변 인자의 이벤트·로그 호출)이다. 이 효과를 사실로 가지면(C 라이브러리 함수의 효과는 P0 표의 비교 갈래와 함께) writer-set-complete 의 일부와 C1 A20 의 `known` 키가 돌아올 수 있다. 효과의 대조는 링크한 C 라이브러리 판의 글이 필요하고 그 글은 이 작업 환경에 없다(새 외부 자원, `unverified`). 자리: 의미와 확인 상태 단계(P0 항목의 의미).
- **A20 의 대조**(§6.10 의 2, (가)): 옵션 있는 facts 의 A20 은 키 `unresolved` 이고 SCH_LAB 생성과 `unordered` 짝을 갖는다. 사전 시험의 대조 선택 규칙(manifest 개정 4 의 생성기)은 이 행을 (가)로 넣지만, 기대([I])를 커밋할 때 이 facts 를 쓰면 키 글의 기대가 옵션 없는 facts 와 다르다. 어느 facts 로 기대를 적는지는 마지막 옵션 회차 뒤에 정한다.
- **같은 호출 안의 out-pointer 쓰기와 그 호출의 사용**(R2 B111): order 가 이를 한 스레드의 `unknown` 짝으로 센다. 같은 호출의 쓰기를 사용의 일부로 볼지 order 규칙에 정할 일이다.
- **함수·객체 키의 절대 위치**: 형 이름만 소스 트리 기준이 되었고, 루트 밖 함수의 키(`initialize_throttling@/home/user/work/procrace/c1_build/pre/…`)와 위치는 절대 경로다. 그래서 같은 관계의 키가 c1_pre_tx 와 c1_post_tx 에서 다르다(A20 `35f07539e7606e2e` 대 `bdcc6978b9a41c8c`; 키 글 `A|OS_common_record_t.name_entry|initialize_throttling@/home/user/work/procrace/c1_build/{pre,post}/src/bp/fsw/src/bp_flow.c|OS_CountSemGetIdByName#1|1`, `a20_keys.txt`). 빌드 사이 비교 단계가 이 키를 쓰기 전에 정한다. 계획의 ARG_TYPES 참조 글에도 절대 위치가 든다(`plan_key_c1.txt`).
- **`count.type_names_outside_root` 의 뜻**(§7.7): facts 에 쓰인 이름만 세도록 할지, 문서의 글을 고칠지.
- 이 회차의 출력은 중간 회차다. 마지막 옵션 회차(루프 술어·잠금 정체성 단계)가 섹션 단위로 견주고, 같으면 이 회차를 “뒤 회차로 다시 만들 수 있음”으로 적는다.

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
- **첫 관측 경로가 쓰는 항목**(C1 관계 1, 사용 지점 키 `17975444d83f6589`, c1_pre_tx 의 A21 행): 그 행이 이름 대는 함수 가운데 framework 항목이 있는 것 15(`CFE_ES_ExitApp`, `CFE_ES_ResetCFE`, `OS_BinSemCreate`, `OS_CondVarCreate`, `OS_CountSemCreate`, `OS_CountSemGetIdByName`, `OS_DirectoryOpen`, `OS_FileSysAddFixedMap`, `OS_ModuleLoad`, `OS_MutSemCreate`, `OS_OpenCreate`, `OS_QueueCreate`, `OS_SocketBind`, `OS_TaskCreate`, `OS_TimeBaseCreate`), P0 항목 6(`abort`, `dlsym`, `exit`, `memset`, `pthread_exit`, `strcmp`). 곧 적용된 158 framework 항목 가운데 15, 적용된 P0 선언 35 가운데 6. 그 15 가운데 13 은 unverified 칸이 하나씩 있다(일치를 비교 못 함). c1_post_tx 의 같은 키 행은 16 과 7 이다(`OS_TaskDelay`, `clock_nanosleep` 더함). c1_pre_tx 계획의 site 가 이름 대는 19 함수 가운데 framework 항목이 있는 것 11, P0 3(`memset`, `pthread_create`, `strncpy`). r2·c5 에는 그 키가 없다.
- **`OS_CountSemGetIdByName`**(osal-f34759d, 첫 경로의 사용 호출): 문서 반환 {0, -2, -13, -17}, 본문은 그에 더해 -35, -16, -33(`OS_ObjectIdConvertToken` 을 거친 것)을 돌려준다: "the body returns a value the documentation omits". 본문 8 항목 body-derived, 문서 4 항목 doc-collated.
- **비교 못 함의 큰 몫**은 본문이 돌려주는 값의 집합이 서지 않는 함수다(c1 111): 따라가지 못한 반환(간접 호출의 결과, 예: `OS_NotifyEvent` 가 `<indirect>` 의 결과를 돌려줌 — c1_pre_tx 에서 이 까닭을 적은 일치 행 16)이나, 상수가 뒤에 다시 정의되는 정의를 지나 반환에 닿는다고 보이지 않는 것(relations rules revision 9 와 같은 표시)이다. c5 의 문서 반환 없음 37/35 는 OSAL 4.1.1 에 함수별 문서가 없기 때문이다(규칙 §3).
- **P0**: 검증된 P0 행은 선언 속성(doc-collated: 머리말의 `__attribute_pure__`, `__nonnull` 등)과 로컬 사본이 있는 `pthread_create` 의 두 줄(역할과 arg 0 의 out 동작, standard-collated)이다. 나머지 P0 행위 문장(역할, out 인자, 막힘)은 링크하는 판의 글이 이 환경에 없어 `unverified` 다.

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

### 8.8 열린 것

- **이 층이 방법이 받아들이는 framework adapter 층인가**: 판이 고정된 framework 소스에서 기계적으로 만든 의미 자료(`framework-layer data`)다. 그 층인지는 PI 가 정할 물음이고, 이 단계는 그렇다고 가정하지 않았다(규칙 머리).
- **P0 의 대조**: 행위 문장 47/77(c1) 등은 `unverified` 다. 링크하는 판의 글(로컬 사본 밖의 glibc 2.39 소스, POSIX 글)이 새 외부 자원이라 하지 않았다. 배포 패치도 대조하지 않았다(changelog 에 `pthread_create` 줄 1).
- **범위 밖**: 플랫폼 지원 묶음과 응용, framework 의 단위 시험 stub. cFE 6.7.99(6cccb3f)·7.0.1 의 이름 공간은 저장된 이름이 필드 경로가 아니라 접근 함수로 닿아 규칙 T 가 잡지 못하고, 그 두 판의 이름 공간 파일은 없다(규칙 §7).
- **이름 공간 자료는 kz-static 이 읽지 않는다**: 판정 규칙 8.2 조건 (가)의 입력으로 만들었고, 읽는 쪽(설명 스크립트의 다음 개정)은 다음 단계다 [I].
- **읽기 전용 효과**(§7.8 첫 항목): `strcmp`·`strlen` 등의 pure 선언이 이제 doc-collated 로 SEMANTICS 에 있으나, writer 집합의 계산은 아직 그것을 쓰지 않는다. 쓰는 것은 뒤 단계다.
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

`compare.sh extract_on`(`records/result-flow-20261116/compare_extract_on.txt`, rc 1)에서 c1_pre_tx·c1_post_tx·r2_core 의 `relations.txt` 비교가 선언 §9.0 의 허용 밖으로 나왔다(`compare/on_relations_<b>.txt`: 다른 줄 23/23/12; c5 두 빌드는 허용 안). 나머지 47 줄은 OK 다(basis, facts 머리, 섹션의 레코드 글, facts 보기, extract.txt, args.txt, basis.txt, sha256.txt, relations.log). 선언 밖의 차이이므로 여기서 멈추고 까닭을 적는다.

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

### 9.8 열린 것

- **event type·run status 의 등급**: 효과는 event 보내기와 그 형 상수(예: `CFE_EVS_EventType_ERROR` 3)를 적지만, 그 형이 실패를 알리는 등급인지는 자료에 없어 event 보내기만인 효과는 T? 로 둔다. 계획 생성기가 event 보내기와 그 형을 계획에 올리는 것은 뒤 단계다.
- **예산 출구가 return 이 아닌 대기**: 루프를 벗어나 지역 플래그로 이어지는 대기(`CFE_ES_GetTaskFunction`, `CFE_ES_MainTaskSyncDelay`, `CFE_ES_ApplicationSyncDelay` 형)는 호출자 쪽 사용을 정하지 못해 HB-await 가 0 이다. 루프 출구의 지역 플래그 값을 따라가는 것(루프 조건의 정의 섹션 LOOP_DEFS)이 뒤 단계다.
- **open 효과의 큰 몫**: RESULT_EFFECT 의 open 이 c1 8,549/15,930 이다(그 값이 정하지 않는 분기, 따라가지 않은 return, 주소를 받는 몸 없는 호출). A21 의 exit-app 도 `CFE_ES_RunLoop` 의 매개변수 저장 때문에 open 이다. 그 함수의 저장이 run status 를 바꾸는지는 자료의 의미 항목(run-status 인자)과 저장 값을 잇는 규칙이 있어야 정해진다.
- **c5 의 child without success 가능 다섯**: 스레드를 만든 뒤의 실패가 실제로 일어나는 조건(속성·detach 의 실패)을 판정 규칙이 어떻게 다룰지 정해야 한다 [미확인].
- **J? 16**(c5): 부재 값이 정해지지 않은 버려지는 호출이다. 부재 값의 평가가 뒤 단계다.
- 이 회차는 중간 회차다: 섹션별 레코드 글 sha256(`section_text_on.txt`)을 남겼고, 압축 보존하지 않았다.

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

### 10.8 열린 것

- **의미 자료의 `lock-object` 역할**: 버전에 묶인 헤더의 brief 줄(`osapi-mutex.h` 의 `OS_MutSemTake` “Acquire the mutex object referenced by sem_id”, `OS_MutSemGive` “Releases the mutex object referenced by sem_id”)에서 이 역할을 만드는 것은 semantics coverage rule 의 다음 개정과 생성된 자료의 다시 만들기다(그 규칙을 어느 자료보다 먼저 커밋하는 차례). 그때까지 LOCK_HANDLES 는 실제 빌드에서 static-instance 를 주지 않는다.
- **핸들 필드의 writer 둘**: `CFE_ES_Global_t.SharedDataMutex` 는 전역의 memset(0)과 생성 호출의 out-pointer 두 인스턴스다. 앞선 0 채움을 초기값으로 읽어 “한 번 도는 writer 하나”로 칠지는 LOCK_IDS 의 값 읽기 규칙과 함께 정할 일이다 [미확인: 그렇게 읽어도 건전한지].
- **2 세대 잠금 집합**: LOCKSET 은 정체성을 모르는 해제(다른 형의 잠금이라도)가 있는 호출 뒤 OSAL 잠금을 잃는다(c1 `CFE_ES_GetTaskFunction` 안 `:513` 뒤 held 0). 핸들 단위의 잡힘 집합은 잠금 집합 분석을 핸들로 다시 돌려야 하고, 그 전에는 경로 C 가 핸들 잠금을 볼 일이 드물다.
- **LOOP_DEFS 술어의 L1 인증**: HB-await 가 hard 가 되려면 그 술어의 읽기에 대한 L1 (i)/(ii) 인증이 있어야 한다. AWAIT_L1 을 바꾸지 않는 새 섹션이나 order 쪽 규칙으로 두는 것이 뒤 단계다.
- **호출자에 기대는 spawn 의 위 한계 461**(r2_core): 그 짝 가운데 실제로 순서가 있는 짝은 정해지지 않았다 [미확인]. 풀린 entry 를 넘기지 않는 호출자 셋(`OS_TimeBaseCreate` 사슬 등)의 entry 를 푸는 것이 그 수를 줄이는 길이다.
- **c5 의 대기**: 6.4.x 의 대기 다섯은 flag·value 규칙에 맞지 않는다(refused-unresolved 3, refused-address-taken 1, not-examined 1). 그 꼴(주소를 넘겨 채우는 지역, 여러 정의)을 받을지는 뒤 단계다.
- 앞 단계에서 넘어온 열린 것(event type 의 등급, open 효과, c5 의 child without success, J?, 이름 결합)은 그대로다.
- **마지막 facts**: `ll_final_extract` 가 뒤 단계의 입력이다(장부 `OBS_ll_final_extract`). 계획 생성기의 다음 판(manifest 개정 4)은 이 facts 를 읽는다.

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

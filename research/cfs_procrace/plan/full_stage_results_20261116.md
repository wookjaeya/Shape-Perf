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
- **소속 판정은 사용 자신의 인자에서 멈춘다.** callback 이 사용의 매개변수로 들어오면 그 위의 호출자는 따라가지 않고 "풀리지 않음" 이다(c1·r2 에 한 행씩).
- **다시 뽑기의 키 목록은 없다.** 선언은 다시 뽑기의 `relations.txt` 와 "keys" 를 재실행의 것과 비교한다고 적었으나, 단일 빌드 실행기는 `--keys` 를 쓰지 않는다. relations 글만 비교했다(같음). 키 목록은 재실행의 것이 기록이다.
- **새 도구의 사본을 두지 않았다**(2.4). 계획 문서는 생성기의 사본을 두라 하지만 그 규칙은 부팅에 닿는 계획의 생성기에 대한 것이다.

### 2.8 열린 것

- 설명 스크립트(`env/obs1/explain.py` 개정 2)가 새 줄(`membership:`, `id holder:`, 생성자의 `callee-check:`, `writers:`)을 담은 행을 읽는지는 이 단계에서 돌려 보지 않았다(부팅 없음, 스크립트 변경 없음). 생성자의 성공 상수는 그 스크립트의 규칙으로 서는 것만 확인했다(`creator_success.txt`). 생성자의 out id 를 실제로 읽으려면 계획이 생성자 호출의 out 을 after-call 에서 기록해야 한다(계획 생성기의 일).
- 판정 규칙 개정 2 의 이름 결합 입력 가운데 이 단계가 준 것: 종류별 표 선택(partition 은 표 선택 상수만), 소속(같은 비교 함수), 생성자 성공 상수. 남은 것: 이름 공간 항목(공통 의미 단계), 다른 이름 대응이 없음의 근거(종류마다), 삭제·회수 site, 이름 수명.
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

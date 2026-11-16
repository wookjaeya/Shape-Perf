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

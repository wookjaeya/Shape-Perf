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

### 3.8 열린 것

- 호출의 guard(facts schema 개정 11 의 호출 행에 없음): 예산 탈출 뒤 호출자 쪽 사용의 결과, HB-await 의 호출자 쪽 간선, `skip` 의 판정이 그것을 기다린다. 조건의 지역을 그 정의에 잇는 사실(reaching definitions)이 있으면 child without success 의 위치 근사도 좁힐 수 있다(C1 의 거짓 `possible`).
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
- 최종(`ctest2_full.log`): 254 개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). 확인 수: `test_static_cli` 953(911 + 42), `test_static_reasons` 660(앞 단계 655; 사유 표에 한 줄이 늘고 relations golden 의 줄이 둘 늘었다. 빌드 1 의 663 은 행 없는 relations golden 하나를 더 읽은 수), `test_static_plan` 182, `test_core_schema` 34,017, `lint_selftest` 109, lint 7/7. 단위 점검 15/15(`unit_tools.txt`), 7/7(`unit_step2a.txt`).

### 5.4 바이트 동일성과 다섯 빌드 [M]

| 비교 | 결과 |
|---|---|
| relations, T8CF facts (`compare_relations.txt`) | 다섯 빌드 `relations.txt`·`keys.txt` 가 `schema12_relcheck` 와 3 번째 줄(rules revision 8) 밖에서 바이트 동일, `relations.log` 빈 파일(15 OK) |
| order, T8CF facts (`compare_order.txt`) | 두 번 같은 바이트, `order.txt` 는 3 번째 줄 밖에서 같음, `edges.tsv` 바이트 동일, `order.log` 빈 파일(25 OK) |
| 다시 뽑기 (`compare_extract.txt`, `compare/`) | 다섯 빌드 45 OK: `basis.kz` 레코드 모두 같음(머리의 해시 둘만), `facts.kz` 머리는 해시 넷과 크기 줄(§5.0b), facts 의 보기·`extract.txt`·`relations.txt` 는 선언한 갈래만(`compare/facts_<b>.txt`, `extract_txt_<b>.txt`, `relations_<b>.txt`), `basis.txt`·`sha256.txt` 는 해시 줄만, `args.txt`·`relations.log` 같음 |
| 다시 뽑은 facts 위 (`compare_onextract.txt`) | order 두 번 같음, `order.txt` 는 선언한 갈래만(facts 해시, relations 개정, 레코드 전체 쓰기의 짝 줄 5: c1 둘·r2), `edges.tsv` 바이트 동일, `relations.txt` 는 다시 뽑기의 것과 바이트 동일, 키 표(30 OK) |
| 계획 (`compare_plans.txt`) | T8CF 위 다섯과 다시 뽑은 facts 위 다섯이 모두 "only declared differences"(site 와 필요 줄 같음) |

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
- `<anonymous>.sys_timebase_id` 는 `PSP_SoftTimebase_Global@psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase_c:44::type.sys_timebase_id` 이고 writer 가 1 → 2(memset `cfe_psp_soft_timebase.c:50:5` 가 더해짐). 앞판에서 빠진 까닭은 memset 의 대상 형을 이름 표로 찾는 `owners_contained_in`(`lib/llvm/ast_fn.cpp` 의 데이터 함수 처리)이 이름 표에 없는 형을 맞추지 못한 것이고, 이제 그 형이 이름 표에 있다.
- C1 의 `<anonymous>.State`(writers 0, complete)는 `UnSock_Global@…/unsock_intf_c:85::type.State` 이고 memset writer 1(`unsock_intf.c:231:5`)과 untyped 133·escape 1 hit 로 DISABLED 다.
- C5 에는 FACT_LOCS·FIELD_STATUS 의 그런 행이 없고, 바뀐 것은 FUNC_WRITES·LOCKSET·LOCK_PATHS 의 `record … .*` 이름이다(OSAL 4.1.1 의 이름 없는 레코드가 시스템 머리말 형 `__pthread_mutex_s::__elision_data` 등으로).
- 시스템 머리말 형도 이름을 얻는다(`siginfo_t::_sifields::_sigfault.si_addr` 등). 같은 정의는 모든 단위에서 같은 이름이다.
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
- **scratchpad 점검 빌드**는 감시 아래 돌렸다(앞 단계의 다짐대로). LLVM Release 구성으로 만들어 MC 의 것과 바이트가 다르다. 증거로 쓰지 않았고 지웠다.

### 5.8 열린 것

- 이름 없는 형이 FACT_LOCS 에서 writer-set-complete 이면서 그 배열 원소 쓰기는 FIELD_STATUS 만 본다(`CFE_SB_MemParams_t::Partition.Data`: FACT_LOCS complete writers 1, FIELD_STATUS DISABLED `array-element-stores-not-collected`). FACT_LOCS 의 규칙(배열 원소 쓰기를 세지 않음)은 앞판 그대로다.
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

# 형과 객체 접근: 추출 옵션 `--object-access` (2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/full_stage_results_20261116.md` §7. 텍스트 기록만 둔다. cFS 는 부팅하지 않았다. push 하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §7.0 과 같은 글). 그 sha256 이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh` | 다섯 빌드의 relations, order(와 다시 뽑은 facts 위의 키), 다시 뽑기(옵션 없음·있음), 계획 |
| `compare.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py` | 옵션 없는 경로의 비교(앞 단계 출력과) |
| `compare_on.py`, `name_stability.py`, `measure_on.py`, `section_sha.py` | 옵션 있는 경로: 갈래 비교, 같은 정의의 같은 이름, 뒤 출력의 측정, 회차의 섹션별 sha256 |
| `relink_expected.sha256`, `not_relink.sha256` | 빌드 전 sha256: 다시 링크되는 것 14, 다시 링크되지 않아야 하는 것 110 |
| `golden_scratch.sha256` | scratchpad 점검 빌드로 미리 만든 golden 셋의 sha256 |
| `tool_copy/` | 덮기 전 사본의 왕복 확인과 공간 감시 |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | 빌드 전 등록부와 누적 목록 |
| `df_scratch_*.log` | scratchpad 점검 빌드의 공간 감시(근거 아님) |
| `build_start.txt`, `pre_mc.txt`, `post_mc.txt`, `identity_kz-static.txt`, `relink_result.txt` | 빌드 전·뒤의 MC 해시와 도구 정체성 |
| `make_*.log`, `df_build_*.log`, `ctest_first.log`, `golden_update.log`, `ctest_full.log`, `ctest_counts.log`, `unit_tools.txt`, `unit_step2a.txt` | 빌드, 시험, 확인 수, 단위 점검 |
| `*_run.txt`, `df_*.log`, `objacc_*.sha256` | 다섯 빌드 실행의 rc, 공간 감시, 출력의 sha256 |
| `compare_*.txt`, `compare/` | 비교의 결과(옵션 없음: 앞 단계 출력과; 옵션 있음: 갈래, 이름, 측정) |
| `ledger_check_after_build.txt`, `tools_check_after_build.txt`, `ledger_check_end.txt`, `tools_check_end.txt` | 빌드 뒤, 다섯 빌드 실행 뒤의 누적 목록과 등록부 |
| `measure_basis.txt`, `plan_key_c1.txt`, `section_sha_off.txt`, `section_sha_on.txt`, `section_compare.txt`, `name_check_tree.txt` | BASIS 항목과 위치 행, A21 의 키 값, 회차의 섹션별 sha256, 바깥 이름 점검 |
| `resume_check.txt` | 세션이 다시 시작된 뒤 디스크의 도구와 출력을 기록과 맞댐 |
| `status_change.py`, `status_change.txt` | 대상 위치의 writer 집합 상태: 옵션 없음과 있음의 바뀜, 그 A-TYPE 의 바뀜, 더해진 탈출 줄의 갈래 |
| `order_changed.txt`, `order_changed_rows.txt`, `a20_keys.txt` | order 행 값이 바뀐 행과 그 relations·order·facts 보기 줄, A20 의 사용 지점 키 |
| `names_in_dump_c1.txt` | 옵션 있는 c1 facts 에 든 소스 트리 위치의 형 이름과 BASIS 의 셈 |
| `ledger_check_final.txt`, `tools_check_final.txt` | 끝의 누적 목록과 등록부(새 이름) |

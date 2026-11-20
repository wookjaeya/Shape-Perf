# 순서 간선 `kz-static order` (order rules revision 1, 2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/full_stage_results_20261116.md` §3. 텍스트 기록만 둔다. cFS 는 부팅하지 않았다. push 하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §3.0 과 같은 글). 그 sha256 이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `compare.sh`, `compare_plans.py` | 빌드 전에 커밋한 비교: relations 바이트 동일, order 두 번 실행, 다시 뽑은 facts 의 order, 계획의 선언한 줄 |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `order_extract_allow.tsv` | relations 다섯 빌드, order 다섯 빌드, 다섯 빌드 다시 뽑기(단일 빌드 실행기), 계획 다섯 빌드, 다시 뽑기의 허용 파일 |
| `tool_copy/` | 덮기 전 사본 둘(`038cd9e3`, 첫 빌드의 `b1bf545d`)의 왕복 확인과 공간 감시 |
| `pre_mc.txt`, `post_mc.txt`, `make.log`, `df_build.log`, `build_start.txt` / `…_build2…`, `build2_start.txt` | 첫 빌드 / 둘째 빌드의 MC 전·후, 빌드 출력, 공간 감시 |
| `ctest_first.log`, `golden_update.log`, `ctest_full.log`, `skip_reason.log`, `unit_tools.txt`, `unit_step2a.txt` / `…_build2…` | 시험(첫 / 둘째 빌드) |
| `declared_addendum_before_build2.txt` | 둘째 빌드 전 덧붙임(결과 문서 §3.0a). 그 sha256 이 `commands.txt` §3 에 있다 |
| `compare_order.py` | 둘째 빌드의 order 를 첫 빌드의 것과 덧붙임의 갈래로 비교 |
| `compare_*.txt`, `compare/`, `*_run*.txt`, `df_*.log` | 선언한 비교의 결과, 다시 뽑기의 파일별 비교, 실행 기록, 공간 감시 |
| `<출력 디렉터리>.sha256` | 열 출력 디렉터리의 sha256 목록(장부 행) |
| `summary_build2.txt`, `named_rows_build2.txt`, `awaited_build2.txt`, `spawn_flags.txt` | 측정(머리 줄, 이름 댄 행과 대기의 글, 인식된 대기의 사용, 첫 빌드의 spawn 간선 표시) |
| `tools_check_pre.txt`, `ledger_check_pre.txt`, `tools_check.txt`, `ledger_check.txt` | 빌드 전·뒤 등록부와 누적 목록 |

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

# 설명 기록 단계의 검토 뒤 고침 (2026-11-16)

`plan/observation_path_results_20261116.md` 7.11 의 기록이다. 선언은 7.11.0, 결과는 그 뒤.

- `run_fix.sh`: 단계 explain, copy, summary, rowsites, runtime-refs, cfe-assert, bindings. 출력은 `/home/user/work/redesign/obs1/c1pre_explain_reviewfix/`(저장소 밖, 새 디렉터리).
- `boot_budget.tsv`: 부팅 전 선언(§6.0 예산 2·4)을 옮겨 적은 예산 기록. `explain.py --boot-budget` 의 입력.
- `summary.py`: 기록 수 셈(개정 2: 사유는 기록마다 한 번).
- `row_sites.py`: 한 행의 site 를 sidecar 의 `map` 줄에서 읽고 바인딩과 사건 수를 적는다.
- `comparator_fixtures.txt`: 점검 11 의 새 fixture 를 빌드 전 비교기와 지금 비교기에 돌린 결과.
- `pre_mc.txt`, `post_mc.txt`, `unit_*`, `ledger_check.txt`, `tools_check.txt`, `df_*.log`, `commands.txt`: 점검과 공간 기록.

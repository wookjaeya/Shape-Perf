# 설명 기록과 완료 시험 (2026-11-16)

`plan/observation_path_results_20261116.md` 7절의 기록이다. 선언은 7.0, 결과는 7.1 뒤.

- `run_explain.sh`: 단계 facts-view, explain, copy, plan-checks, hand, bundle. 출력은 `/home/user/work/redesign/obs1/c1pre_explain/`(저장소 밖).
- `hand_compare_record.py`: 손 manifest 기록(2026-10-29)과 이 기록의 비교. 비교 자료이고 생성 경로의 입력이 아니다.
- `records/`: 설명 기록(JSON, 행과 소비 행위자마다 하나), `index.tsv`, `inputs.txt`, `refused.txt`, `explain.sha256`: explain 단계가 만든 것의 사본.
- `pre_mc.txt`, `post_mc.txt`, `unit_*`, `ledger_check.txt`, `tools_check.txt`, `df_*.log`, `commands.txt`: 점검과 공간 기록.
- 로컬 묶음의 글 기록은 `bundle/` 에 둔다(조각은 저장소 밖).

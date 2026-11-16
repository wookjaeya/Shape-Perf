# 추출의 기본 경로 고침: 읽기 옵션의 대조, 잘린 이미지의 소비자 둘, 이름 없는 레코드 형의 이름 (2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/full_stage_results_20261116.md` §5. 텍스트 기록만 둔다. cFS 는 부팅하지 않았다. push 하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §5.0 과 같은 글). 그 sha256 이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `pre_ref.sh`, `plan_one.sh`, `pre_ref_run.txt`, `pre_ref_check.txt`, `df_pre_ref.log`, `extfix_pre_onextract.sha256` | 빌드 전 옛 도구(5ae220d2)가 schema12_extract facts 위에 낸 relations·키·계획(비교의 참조) |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh` | 다섯 빌드의 relations, order(와 다시 뽑은 facts 위의 키), 다시 뽑기, 계획 |
| `compare.sh`, `compare_classes.py`, `check_head.py`, `compare_plans.py`, `key_map.py` | 선언한 비교 |
| `facts_allow.tsv`, `extract_txt_allow.tsv`, `relations_allow.tsv`, `order_allow.tsv` | 선언한 차이의 갈래(빌드 전 커밋) |
| `relink_expected.sha256`, `not_relink.sha256` | 빌드 전 sha256: 다시 링크되는 것 14, 다시 링크되지 않아야 하는 것 110 |
| `golden_scratch.sha256`, `df_scratch_*.log` | scratchpad 점검 빌드의 golden sha256 과 공간 감시(근거 아님) |
| `tool_copy/` | 덮기 전 사본의 왕복 확인과 공간 감시 |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | 빌드 전 등록부와 누적 목록 |

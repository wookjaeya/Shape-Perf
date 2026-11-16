# 스키마 개정 12: 새 섹션만, 뒤 작업의 공개 구조를 한 번에 (2026-11-16, 모든 도구 목표별 빌드)

결과 문서: `../../plan/full_stage_results_20261116.md` §4. 텍스트 기록만 둔다. cFS 는 부팅하지 않았다. push 하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §4.0 과 같은 글). 그 sha256 이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `readback.sh`, `readback_pre.txt` | 보존 컨테이너 열의 읽기(파일·`--check`·`--hash`·dump 텍스트의 sha256); 빌드 전 kz-dump 의 결과 |
| `layout_check.py` | 옛·새 `kz-dump --layout`: 옛 줄이 순서대로 남고 더한 줄뿐인지 |
| `identity_all.sh` | 한 트리의 도구 넷이 같은 개정과 fingerprint 를 적는지 |
| `convert_c1boot.sh` | 관측 주기 C1 부팅 dump 의 재변환과 보존된 변환의 비교 |
| `check_extract.py`, `extract_allow.tsv` | 다시 뽑기의 레코드 단위 비교(허용 줄만), 파일 단위 허용 |
| `compare.sh`, `compare_plans.py`, `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh` | 다섯 빌드의 relations, order, 다시 뽑기, 계획과 그 비교 |
| `relink_expected.sha256`, `not_relink.sha256` | 빌드 전 sha256: 다시 링크되는 것 75, 다시 링크되지 않아야 하는 것 29 |
| `tool_copy/` | 덮기 전 사본의 왕복 확인과 공간 감시 |

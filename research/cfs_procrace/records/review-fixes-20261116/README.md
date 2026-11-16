# 검토 뒤 고침 (2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/full_stage_results_20261116.md` §6. 텍스트 기록만 둔다. cFS 는 부팅하지 않았다. push 하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §6.0 (가), (가-1), (나)와 같은 글). 그 sha256 이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `keys_check/` | relations 개정 7 의 다시 뽑기의 키 목록 점검(보존 사본 038cd9e3 을 풀어 돌림): 스크립트, 결과, 공간 감시 |
| `revfix_relrev7_extract_keys.sha256` | 그 점검의 출력 디렉터리 sha256 목록(장부 행) |
| `extract_compare/` | 추출 고침 단계의 다시 뽑기 비교를 다시 돌림: `compare_classes.py` 개정 2, 허용 파일, `compare_extract.sh`, 결과(`compare_extract.txt`, `compare/`), 덧붙임 전 첫 실행(`first/`) |
| `counts_before_build.log` | 빌드 전 MC 의 `test_static_reasons` 확인 수(추출 고침 단계의 바이너리) |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh` | 다섯 빌드의 relations, order(와 다시 뽑은 facts 위의 키), 다시 뽑기, 계획 |
| `compare.sh`, `compare_relations.py`, `compare_order.py`, `compare_plans.py` | 선언한 비교 |
| `creator_success.py` | 설명 스크립트의 함수로 읽은 생성자 성공 상수(재측정) |
| `relink_expected.sha256`, `not_relink.sha256` | 빌드 전 sha256: 다시 링크되는 것 13, 다시 링크되지 않아야 하는 것 111 |
| `tool_copy/` | 덮기 전 사본의 왕복 확인과 공간 감시 |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | 빌드 전 등록부와 누적 목록 |

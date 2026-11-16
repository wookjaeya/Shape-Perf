# 순서 간선 `kz-static order` (order rules revision 1, 2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/full_stage_results_20261116.md` §3. 텍스트 기록만 둔다. cFS 는 부팅하지 않았다. push 하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §3.0 과 같은 글). 그 sha256 이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `compare.sh`, `compare_plans.py` | 빌드 전에 커밋한 비교: relations 바이트 동일, order 두 번 실행, 다시 뽑은 facts 의 order, 계획의 선언한 줄 |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `order_extract_allow.tsv` | relations 다섯 빌드, order 다섯 빌드, 다섯 빌드 다시 뽑기(단일 빌드 실행기), 계획 다섯 빌드, 다시 뽑기의 허용 파일 |

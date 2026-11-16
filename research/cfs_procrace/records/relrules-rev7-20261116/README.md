# relations 규칙 개정 7: 객체 정체성과 writer 의 전제 (2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/full_stage_results_20261116.md` §2. 텍스트 기록만 둔다. cFS 는 부팅하지 않았다. push 하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §2.0 과 같은 글). 그 sha256 이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `compare_declared.py`, `callback_check.py`, `plan_diff.py` | 빌드 전에 커밋한 비교 스크립트: 선언한 갈래의 비교(사용 지점 키로 맞춤, golden 모드), 빠진 짝마다 소스의 callback 인자, 계획의 차이 |
| `rerun.sh`, `extract.sh`, `plans.sh`, `rev7_extract_allow.tsv` | relations 다섯 빌드, 다섯 빌드 다시 뽑기(단일 빌드 실행기), 계획 다섯 빌드, 다시 뽑기의 허용 파일 |
| `tool_copy/` | 이 빌드가 덮는 kz-static `de1315b3` 의 기존 압축 사본의 왕복 확인 |
| `pre_mc.txt`, `post_mc.txt`, `make.log`, `df_build.log`, `build_start.txt` | 병합 확인 트리(MC)의 빌드 전·후, 빌드 출력, 공간 감시 |
| `ctest_first.log`, `golden_update.log`, `golden_compare.txt`, `ctest_full.log`, `skip_reason.log`, `unit_tools.txt`, `unit_step2a.txt` | 시험 |
| `byte_identity.txt`, `compare/`, `outputs.sha256`, `df_rerun.log`, `c1_rows.txt`, `creator_success.txt` | relations 다섯 빌드의 선언 비교, 키마다 전후 표, 소스 대조, C1 의 두 행 |
| `extract_run.txt`, `extract_compare.txt`, `extract.sha256`, `df_extract.log` | 다섯 빌드 다시 뽑기 |
| `plans_run.txt`, `parse_check.txt`, `regen_*.txt`, `plans/`, `plans.sha256`, `df_plans.log` | 계획 다섯 빌드, 파서 점검, 재생성 점검, 앞 계획과의 차이 |
| `tools_check.txt`, `ledger_check.txt` | 도구 등록부와 누적 목록의 확인 |

# relations 행의 기록, 사용 지점 키, 공유 적재기 (2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/observation_path_results_20261116.md` §2. 텍스트 기록만 둔다. cFS는 부팅하지 않았다. push하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §2.0과 같은 글). 그 sha256이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `pre_mc.txt`, `post_mc.txt` | 병합 확인 트리(MC)의 빌드 전·후(`env/tools/build_record.sh`) |
| `make.log`, `df_build.log` | 빌드 출력과 공간 감시(kind kz-static) |
| `ctest_full.log`, `skip_reason.log` | 전체 ctest와 건너뛴 시험 하나의 까닭 |
| `unit_tools.txt`, `unit_step2a.txt` | 작업 스크립트 단위 점검 8/8, 2a 드라이버 단위 점검 7/7 |
| `rerun.sh`, `df_rerun.log` | relations 다섯 빌드 재실행 스크립트와 공간 감시(kind relations) |
| `byte_identity.txt` | 재실행의 `relations.txt`·`relations.log`를 T8CF와 `cmp` (20 of 20 same) |
| `keys.sha256` | 남긴 사용 지점 키 목록 다섯(`/home/user/work/redesign/obs1/relrows_check/<빌드>/keys.txt`)의 sha256 |
| `tools_check.txt`, `ledger_check.txt` | 도구 등록부(`../TOOLS.tsv`)와 누적 목록(`../LEDGER.tsv`)의 빌드 뒤 확인 |

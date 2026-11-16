# relations 규칙 개정 6: 잘린 정적 이미지, 행마다의 미완 사유 (2026-11-16, kz-static 빌드 하나)

결과 문서: `../../plan/observation_path_results_20261116.md` §3. 텍스트 기록만 둔다. cFS는 부팅하지 않았다. push하지 않았다.

| 파일 | 무엇 |
|---|---|
| `declared_before_build.txt` | 빌드 전 선언(결과 문서 §3.0과 같은 글). 그 sha256이 `commands.txt` 첫 줄이다 |
| `commands.txt` | 명령, rc, 여유 공간 |
| `compare_declared.py`, `vocab_check.py` | 빌드 전에 커밋한 비교 스크립트(선언한 세 변화를 되돌린 바이트 비교; 어휘 자료 = 판정 규칙 3.5 표) |
| `tool_copy/` | 이 빌드가 덮은 kz-static `a763e255` 의 압축 사본 기록(목록, 왕복, 공간 감시). 사본은 `/home/user/work/preserve/tools-20261116-b/` |
| `pre_mc.txt`, `post_mc.txt` | 병합 확인 트리(MC)의 빌드 전·후 |
| `make.log`, `df_build.log`, `build_start.txt` | 빌드 출력, 공간 감시(kind kz-static), `find -newer` 의 기준 시각 |
| `ctest_first.log`, `golden_update.log`, `golden_compare.txt` | golden 갱신 전 ctest(선언한 실패 일곱), 갱신, 바뀐 golden 넷의 선언 비교 |
| `ctest_full.log`, `skip_reason.log` | 갱신 뒤 전체 ctest와 건너뛴 시험 하나의 까닭 |
| `unit_tools.txt`, `unit_step2a.txt` | 작업 스크립트 단위 점검 8/8, 2a 드라이버 단위 점검 7/7 |
| `rerun.sh`, `df_rerun.log`, `byte_identity.txt`, `outputs.sha256` | relations 다섯 빌드 재실행, 공간 감시, 선언 비교, 남긴 출력의 sha256 |
| `sens_run.txt`, `sens_status.txt`, `sens_args.txt`, `df_sens.log`, `sens_outputs.sha256`, `sens_detail.txt` | c1_pre_tx `--image-budget 3` 민감도 실행 |
| `tools_check.txt`, `ledger_check.txt` | 도구 등록부와 누적 목록의 확인 |

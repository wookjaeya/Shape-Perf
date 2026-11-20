# closure-explain-rev4-20261119 — 설명 스크립트 개정 4 와 그 요약, 동결 (도구 빌드 없음, 부팅 없음)

결과는 `plan/full_stage_results_20261116.md` 24 절. 명세는 `plan/c1_closure_explain_rev4_spec_20261118.md`(10절과 10.7 은 이 단계가 코드보다 먼저 커밋함).

- `freeze.tsv`: 동결한 두 부분(읽는 부분, 판정 부분)과 구동부·갈래 파일 생성·요약·기대 대조·fixture·개정 3 사본의 sha256. 부팅 선언이 이 파일을 인용한다. `out/freeze_check_2.txt` 가 저장소의 파일과 대조한 것(8 OK). `out/freeze_check.txt` 는 첫 줄을 빠뜨린 첫 점검(7 줄)이고 그대로 남김.
- `scripts/`: `regress.sh`(2026-11-16 입력에서 개정 3 또는 4), `dryrun.sh`(예행 입력, 두 C1 빌드의 정적 출력, 런타임 단계의 시험 dump), `compare_r3_r4.py`, `static_rows.py`.
- `out/`: 갈래 파일 `branches_d1.txt`(규칙으로 만듦, 후보 목록 없음 → D1 여덟), fixture 기록 `fixtures.txt`, 단위 점검(`unit_tools.txt` 첫 실행 18/19 — 열기 시험의 경로 꼴 결함, `unit_tools_2.txt`·`unit_tools_3.txt` 19/19, `unit_step2a*.txt` 7/7), 회귀(`regress_rev3.log`, `regress_rev4*.log`, `compare_r3_r4*_<갈래>.txt`, `rev2_vs_rev3_index.txt`), 예행(`dry_*`), 요약(`summary_*`), 원장·등록부 점검.
- `guard/`: 감시 기록 14(모두 completed).
- `<출력 디렉터리>.sha256`: `/home/user/work/redesign/obs1/xr4_*` 열둘의 파일 목록(원장 `OBS_xr4_*`). `_2` 가 붙지 않은 regress_rev4·dry_static·dry_rt 는 동결 전 코드(cb9a4d4)의 첫 실행이고, `_2` 가 동결한 코드(e4b2b98)의 실행이다.

# 날짜 붙인 주의 수 (2026-11-19)

결과 문서 `plan/full_stage_results_20261116.md` §26.1 의 날짜 붙인 주(닫기 검토 `../closure-review-20261119/` 의 지적 가운데 §26.0 (하)·(거)가 이름 댄 것과, 같은 날 더한 몇 개)가 적는 수를 남긴 파일에서 다시 읽은 기록이다. 부팅·빌드·추출은 없고, 도구·자료·계획·동결은 건드리지 않았다.

## 파일

| 파일 | 내용 |
|---|---|
| `check.sh` | 읽기만 하는 점검(bash). 행 번호·키·판정은 입력이 아니고, 파일이 가진 것을 찍는다. A 기록 디렉터리마다 감시 기록의 수, 끝 줄, `completed`, 가장 낮은 값, 끝 값이 하한 아래인 기록; B 감시 아래 일의 시간 겹침; C 고침 빌드(`../review-fixes-2-mc-20261119/`)의 측정 파일 시각과 비교 줄, 커밋 시각; D 주기 준비(`../closure-c1prep-20261119/`)의 원장·등록부 점검 시각과 끝 줄; E 시험 `tool_identity_schema_revision` 이 든 목록과 기록; F 다섯 빌드의 alternatives `none found` 줄과 그 행의 효과·`layers`; G `clear-slot` 줄의 수와 C1 두 빌드의 자리; H HB-await 간선(쓰기, writer, writer × 대기 task 짝); I framework 의미 자료 일곱 판의 끝줄, 서로 다른 `api` 이름, c1 의 일치 칸; J P0 선언 자료의 `strlen` 과 `strcmp` |
| `check.txt` | `bash check.sh > check.txt` 의 출력(rc 0, HEAD `3535b43`) |
| `guard_check.log` | 그 실행의 공간 감시 기록(`env/tools/space_guard.sh`, 필요량 10,000,000) |
| `lint.txt` | 주를 단 뒤의 `kairos/` 트리에 MC 의 `kz_lint`(`3d4d5a8f…`)의 규칙 일곱(`case-ids`, `case-ids-data`, `semantics-names`, `retired-term`, `banned-files`, `no-llvm`, `no-generated`)을 돌린 출력: 모두 clean, rc 0 |
| `guard_lint.log` | 그 실행의 공간 감시 기록 |

## 읽는 데 주의할 것

- 출력의 짝·줄 목록은 바탕이다. 그 가운데 무엇이 소스로 성립하는지(HB-await 의 생성자 → 만든 task 짝 15, clear-slot 줄 가운데 이름을 지우는 2)는 소스를 읽은 것이고, 결과 문서의 주가 그 소스 줄을 적는다.
- 같은 스크립트의 앞판을 저장소 밖에서 두 번 시험으로 돌려 출력의 꼴(감시 기록의 끝 줄 수, `layers` 를 행 끝에서 찍기, 머리 줄)을 고친 뒤 이 출력을 냈다. 시험 출력은 근거가 아니다.

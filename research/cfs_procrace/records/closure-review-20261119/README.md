# C1 주기 준비 뒤의 세 검토와 그 검증 (2026-11-19)

C1 자동 주기 하나의 준비(결과 문서 `plan/full_stage_results_20261116.md` 25.8–25.13; 부팅 선언 `860c2cd`, 그 단계의 끝 `4c2fefe`)를 마친 뒤, 첫 부팅 전에 한 검토 셋과 그 발견마다의 검증이다. 부팅은 하지 않았다.

- 검토 셋: 규칙(`rules-NN`: 판정 규칙·평가 절차와 동결한 설명 스크립트가 서로 맞는지), 빌드(`builds-NN`: 선언한 빌드와 점검이 선언대로 되었는지), 의미(`semantics-NN`: 계획·판정이 소스의 뜻과 맞는지).
- 검증: 발견마다 저장소의 파일, 남긴 출력과 동결한 스크립트로 다시 확인해 진짜인지(`real`)와 첫 부팅 전에 다루어야 하는지(`before_boot`)를 적었다.
- 결과 문서 26절은 이 목록의 id 로 인용한다. 그 판단은 26.0(lead 의 판단)이고, 이 목록의 `fix` 칸은 검토와 검증이 낸 제안일 뿐 결정이 아니다.

## 파일

| 파일 | 내용 |
|---|---|
| `findings.json` | `findings` 22(id, severity, claim, evidence, fix), `verdicts` 22(id, real, before_boot, reason, fix), `prep_open_points` 9(주기 준비 단계가 남긴 열린 점, 25.13), `xp_open_points` 7(설명 스크립트 개정 4 단계가 남긴 열린 점, 24절). 글은 영어이고 검토·검증이 쓴 그대로다. 저장소 밖 임시 출력의 경로 한 곳(`semantics-01` 의 증거)만 `<scratch>` 로 바꾸었다. 그 밖의 경로는 저장소 안이거나 남긴 출력(`/home/user/work/redesign/obs1/…`)이다 |
| `count.py` | 아래 수를 `findings.json` 에서 센다: `python3 -I count.py findings.json > counts.txt` |
| `counts.txt` | 그 출력(rc 0) |

## 수 (`counts.txt`)

- 발견 22: 규칙 8(blocking 2, major 4, minor 2), 빌드 7(blocking 1, major 4, minor 2), 의미 7(blocking 1, major 5, minor 1). 모두 blocking 4, major 13, minor 5. 심각도는 검토가 붙인 것이다.
- 검증 22: `real` 20, 아님 2(`rules-07`, `rules-08`: 검증이 그 내용이 결과 문서에 이미 적혀 다루어지고 있다고 본 것 — `rules-07` 은 21.5·21.8, `rules-08` 은 24.4 (라)·25.0 (a)·25.13; `before_boot` 칸 없음).
- `real` 20 가운데 `before_boot` 5(`rules-01`, `rules-02`, `rules-04`, `builds-04`, `semantics-01`), 15 는 첫 부팅 전이 아니어도 됨.
- blocking 넷 가운데 `builds-01` 만 `before_boot` 가 아니다(빌드 뒤 점검 둘을 건너뛴 기록의 문제이고, 뒤의 깊은 점검과 원장 점검은 지났다).

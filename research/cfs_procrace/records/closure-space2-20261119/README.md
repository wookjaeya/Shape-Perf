# closure-space2-20261119 — 부팅을 위한 공간 2: 26.3 의 남긴 출력(`obs1/xr5_*`)을 xz tar 로 (cFS 부팅 없음)

결과는 `plan/full_stage_results_20261116.md` 26.8 절. 근거: lead 의 판단 26.7 (나), 평가 절차 14.9, 부팅 선언 3(`../closure-c1prep-20261119/boot/declaration3.tsv`)의 머리. 이 디렉터리의 목록·필요량·차례·스크립트·장부 규약은 **어느 압축보다 먼저** 커밋했다. 방법은 26.2(`../closure-space-20261119/`)의 압축과 같다.

## 파일

| 파일 | 무엇 |
|---|---|
| `free_start.txt` | 시작의 `df -B1 /` |
| `listing.py`, `listing.tsv` | 목록: 26.3–26.5 가 `/home/user/work/redesign/obs1` 에 남긴 디렉터리(26.2 를 닫은 커밋 `c7b9d90` 뒤에 더한 장부 행의 root, 그리고 26.2 끝 13:57Z 뒤에 바뀐 디렉터리; 두 길이 같은 열둘을 냄). 디렉터리마다 장부 행과 목록, 디스크의 파일 수, 목록이 디스크와 같은지, sha256 이 다른 파일 수, 링크와 다른 형의 항목, 겉 크기와 블록, 부팅 단계·동결 파일 가운데 이름 대는 것, 다른 장부 행, 이름 대는 커밋된 스크립트와 그 단계, 바꿀 수 있는지와 까닭 |
| `need.py`, `need.txt`, `chosen.tsv` | 필요량(부팅 선언 3 의 머리에서 읽음; 부분의 합을 점검함), 지금 여유, 차례(`need.py` 의 `ORDER`, 까닭과 함께), 줄마다 기대 여유 [I] |
| `archive.sh`, `selftest.sh`, `selftest.txt` | 압축: `../closure-space-20261119/archive.sh` 를 옮기고 머리글과 층 이름(`A`)만 바꿈. 다섯 단계는 같다. 자체 시험 다섯 모두 통과 |
| `ledger_rewrite.py` | 장부 한 행 고치기(아래 규약). `../closure-space-20261119/ledger_rewrite.py` 에서 회수 갈래를 빼고 기록 디렉터리·절·머리 줄만 바꿈 |
| `run_seq.sh` | `chosen.tsv` 의 줄을 하나씩: 줄 앞에서 여유가 필요량(`need.txt` 의 `need_free`)에 닿았으면 멈춤; 감시 아래 압축 → 제안 장부 → 제안 장부의 `ledger_check.sh` → 장부 교체 → `progress.tsv` 한 줄. `../closure-space-20261119/run_seq.sh` 에서 머리글, 층 이름, 필요량 줄 이름만 바꿈 |
| `guard/` | 출력을 쓰는 모든 실행의 `space_guard.sh` 기록 |
| `archive/`, `ledger/`, `progress.tsv` | 압축마다의 왕복 기록(`roundtrip_<이름>.txt`), 장부 고침과 점검(`rewrite.txt`, 줄마다 `check_<줄>_<이름>.txt`, 시작과 끝의 점검), 진행(줄마다 앞뒤 여유, 압축 크기, 감시의 가장 낮은 값) |
| `commands.txt` | 돌린 차례 |

## 고르는 규칙 (`listing.py` 의 머리글)

바꿀 수 있는 디렉터리는 다음을 모두 지킨다: 그 root 를 가진 장부 행이 정확히 하나이고 `sha256c`·local `yes` 다; 그 목록이 디스크의 정규 파일을 빠짐없이, 더함 없이 이름 대고 sha256 이 모두 같다; 심볼릭 링크·하드 링크·다른 형의 항목이 없다; 부팅 단계의 파일(`boot/run_cycle.sh`, `boot/commands.txt`, 가장 새 부팅 선언 `boot/declaration3.tsv`, `boot/test_run_cycle.sh`)과 동결 파일 셋이 `xr5_` 도 그 이름도 적지 않는다; 다른 장부 행의 list·root 칸이 그것을 이름 대지 않는다.

부팅 단계가 obs1 에서 읽는 것은 그 파일들이 적는 경로뿐이다: `cz_plans_v4_ns2`, `c1p2_*`, `cz_final_extract`, `cz_final_order`, `xr4_dry_inputs`(그리고 `c1cyc_*` 에 씀). `xr5_*` 를 이름 대는 커밋된 스크립트는 둘이고 끝난 단계의 것이다(26.3 의 `../closure-explain-fix-20261119/scripts/runs.sh`, 26.5 의 `../closure-prebootfix-20261119/scripts/runs.sh`). 26.4 의 가름 비교는 그 경로를 명령줄로 받았다.

## 장부 규약

- 압축: 행의 method `parts`, root `/home/user/work/preserve/archives-20261119/<이름>`(`PARTS.sha256` 이 압축 파일의 sha256, `MEMBERS.sha256` 이 목록의 사본), list 칸은 그대로(디렉터리의 목록; 지우기 전에 압축의 흐름과 파일마다 대조함), local `replaced 2026-11-19 by an xz tar archive (<압축 파일>)`, verified_copy 는 이 디렉터리의 왕복 기록, note 에 한 구절을 더함. 장부 머리에 이 단계의 범위를 적는 줄 하나를 첫 고침 때 더한다(26.2 의 줄 바로 아래). 장부 머리의 압축 규약(첫 쓰임 `f9ce046`)과 같다.
- 고칠 때마다 제안 장부를 `ledger_check.sh` 로 먼저 점검하고(rc 0) 그다음에만 장부를 바꾼다.

## 잃는 것

압축은 바이트·mode·소유자·시각을 tar 안에 보존한다. 지운 경로를 그대로 읽는 앞 단계의 스크립트(위의 둘)는 그대로는 다시 돌지 않는다. 26.7 (라)의 검토가 고칠 것을 찾아 회귀·예행·합성 맞댐을 다시 하면, 견줄 디렉터리를 압축에서 scratch 로 풀고 `MEMBERS.sha256` 로 점검한 뒤 읽는다.

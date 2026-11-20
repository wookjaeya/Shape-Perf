# closure-space3-20261119 — 부팅을 위한 공간 3: 다시 적은 부팅마다의 필요와 26.8 목록의 9–12 줄 (cFS 부팅 없음)

결과는 `plan/full_stage_results_20261116.md` 26.9 절. 근거: 26.5 고침의 따로 한 검토(`../closure-review-bootfix-20261119/findings.tsv` 의 `fix-judge-02`, `fix-decl-01`, `fix-decl-03`), 평가 절차 14.14 의 4, lead 의 판단 26.7 (나). 이 디렉터리의 필요·목록·스크립트는 **어느 압축보다 먼저** 커밋했다. 방법은 26.2(`../closure-space-20261119/`)와 26.8(`../closure-space2-20261119/`)의 압축과 같다.

## 파일

| 파일 | 무엇 |
|---|---|
| `free_start.txt` | 시작의 `df -B1 /` |
| `need3.py`, `need.txt` | 부팅 하나의 필요를 남긴 파일에서 읽어 다시 적음(부분마다 읽은 파일을 그 줄에 적음); `c1_pre_tx` 첫 부팅 앞(12 부팅)과 `c1_post_tx` 첫 부팅 앞(6 부팅)의 필요; 지금 여유와 모자람 |
| `chosen.tsv` | 26.8 의 `chosen.tsv` 9–12 줄을 그대로 옮김(머리 한 줄을 더함). 이 단계는 넷을 모두 씀 |
| `archive.sh`, `selftest.sh`, `selftest.txt` | 26.8 의 사본, 머리글만 바꿈. 자체 시험 다섯 모두 통과 |
| `run_seq.sh` | 26.8 의 사본, 머리글과 줄 앞의 멈춤 한 줄을 뺌(넷을 모두 쓰고 끝에 필요와 견줌) |
| `ledger_rewrite.py` | 26.8 의 사본, 기록 디렉터리·절·장부 머리 줄(넷째 쓰임)만 바꿈 |
| `guard/`, `archive/`, `ledger/`, `progress.tsv` | 감시 기록, 왕복 기록, 장부 고침과 점검, 진행 |
| `free_end.txt` | 끝의 `df -B1 /` |
| `commands.txt` | 돌린 차례 |

## 장부 규약

26.8 과 같다: 압축마다 행을 `parts` 로 고치고(root 는 `/home/user/work/preserve/archives-20261119/<이름>`), 제안 장부를 `ledger_check.sh` 로 먼저 점검한 뒤에만 장부를 바꾼다. 장부 머리에 이 단계의 줄 하나를 첫 고침 때 셋째 쓰임 줄 아래에 더한다.

## 잃는 것

압축은 바이트·mode·소유자·시각을 보존한다. 지운 경로를 그대로 읽는 앞 단계의 스크립트(26.3·26.5 의 `runs.sh`)는 그대로는 다시 돌지 않는다. 판정 부분이 다시 바뀌어 회귀·예행을 다시 하면 압축에서 scratch 로 풀어 `MEMBERS.sha256` 로 점검한 뒤 읽는다.

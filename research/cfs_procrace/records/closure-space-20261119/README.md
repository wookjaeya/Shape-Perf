# closure-space-20261119 — 부팅을 위한 공간: 바이트를 보존하는 압축과 바이트가 같은 디렉터리의 회수 (cFS 부팅 없음)

결과는 `plan/full_stage_results_20261116.md` 26.2 절. 근거: lead 의 판단 26.0 (카), 평가 절차 14.9. 이 디렉터리의 목록·필요량·고른 디렉터리·스크립트·장부 규약은 **어느 압축이나 지우기보다 먼저** 커밋했다.

## 파일

| 파일 | 무엇 |
|---|---|
| `free_start.txt` | 시작의 `df -B1 /` |
| `listing.py`, `listing.sh`, `readers_extra.txt`, `listing.tsv` | 목록: `/home/user/work/redesign/obs1` 아래 디렉터리 모두(134)와 `/home/user/work` 의 디렉터리(34; 소스 트리·도구 사슬·`kairos-build`·`preserve`·`llvm-project` 는 크기 없이 '제외'로만). 디렉터리마다 크기(파일 크기의 합, 그리고 df 가 세는 블록), 파일 수, 링크, 장부 행, 장부 목록이 디스크의 파일과 같은지, 누가 아직 읽는지, 바꿀 수 있는지와 그 까닭 |
| `need.py`, `need.txt` | 필요량: 항목마다 남긴 파일(감시 기록, 디렉터리 크기)에서 읽음 |
| `chosen.tsv` | 이 단계가 건드릴 수 있는 디렉터리와 그 차례(R → 1 → 2), 줄마다 기대 여유 |
| `archive.sh`, `selftest.sh`, `selftest.txt`, `selftest_2.txt` | 압축: `f9ce046` 의 스크립트(`../closure-c1prep-20261119/reclaim/archive.sh`)와 같은 다섯 단계, 더한 것은 `chosen.tsv` 의 tier 1·2 줄만 받음과 정규 파일·디렉터리 밖의 항목 거부. 자체 시험 넷 모두 통과(`selftest.txt`). 첫 압축 시도가 1 단계에서 멈춘 뒤(목록이 `./` 없이 경로를 적음; 아무것도 지우지 않음) 두 대조가 목록을 `./<경로>` 꼴로 읽게 고치고 시험 하나를 더함: 다섯 모두 통과(`selftest_2.txt`) |
| `ledger_rewrite.py` | 장부 한 행 고치기(아래 규약) |
| `run_seq.sh` | `chosen.tsv` 의 줄을 하나씩: 감시 아래 압축 → 제안 장부 → 제안 장부의 `ledger_check.sh` → 장부 교체 → `progress.tsv` 한 줄. 줄마다 먼저 여유가 목표에 닿았는지 보고 닿았으면 멈춤 |
| `reclaim/` | `rf2_pre_*` 셋: `pairs.tsv`, `../reclaim-20261118/identity_listing.py` 의 출력(`identity_summary_before.txt`, `listing_before_<디렉터리>.txt`), 그리고 뒤의 지우기 기록 |
| `guard/` | 출력을 쓰는 모든 실행의 `space_guard.sh` 기록 |
| `archive/`, `ledger/`, `progress.tsv` | 압축마다의 왕복 기록(`roundtrip_<이름>.txt`; 첫 시도의 실패 기록은 `.trial1_fail` 이름), 장부 고침과 점검(`rewrite.txt`, 압축마다 `check_<줄>_<이름>.txt`, 시작·회수·끝의 점검), 진행(줄마다 앞뒤 여유, 압축 크기, 감시의 가장 낮은 값) |
| `free_end.txt`, `tools_check_deep_end.txt` | 끝의 `df -B1 /`, 등록부 `--deep` 점검 |
| `scripts_naming_removed.txt` | 압축하거나 지운 31 디렉터리를 경로로 이름 대는 커밋된 스크립트(앞 단계의 것; 그대로는 다시 돌지 않음) |
| `commands.txt` | 돌린 차례 |

## 고르는 규칙 (`listing.py` 의 머리글)

바꿀 수 있는 디렉터리는 다음을 모두 지킨다: 추출 회차 또는 order·relations 회차다(점검 출력 `_check`·`_keys` 는 회차가 아님); 아직 돌 단계의 어느 파일도 그 이름을 대지 않는다(부팅: `run_cycle.sh`, `commands.txt`, `declaration.tsv`; 코드와 동결: `regress.sh`, `dryrun.sh`, 2026-11-16 회귀의 `inputs.txt`, 명령줄로만 주는 입력 `readers_extra.txt`); 동결한 판의 출력이라 고친 판과 견줄 기준(`../closure-explain-rev4-20261119/<이름>.sha256` 의 `xr4_*`)이 아니다; 회수 규약의 기준 디렉터리(장부에서 `removed` 행의 root, 그리고 이 단계의 회수가 가리킬 `ll_final_order`)가 아니다; 지금 회차 `cz_*`, 주기의 파생 빌드 `c1p2_*`, 2026-11-16 부팅과 그 설명 `c1pre_*` 가 아니다; 그 root 를 가진 장부 행이 정확히 하나이고 `sha256c`·local `yes` 이며 그 목록이 디스크의 정규 파일을 빠짐없이, 더함 없이 이름 댄다; 심볼릭 링크·하드 링크·다른 형의 항목이 없다.

`/home/user/work` 의 다른 디렉터리는 obs1 의 회차 차례 밖이고 이 규칙으로 고르지 않는다. 예: `redesign/step2a` 의 장부 행(T, T_RV4, T_8B, T_8BF, T_8C, T8CF)은 목록이 디렉터리의 파일을 다 이름 대지 않는다(`tests_8b` 파일 150, 목록 137 줄). `f9ce046` 의 방법은 '디스크의 파일 = 목록'을 요구한다.

## 장부 규약

- 압축(tier 1·2): 행의 method `parts`, root `/home/user/work/preserve/archives-20261119/<이름>`(`PARTS.sha256` 이 압축 파일의 sha256, `MEMBERS.sha256` 이 목록의 사본), list 칸은 그대로(디렉터리의 목록; 지우기 전에 압축의 흐름과 파일마다 대조함), local `replaced 2026-11-19 by an xz tar archive (<압축 파일>)`, verified_copy 는 왕복 기록, note 에 한 구절을 더함. 장부 머리에 이 단계의 범위를 적는 줄 하나를 첫 고침 때 더한다(`f9ce046` 의 줄 바로 아래). 이 규약은 `f9ce046` 이 장부 머리에 적은 것과 같다.
- 회수(tier R): `reclaim-20261118` 의 규약. 행과 목록은 그대로, root 는 바이트가 같은 남긴 디렉터리, local `removed 2026-11-19; bytes in <그 행>`, note 는 지운 디렉터리·파일 수·바이트·기준 행·목록 파일, 그다음 옛 note.
- 고칠 때마다 제안 장부를 `ledger_check.sh` 로 먼저 점검하고(rc 0) 그다음에만 장부를 바꾼다.

## 잃는 것

압축은 바이트·mode·소유자·시각을 tar 안에 보존한다. 회수는 지운 파일의 수정 시각과 mode 를 잃는다(목록의 끝 두 칸에 적어 둠). 지운 경로를 그대로 읽는 앞 단계의 스크립트(이 단계가 아직 돌 단계로 보지 않는 것)는 그대로는 다시 돌지 않는다. 바이트는 압축이나 남긴 기준 디렉터리에 있다.

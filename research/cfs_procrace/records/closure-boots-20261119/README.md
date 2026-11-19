# closure-boots-20261119 — C1 주기의 부팅: 단계마다의 기록과 부팅마다 뽑은 수

결과는 `plan/full_stage_results_20261116.md` 27절(`c1_pre_tx` 는 27.1). 근거: lead 의 판단 26.7 (라)·(마), 평가 절차 2·3절과 14.14, 부팅 선언 4(`../closure-c1prep-20261119/boot/declaration4.tsv`, `b268e2d2…`), 명령 `../closure-c1prep-20261119/boot/commands.txt`(`365ce243…`), 실행 스크립트 `../closure-c1prep-20261119/boot/run_cycle.sh`(`2508f144…`). 이 README 와 27.1 의 차례는 **첫 부팅보다 먼저** 커밋했고, 아래 두 스크립트는 그 뒤, 첫 부팅 전에 커밋했다.

## 무엇이 어디에 있는가

- **실행 스크립트가 쓰는 기록**은 선언된 자리 `../closure-c1prep-20261119/boot/<빌드>/boot<N>/` 에 있다(`verify.txt`, `verify_at_boot.txt`, `space_at_boot.txt`, `boot.txt`, `control_check.txt`, `convert.log`, `convert.sha256`, `chain.txt`, `stop_eval.tsv`, `gate.txt`, `verify_at_explain.txt`, `explain.txt`, `eval_run.txt`, `eval_tail.txt`, `expect_compare.txt`, `keep.txt`, `ledger_check_keep.txt`, `c1cyc_<s>_<무엇><N>.sha256`, 감시 기록 `guard_*.log`). 실행 스크립트는 바꿀 수 없으므로(부팅 단계가 커밋된 그대로를 요구함) 그 자리를 옮기지 않는다. 그 디렉터리를 부팅마다 그대로 커밋한다. 첫 부팅이 쓰는 `boot/declaration_pin.txt` 도 함께 커밋한다.
- **큰 출력**(부팅의 작업 디렉터리와 덤프, 변환, 16 갈래의 설명 기록, 평가 출력 전문 `eval.txt`)은 `/home/user/work/redesign/obs1/c1cyc_<s>_{boot,convert,explain,eval}<N>/` 에 있고, `keep` 단계가 sha256 목록과 장부 행(`OBS_c1cyc_<s>_<무엇><N>`)을 단다.
- **이 디렉터리**는 이 단계가 쓴 것이다: 단계마다의 시각·rc·여유(`<빌드>/steps.tsv`, 단계의 출력 `<빌드>/boot-<N>/<단계>.out`), 부팅마다 뽑은 수(`<빌드>/boot-<N>/`; 아래), 시작과 끝의 여유와 장부 점검.

## 스크립트 (첫 부팅 전에 커밋함)

| 파일 | 무엇 |
|---|---|
| `scripts/step.sh 빌드 N 단계` | `boot/commands.txt` 의 한 줄을 저장소 뿌리에서 그대로 돌린다(`summary` 는 N 을 `-` 로 주고 N 없이 부른다). 출력은 `<빌드>/boot-<N>/<단계>.out`(있으면 앞의 것을 UTC 나노초 이름으로 옮김), 한 줄을 `<빌드>/steps.tsv` 에 더한다(시작·끝 UTC, rc, 앞뒤 `df -B1 /`). 시험 이음매(`RUN_CYCLE_*`)가 설정되어 있으면 돌지 않는다. 단계에 아무것도 더하지 않는다 |
| `scripts/extract.sh 빌드 N` | `keep` 뒤에 실행 스크립트의 기록 디렉터리와 작업 디렉터리를 읽기만 하여 `<빌드>/boot-<N>/` 에 쓴다: `index.txt`(verify 줄, 공간 판정, 부팅 명령의 끝 줄과 rc, 제어 점검의 요약·손실 줄, 연결 사슬 줄, 문 줄, 설명의 갈래마다 수(`judge.txt`), 평가 머리의 주 갈래 줄, 기대 대조의 합, `keep` 줄, `note` 가 정확히 600 글자인 F3 줄의 수(26.9 `fix-judge-03`), 모든 `verify*.txt` 와 고정 파일의 선언 sha256(26.9 `fix-decl-09`), `run.meta` 의 `instance_files` 줄과 `.other` 의 줄 수), `explain_judge.txt`·`explain_inputs.txt`·`explain_input_checks.txt`(설명 출력의 사본), `explain_index_D1-R0.tsv`(주 갈래의 `index.tsv` 사본), `explain_counts.tsv`(갈래마다 `index.tsv` 의 열 `link`, `use_cond`, `order`, `intervention`, `effect`, `violation`, `odf`, `headline`, `pair_kind` 과 `uses.tsv` 의 열 `status`, `outcome`, `name_state`, `link_result` 의 값마다 줄 수), `expect_counts.tsv`(갈래·종류마다 결과의 줄 수), `large_outputs.tsv`(작업 디렉터리마다 파일 수, 바이트, 블록, `keep` 목록의 sha256, 장부 행), `guard_ends.txt`(그 부팅의 감시 기록마다 끝 줄), `runner_files.sha256`(실행 스크립트의 기록 디렉터리의 모든 파일). 출력이 있으면 돌지 않는다(rc 2). 세기만 하고 읽지 않는다 |

## 지키는 것

- 부팅 수, 문, 다시 한 부팅의 규칙은 선언 4 의 머리와 평가 절차 2·3절, 14.10, 14.14 의 3 이다. 이 디렉터리의 어느 스크립트도 그것을 정하지 않는다: 다음 번호는 실행 스크립트의 `gate.txt` 와 `allowed` 가 정한다.
- 단계마다 rc 를 본 뒤 다음 단계로 간다. 감시는 실행 스크립트가 거는 단계(boot, convert, explain, eval)에 걸리고, 그 기록은 실행 스크립트의 기록 디렉터리에 남는다.
- 첫 부팅 뒤에는 선언, 실행 스크립트, 판정 부분, 기대, 규칙을 바꾸지 않는다(평가 절차 12절). 이 디렉터리의 뽑는 스크립트를 고치면 앞판의 출력을 지우지 않고 새 이름으로 다시 내며 그렇게 적는다.

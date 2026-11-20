# 첫 자동 관측 경로의 바닥 기록 (2026-11-16, 빌드 없음)

결과 문서: `../../plan/observation_path_results_20261116.md` §1. 여기에는 텍스트 기록만 둔다. cFS는 부팅하지 않았다. 아무것도 빌드하지 않았다. push하지 않았다.

| 디렉터리·파일 | 무엇 |
|---|---|
| `scratch_third_copies/` | scratchpad의 셋째 사본 지우기: 목록(`list.tsv`: sha256, 바이트, scratchpad 안 경로, W 아래 원본, 묶음), 원본 목록(`originals.sha256`), 묶음 다섯의 스트림 재해시(`bundle_stream.txt`), 지운 뒤 확인(`delete.log`), 여유(`df.txt`) |
| `scratch_pack/` | 나머지 scratch의 무손실 압축: 항목별 요약(`pack.txt`), 파일마다 sha256(`files.sha256`), 스크립트(`pack_check.sh`), 여유(`df.txt`). 압축본은 scratchpad `pack_20261116/`에 있고 scratch다(근거가 아니다) |
| `pre_mc.txt` | 병합 확인 트리(MC)의 빌드 전 상태(`env/tools/build_record.sh pre`) |
| `runner_check/` | 단일 빌드 실행기(`env/redesign/one_build.sh`)를 c5_641에 지금 도구로 돌린 결과와 T8CF 대조(`compare.tsv`). 출력 자체는 대조 뒤 지웠다(T8CF와 같으므로 다시 만들 수 있다) |
| `chain/` | `env/tools/chain_check.py`를 W6 사슬과 C5 6.4.1 사슬에 돌린 결과 |
| `harness/` | `env/tools/hash_instance.sh`를 W6 G-I 기록의 `inst/`에 읽기 전용으로 돌린 목록 |
| `tool_copies/` | `/home/user/work/preserve/tools-20261116/`의 목록과 왕복 확인 사본 |
| `lists/` | 누적 바이트 동일성 목록의 해시 목록(F1T, C5 641 `b/`, kairos-build 도구, 플러그인 둘) |
| `readings/` | 읽기 과제: 종료 상태(`exit_status.txt`), 부팅 전 두 표(`startup_tables.txt`), C 라이브러리(`c_library.txt`) |

도구 등록부는 `../TOOLS.tsv`(확인 `env/tools/tools_check.sh`), 증거 장부와 누적 목록은 `../LEDGER.tsv`(확인 `env/tools/ledger_check.sh`)다.

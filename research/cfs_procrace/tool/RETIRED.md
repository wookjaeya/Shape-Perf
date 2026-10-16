# 폐기된 도구: CORD (2026-10-12)

CORD는 사용자 결정에 따라 폐기했다. 대상은 Python/JSON 기반 분석기, 기록기 `cord.c`, R2 계측 patch, 벤치마크다.

- 마지막으로 이 도구가 포함된 commit은 `62e05a9`다. 코드가 필요하면 `git show 62e05a9:research/cfs_procrace/tool/cord/<파일>`로 꺼낸다.
- CORD로 얻은 관측 기록은 근거로 그대로 둔다: `../results/R2_cord_v701.md`, `../env/logs/r2_c*`. 다만 새 탐지기의 결과로 세지 않는다.
- 새 탐지기는 `../Architecture/`의 아키텍처 문서를 기준으로 설계한다. CORD 코드, 기록 형식, 분석기는 계승하지 않는다.
- `gdb/`는 남긴다. R2 보고자가 쓴 관측 방법(breakpoint)을 재현한 기준선이기 때문이다.

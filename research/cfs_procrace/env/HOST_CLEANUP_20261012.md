# 호스트 정리 기록 (2026-10-12, 사용자 지시)

사용자 지시: "src랑 예전 프로젝트 다 지워도 돼, 이번 연구에 쓰이지 않는 건 다 지워". 정리 전 디스크 여유는 1.1 GB였고, 정리 후 16 GB가 되었다.

## 지운 것

| 경로 | 내용 | 다시 얻는 방법 |
|---|---|---|
| `/home/user/work/procrace/src` (1.4 GB) | 사례 조사용 clone과 tarball 압축 해제본(cFE, R2, C2, C3, C3-recheck, C5, C5-provenance, C5-recheck) | 사례 기록(`../cases/*.md`)에 permalink, commit, tarball sha256이 있다 |
| `/home/user/work/procrace/cfs_cord` (517 MB) | 폐기한 CORD의 계측 사본과 빌드 | `../tool/RETIRED.md`의 commit에서 patch를 꺼낼 수 있다 |
| `/home/user/work/procrace/cord_runs`, `issues`, `hist` | CORD 원시 trace(사본은 `logs/r2_c*`에 있음), 이슈 API 403 응답 본문, 빈 디렉터리 | 필요 없음 |
| `/home/user/work/` 아래 이전 프로젝트 | onnx-mlir, protobuf, dev_smoke, r1, v3, v3_followup, variants, venv, venv-export, passdev, squad-meta, onnx-models-meta, logs와 작은 파일들 | 이 연구에 쓰지 않는다 |
| `/root/.cache/pip`, `/root/.cache/uv` | 패키지 cache | 필요 없음 |
| 저장소의 이전 프로젝트 파일 | Shape-Perf harness·결과, `research/cfs_mlir` | git 이력(commit `5e087e4` 이전) |

## 남긴 것

| 경로 | 이유 |
|---|---|
| `/home/user/work/procrace/cfs_ref` | 기준 cFS v7.0.1 빌드(ENV) |
| `/home/user/work/procrace/baselines` | 기존 도구 조사 자료. A5(인증 정보 이름의 환경변수가 든 log) 결정이 아직 나지 않았다 |
| `/home/user/work/procrace/cfs_nonref_attempt` | CONDITIONS D-4가 인용하는 근거 |
| `/home/user/work/procrace/r2_runs` | R2 gdb 실행 디렉터리(`run_r2_gdb.sh`) |
| `/home/user/work/llvm-project` | 새 탐지기에 쓸 LLVM/MLIR/Clang 빌드(재사용 예정) |

cFS 실행 조건에 영향을 주는 파일(실행 디렉터리, `/dev/shm`, sysctl)은 건드리지 않았다.

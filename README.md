# cFS Process-Level Race Detection (연구 저장소)

NASA core Flight System(cFS)에서 앱·태스크 사이의 순서 위반(process-level race)을 탐지하는 연구다. 목표는 학회 논문이다.

| 위치 | 내용 |
|---|---|
| `research/cfs_procrace/Architecture/` | 새 탐지기의 아키텍처, 시각화된 워크플로우, 설계 결정 기록 |
| `research/cfs_procrace/CONDITIONS_POLICY.md` | 실험 조건 원칙(레퍼런스 기반, 임의 설정 금지) |
| `research/cfs_procrace/cases/` | 다섯 사례(C1, C2, C3, C5, R2)의 근거 기록 |
| `research/cfs_procrace/env/` | 기준 cFS(v7.0.1) 빌드·실행 조건과 측정 |
| `research/cfs_procrace/baselines/` | 기존 process-race 도구의 실행 가능성 조사 |
| `research/cfs_procrace/results/` | 실행 관측 결과 |
| `research/cfs_procrace/STATUS_*.md` | 현황과 결정 요청 |

이전 프로젝트(Shape-Perf, cfs_mlir)는 2026-10-12에 이 저장소에서 지웠다. 필요하면 commit `a78863d` 이전의 git 이력에서 꺼낸다.

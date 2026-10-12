# 배경 조사 보고서 (2026-10-12)

아키텍처 설계의 근거 자료다. 영문 원문이며, 조사 워크플로우가 작성했다. 설계 문서는 이 보고서들을 [R01]–[R07]로 인용한다.

| ID | 파일 | 내용 |
|---|---|---|
| R01 | `01_cfs_osal_psp.md` | OSAL/PSP 객체 모델과 ID 생명주기(RESERVED 구간), task 시작, 동기화 primitive별 happens-before, 실행 사이에 남는 PSP 상태, 계측 위험 지점 |
| R02 | `02_cfs_cfe_core.md` | cFE ES/SB/EVS/TBL/TIME: 시작 순서, 느슨한 barrier, 게시 패턴, 이름 레지스트리, 실제 HB edge 19개와 HB가 아닌 것 |
| R03 | `03_cfs_app_patterns.md` | 번들 앱 15개의 앱 간 의존(24행), 준비 계약 15종(다섯 사례는 3종만 포함), 측정된 순서 의존 손실 |
| R04 | `04_lit_dynamic.md` | 동적 탐지 기법 약 65종 문헌 조사, cFS 메커니즘과 기법의 대응, 기존 기법이 다루지 못하는 gap G1–G8 |
| R05 | `05_lit_systematic_static.md` | 체계적 테스트·DPOR·BMC/SMT·정적 분석·MLIR 성숙도·임베디드/비행 SW 검증 문헌, 주장 템플릿, 평가 관행 |
| R06 | `06_critique_v01.md` | 설계안 v0.1 비판, 요구사항 목록, 개방성 확보 방안, 해서는 안 되는 주장 |
| R07 | `07_env_feasibility.md` | 이 머신에서의 구현 가능성: LLVM 23(MLIR·Clang), compile_commands, uprobe/eBPF 관측 비용, ld --wrap 범위 |

문헌 인용에는 검증 상태가 표시되어 있다(V-doc, V-abs, P, UNVERIFIED). 원문 PDF는 접속 정책 때문에 열지 못했다. UNVERIFIED 항목은 논문에 그대로 인용하지 않는다.

조사 중 코드를 읽다가 발견한 결함 후보가 있다(R01, R02). 예를 들어 OSAL 이름 조회의 table lock 누수 가능성, ES `GetAppIDByName`의 RESERVED 구간 반환값 같은 것이다. 모두 코드 해석일 뿐 실행으로 확인하지 않았다.

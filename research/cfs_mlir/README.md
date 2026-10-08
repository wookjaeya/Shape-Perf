# MLIR 기반 cFS 앱 간 순서·시간 의존성 정적 분석: 연구노트

Shape-Perf와는 별개인 연구 주제의 노트와 근거 자료다.

| 파일 | 내용 |
| --- | --- |
| `cFS_MLIR_연구노트_20261002.md` | 전체 연구노트(§1–§11). 문제 정의, cFS 의미론, 선행연구, `cfs` dialect·분석 설계, 실측, 평가·16주 계획, 주장–근거 대응, 참고문헌 |
| `cFS_MLIR_연구노트_요약본_20261002.md` | 요약본. 항목마다 전체 노트의 절 번호를 단다 |
| `inputs/` | 연구노트의 바탕이 된 두 입력 노트(원노트 2026-09-30, 수정본 2026-10-01) |
| `evidence_raw/` | 본문의 `$N/…` 근거: 실행 로그, probe 출력, 검증 기록, 조사 결과 JSON(`evidence_raw/evidence/`) |

## 근거 자료에서 뺀 것

- 클론 저장소 72개(cFE, OSAL, PSP, cFS bundle, F´, IKOS, LLVM 등): 고정 commit으로 다시 받을 수 있다. remote와 HEAD commit은 `evidence_raw/MANIFEST_excluded.json`에 있다.
- 바이너리, build 출력, 중간 IR(`.ll`, `.mlir`, `.bc`), 1 MB가 넘는 파일, 논문 PDF, 공개 release tarball(cFE 6.4.1·6.4.2)을 푼 소스. 목록은 같은 manifest에 있다.

## 상태

설계·실측 단계다. 분석기는 구현하지 않았고, 검출 결과·검출률·신규성·MLIR의 우월성은 주장하지 않는다.
모든 실측은 Linux native simulation 값이며 RTOS 비행 build의 동작 근거가 아니다.

# PI 결정 요청 — 수정 전 W3의 계측 경로(G-I-H) 실행 (2026-10-29)

- 근거 문서: `plan/predict_w6_20261029.md` §5·§7(커밋 `006cb11`, `53632e7`), `plan/instrumentation_selection_20261026.md` §5.4 F2, `plan/results_instr_gate_20261026.md` §2.1 H7(L63–L67).
- 이 문서는 결정 요청이다. 결과 문서가 아니다.

## 1. 사실

| 항목 | 내용 | 표기 |
|---|---|---|
| 계획 | pre W3 = CF를 첫 `BP0_tsem` 조회 진입에서 잡고(G1_CF, `caller-in cf.so`), BP의 `CFE_ES_CreateChildTask` 진입에서 푼다(R1 `es.child_task.entry caller-in bp.so`). `kz predict`가 W6 자연 기록에서 다시 만든 그대로다. 묶음 번호와 수는 기록과 같다 | [M] |
| F0 | 두 후보 모두 계획 읽기 통과. 이번 통제 실행에서 돌린다 | [M] |
| G-I-H | `kz rtplan`이 두 후보 모두 거부한다: `point R1: caller-in bp.so: no call-side site of CFE_ES_CreateChildTask … bound in module 3`. 게이트 G1_CF는 변환된다(site 34). post W13도 같은 이유로 거부된다 | [M] |
| 설계 기록 | 단계 F 설계(F2)는 "해제점은 관측 site로 충분하다"고 적고 그 목록에 `es.child_task.entry`를 넣었다 | [F] |
| 구현 규칙 | H7은 `caller-in`이 붙은 함수 진입 점을 그 모듈의 호출 쪽 site(`place=at-call`)로 바꾸고, 평가할 site마다 manifest의 `control` 줄을 요구한다. 단계 F는 해제용 control 줄을 35·37·45에만 더했다 | [F] |
| 이전 실행 | 단계 F의 G-I-H 실행 9개는 W3·W13을 돌리지 않았다. 이번에 처음 시도해서 드러났다 | [기록] |

## 2. 선택지

| | 내용 | 비용·영향 |
|---|---|---|
| (가) | 이번 라운드에서 pre W3은 F0로만 돌린다. G-I-H 쪽은 거부 기록(`refused.txt`)만 남긴다 | 추가 작업 없음. W3에서는 두 경로 비교가 없다 |
| (나) | bp.so 안의 `CFE_ES_CreateChildTask` 호출 쪽 site와 그 `control` 줄을 C1 계측 manifest(`instr/sites-cf281a941.kzs`, `sites-cf833fdbb.kzs`)에 더한다. W1에 쓰인 호출 쪽 site 44·37·34와 같은 방식이다. 파생 빌드 4개를 다시 만든 뒤 W3(과 W13)를 변환해 돌린다 | F2가 이미 적은 해제점을 구현 규칙에 맞게 채우는 것이다. 빌드가 바뀌므로 같은 빌드에서 비교하려면 자연 G-I 8회와 G-I-H W1 4회도 새 빌드로 다시 돌려야 한다(모두 1회씩, P8). 위치 차이(호출 직전 대 피호출 함수 첫 명령)는 W1과 같은 종류로 `rtplan.txt`에 기록된다 |

## 3. 권고

**(나)**를 권고한다. 이번 재검증은 같은 제어를 두 경로에 걸어 비교하는 것이 목적이고, 근거도 이미 있다(F2의 해제점 목록, H7 규칙, site 34·37·44의 선례). 새 위치를 정하는 것이 아니라 계획이 이미 지정한 호출을 계측하는 것이다.

## 4. 지금 하는 것

결정과 무관하게 필요한 실행은 지금 진행한다: F0 post W1·pre W3, G-I-H post W1(tx·rx × 두 후보). (나)로 정하면 위 재실행을 더한다.

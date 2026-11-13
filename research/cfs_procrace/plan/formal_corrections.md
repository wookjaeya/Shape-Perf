# 형식 명세 정정 기록 (2026-10-15)

- 대상: `Architecture/spec/SPEC_FINAL.md` (rev 1.1 본문에 2026-10-15 정정을 반영한 상태. 요약은 SPEC §0.9).
- 입력: `plan/ROADMAP_20261015.md` §3, `Architecture/spec/REVIEW_RESPONSE_20261012.md` §5 항목 7·8·12, `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` (D20의 추가 완료 조건 §2.1, D22 §3.2, 실행 순서 §7의 2단계).
- 성격: 정의와 문장의 정정이다. 이번 단계에서 빌드·설치·실행을 하지 않았다. Kairos의 결과는 없다. 아래 '예제'는 SPEC의 정의를 손으로 적용한 **기대값 [I]**이며 시험 결과가 아니다.
- 표시: **[F]** 출처가 있는 사실, **[M]** 측정, **[I]** 추론, **[D]** 결정(PI 결정 또는 설계 결정), **[U]** 미확인. PI 결정은 결정으로 적었으며 검증된 사실로 적지 않았다.
- 용어(T1 = (a)): 이 문서는 '준비 상태 명세'(readiness specification), '준비 상태 전제'(readiness precondition, SPEC의 `Req`), '준비 상태 위반'(readiness violation)을 쓴다. SPEC 본문은 이름 변경 단계 전이라 아직 contract, Req, violation을 쓰며, SPEC 문장을 인용할 때는 그 용어가 그대로 남는다. 문서화된 API 자체의 의무를 말할 때만 원출처의 용어를 쓴다.
- 근거 규칙: CORD 결과는 근거로 쓰지 않는다. R2의 실행 근거는 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`)뿐이다.

## 0. 정정 일람

| # | 정정 | SPEC에서 바뀐 곳 | 출처 | 상태 |
|---|---|---|---|---|
| C-1 | 전이 규칙 2와 σ_E 문장을 L-PR·L-FO가 증명하는 것과 일치시킨다. 조기 반환 `NOT_EVALUATED`를 허용한다 | §1.8 규칙 2, §1.10 (안전 기록의 ¬NR_i, `protected` 행, 보편 클래스, 표시 규칙), G2의 σ_E 항, L-TRANS (2), A.1 step 9, A.7, A.8 LP-EARLY 변형 (c), §6.4 항목 44 | ROADMAP §3 첫 행. REVIEW_RESPONSE §5 항목 7 | SPEC에 반영 |
| C-2 | L-CONT (c)와, 인증서의 도달·전이 단계가 쓰는 다른 보조정리를 신뢰 원장 (iii)·(iv)와 R2 원장에 넣는다 | §4.5.1 (iii)·(iv)·셀별 원장, A.1 step 9a (iii)·(iv), §0.9 | ROADMAP §3 둘째 행. REVIEW_RESPONSE §5 항목 8. 결정문 §2.1 | 반영 |
| C-3 | 보조정리·보장마다 증명 상태(`proved` / `depends-on-stated-assumption` / `unproved`)를 두고, 의무로만 적힌 보조정리를 '증명됨'으로 출력하지 않는 규칙을 둔다 | §6.2 머리(정의·규칙 PS1–PS4)와 끝의 표, 각 보조정리·보장 표제, §6.3, §2.3 X-macro 3곳, §1.10 검증 기록·표시 규칙·표시 이름, §4.5·§4.5.1, §6.4 항목 44–46, §9.3 V0·V1·청구 표, ADR-045 | ROADMAP §3 셋째 행. 결정문 §2.1 | 반영 |
| C-4 | C1 기준 구성: D22를 '공개 BP 기반, 후보 쌍 CF `281a941` → `833fdbb`'로 쓴다. 이름·순서·우선순위에 레퍼런스가 없다는 설명을 정정한다 | §0.7 항목 5·7, §1.11, §4.10 항목 9, §7.3, §7.4, §8.2 C1 행, §9.3 S5·V3·사례별 C1 행, §10.1 D22 행, §10.2 위험 두 행, ADR-044, A.4 step 2·3·6 | ROADMAP §3 'C1' 두 행. 결정문 §3.2, §5 | 반영 |
| C-5 | R2 표현: '이벤트 요청 거부'로 쓰고 '이벤트·패킷 손실'로 쓰지 않는다. 시스템 호출에 관한 주장은 코드 분석이나 관측을 근거로 하며 gdb 기록만으로 하지 않는다 | §1.6 `harm` 어휘, §2.3 예시, A.1 step 8·9a·13a, §8.3 시스템 호출 행, §6.4 항목 47 | ROADMAP §3 마지막 행 | 반영 |
| C-6 | §0.8 첫 문단이 더는 사실이 아닌 두 문장을 포인터로 바꾼다 | §0.8 | REVIEW_RESPONSE §5 항목 12 | 반영 |

아래 §6에는 rev 1.1에 이미 들어 있는 두 정정(안전성과 진행성의 분리 = LP-DL, 유한 실행 접두사 포함 = LP-TW)을 이번 정정과 어떻게 연결했는지 적었다. 그 두 예제는 증명 상태 필드가 실제로 무엇을 막는지 보여 주기 때문이다.

**이번에 고치지 않은 것.** 한국어 미러 `Architecture/01_overview.md`–`10_decisions_adr.md`와 `PI_DECISIONS.md`(미러 단계), T1 이름 변경(Spec-C 단계), D20을 '제안'이라고 적은 곳과 2026-10-15의 다른 결정(P-R2CTRL, D13, P-C2 등)의 반영(결정 반영 단계), SPEC에 남은 CORD 기록 인용(CORD 정리 단계)은 건드리지 않았다. 목록은 §7.

---

## 1. C-1 전이 규칙 2와 σ_E 문장

### 1.1 문제

REVIEW_RESPONSE §5 항목 7 (blocking)이 지적한 불일치이다. 네 곳의 문장이 같은 대상을 다르게 말했다.

| 곳 | 내용 |
|---|---|
| 정의 (SPEC §1.10) | `¬NR_i`는 "도달한 범위 안 발생 중 준비 안 됨으로 판정된 것이 없다"이다. `NR_i`의 '준비 안 됨'은 `TOLERATED`, `PENDING`, `VIOLATING`, `NOT_READY_UNJUDGED`이다. |
| 증명 (SPEC §6.2 L-PR) | 모든 판정된 범위 안 발생은 `READY` 또는 `NOT_EVALUATED`이다. 이것이 `¬NR_i`이다. |
| 기대 출력 (SPEC §2.5 O4(b), A.8 LP-EARLY) | `NOT_EVALUATED` 발생(호출이 어느 conjunct의 LP보다 먼저 반환)은 준비 상태도 아니고 준비 안 됨도 아니므로 O4(b)의 실패가 아니다. |
| 보장 문장 (SPEC §1.8 전이 규칙 2, 이전 문안) | σ_E 셀은 "실제 실행에서 범위 안 발생이 … `READY`가 아닌 것이 없다(`¬NR_i`)"고 진술한다. |

마지막 문장의 "`READY`가 아닌 것이 없다"는 `NOT_EVALUATED`까지 배제한다. 조기 반환이 있는 실제 실행은 L-PR과 O4(b)가 허용하는데 규칙 2의 문장을 글자 그대로 읽으면 그 실행이 σ_E 셀의 반례가 된다. 보장이 증명한 것보다 강했다.

### 1.2 이전 문장과 새 문장

**이전** (SPEC §1.8 규칙 2의 마지막 두 문장, 정정 전):

> The σ_E cell then states that no in-scope occurrence in any real run of B is `VIOLATING` or `NOT_READY_UNJUDGED` (`¬Bad_i`), `READY` or `TOLERATED` (`¬OK_i`), or other than `READY` (`¬NR_i`). Otherwise the σ_E cell stays `unknown(model_level)`, and the M_api result is reported as a portability claim.

**새** (같은 곳):

> The σ_E cell then states, for the **decided** in-scope occurrences of every finite real run of B, each safety statement that is established there (§1.10 safety record), in exactly the form in which the certificate lemmas prove it (L-FO, L-PR, §6.2): `¬Bad_i`: none is `VIOLATING` or `NOT_READY_UNJUDGED`; `¬OK_i`: none is `READY` or `TOLERATED`; `¬NR_i`: none is decided not ready (`TOLERATED`, `PENDING`, `VIOLATING` or `NOT_READY_UNJUDGED`), i.e. every decided one is `READY` or `NOT_EVALUATED`. An occurrence that is still open when a run ends has no status and lies outside all three statements. A `NOT_EVALUATED` occurrence (its call returned before one of its conjunct LPs and no conjunct was found false, e.g. an early return on an argument check) is neither ready nor not ready; all three statements allow it, so a real run that contains one is no counterexample to the cell. The cell does not state that every in-scope occurrence of a real run is `READY`, that any occurrence is reached, or anything about progress. Otherwise the σ_E cell stays `unknown(model_level)`, and the M_api result is reported as a portability claim.

바뀐 점:

1. 세 진술(`¬Bad_i`, `¬OK_i`, `¬NR_i`)이 모두 **판정된(decided)** 발생에 대한 것이다. 실행이 끝났을 때 아직 열려 있는 발생(호출은 시작했고 판정 전)은 상태가 없고 진술 밖이다.
2. `¬NR_i`는 "`READY`가 아닌 것이 없다"가 아니라 "준비 안 됨으로 판정된 것이 없다"이며, 같은 말로 "판정된 것은 모두 `READY` 또는 `NOT_EVALUATED`"이다.
3. 셀이 말하지 않는 것(모든 발생이 `READY`, 도달, 진행성)을 문장에 적었다.

### 1.3 같이 고친 문장

| SPEC 위치 | 이전 | 새 |
|---|---|---|
| §1.10 안전 기록 | `¬NR_i` ("no reached in-scope occurrence is decided not ready") | `¬NR_i` ("no in-scope occurrence is decided not ready", i.e. every decided one is `READY` or `NOT_EVALUATED`) |
| §1.10 클래스 표 `protected` 행 | "with `¬NR_i`, every one is `READY`" | 같은 말에 괄호를 붙임: 도달한 발생은 판정되었고 `NOT_EVALUATED`가 아니다(`Reach_i`). `NOT_EVALUATED`는 어느 진술도 세지 않으므로 `¬NR_i`는 "모든 발생이 `READY`"가 아니다 |
| §1.10 보편 클래스 (σ_E) | "they state the safety statement for every real run" | 판정된 범위 안 발생에 대해 인증서가 증명한 것과 같은 진술이며 `NOT_EVALUATED` 조기 반환을 허용한다 |
| §1.10 표시 규칙의 예 | "protected — every reached occurrence READY (certificate, b = ∞); reach proved; progress not checked" | "protected — every decided occurrence READY or NOT_EVALUATED (certificate, b = ∞); reach proved; progress not checked; lemma status: unproved" |
| G2의 σ_E 항 | "It holds for every finite real run of B …" | 같은 문장 뒤에 "What holds is the statement as L-FO or L-PR proves it, over the decided in-scope occurrences of the run (§1.8 rule 2) …"를 추가 |
| L-TRANS (2) | "gives the σ_E statement for every finite real run" | 같은 `¬Bad_i`/`¬OK_i`/`¬NR_i`를 판정된 발생에 대해 말하며, 실행의 발생 상태는 그 사영의 상태(L-STATUS (a))이고 `NOT_EVALUATED`는 셋 어느 것도 반박하지 않는다 |
| A.1 step 9 σ_E | "no in-scope occurrence of any real run is `READY`" | "no decided in-scope occurrence of any real run is `READY` or `TOLERATED` (`¬OK_i`, deciding); a `NOT_EVALUATED` occurrence … is allowed" |
| A.7 | "(`¬NR_i`: every reached lookup is READY)" | "(`¬NR_i`: every decided in-scope lookup occurrence is READY or NOT_EVALUATED, so every reached one is READY)" |
| A.8 LP-EARLY | 변형 (b)(재시도)까지 | 변형 (c) 추가: 레지스트리가 INIT에서 참이고 방해자가 없는 PROTECTED 판 (아래 §1.4) |
| §6.4 | (없음) | 항목 44: σ_E 셀이 "모든 발생이 `READY`"라고 말한다는 주장은 하지 않는다 |

### 1.4 최소 예제: 조기 반환 `NOT_EVALUATED` (손으로 적용, 기대값 [I])

SPEC A.8의 LP-EARLY와 같은 모양에서 레지스트리만 INIT에서 참으로 바꾼 변형 (c)이다.

```c
int32 lk(void *p, const char *name)
{
    if (p == NULL) return ERR_ARG;   /* 조기 반환: 레지스트리를 읽기 전 */
    return registry_lookup(name);    /* 준비 상태 전제(레지스트리 r이 LIVE)의 LP: 이 호출 범위 안의 읽기 */
}
/* 호출자 U는 lk(x, "n")을 부른다. x는 havoc 값(NULL 또는 NULL이 아님). */
/* r은 INIT에서 LIVE이고 writer·disabler가 없다. 따라서 PROTECTED 인증서 (a)가 성립한다. */
```

실제 코드의 대응 [F, 이번 단계에서 다시 읽음]: OSAL d2d877a의 `OS_CountSemGetIdByName`은 `OS_CHECK_POINTER(sem_id); OS_CHECK_POINTER(sem_name);` (`osal/src/os/shared/src/osapi-countsem.c` L209–L210)를 `OS_ObjectIdFindByName` 호출(L212)보다 먼저 실행한다. 레지스트리 읽기가 conjunct의 LP라는 것은 SPEC §1.4의 모델링이다 [I].

SPEC §1.6 '발생의 상태' 정의를 두 실행에 적용한다.

| 실행 | 호출 | LP 통과 | 거짓으로 판명된 conjunct | 정의에 따른 상태 |
|---|---|---|---|---|
| A: `x != NULL` | 시작 | 레지스트리 읽기 LP를 지남. 값은 LIVE | 없음 | 마지막 conjunct LP에서 판정. 모든 conjunct 참 → `READY` |
| B: `x == NULL` | 시작 | 어떤 conjunct의 LP도 지나지 않고 `ERR_ARG`로 반환 | 없음 (capture도 없음) | 호출의 반환이 가장 이른 판정 사건이고 지나지 않은 LP가 있음 → `NOT_EVALUATED` |

세 문장이 이 예제에서 어떻게 읽히는지:

| 항목 | 이전 규칙 2 문장 ("`READY`가 아닌 것이 없다") | 새 문장 (판정된 발생이 모두 `READY` 또는 `NOT_EVALUATED`) | L-PR, O4(b) |
|---|---|---|---|
| 실행 A | 만족 | 만족 | 만족 |
| 실행 B | **위반**: `NOT_EVALUATED`는 `READY`가 아님 | 만족: 판정되었고 `READY`이거나 `NOT_EVALUATED`임 | 허용. O4(b) 실패 아님 |
| σ_E 셀이 반례를 만나는가 | 만난다 (실행 B) | 만나지 않는다 | — |
| 도달 | — | 실행 B만 관측되면 도달은 증명되지 않는다. `assumed`이며 SPEC의 표시 규칙대로 "… if reached"(도달한다면)로 읽는다 | 도달은 인증서가 아니라 판정된 발생이 있는 실행이 증명한다 |

따라서 정의(§1.10), 보장 문장(§1.8 규칙 2), 기대 출력(O4(b), LP-EARLY)이 같은 말을 한다. 레지스트리가 INIT에서 거짓인 FIXED_ORDER 판(기존 LP-EARLY)에서는 실행 A가 `VIOLATING`, 실행 B가 `NOT_EVALUATED`이고 `¬OK_i`가 성립한다. 같은 조기 반환이 두 인증서 종류에서 같은 방식으로 처리된다.

**남는 한계.** `¬NR_i`는 도달, 진행성, 그리고 조기 반환 뒤 호출자가 `ERR_ARG`를 어떻게 다루는가에 대해 말하지 않는다. 호출자의 처리는 같은 호출을 판정하는 이 인스턴스의 일이 아니다 [I].

---

## 2. C-2 신뢰 원장에 L-CONT (c)와 도달·전이 단계의 보조정리

### 2.1 문제

REVIEW_RESPONSE §5 항목 8 (blocking). FIXED_ORDER 인증서의 도달 항목(SPEC §4.5 항목 6)과 A.1 step 9, §1.10의 도달 규칙은 (σ_U, M_impl) 셀의 도달을 "실행 하나가 있다(L-CONT (c))"로 증명한다. 그런데 §4.5.1 (iii)은 L-CONT를 "인증서 자신의 셀이 아닌 셀" 항목에서만 적었고, 셀별 규칙은 실행 기반 도달에 L-REACH-CONE과 L-MAPI-CONE만 이름 붙였으며, A.1 step 9a (iii)의 보조정리 목록에는 L-CONT가 없었다. 결과 하나를 골랐을 때 그 결과가 쓴 모든 보조정리를 추적할 수 없었고, 렌더링되는 `ledger:` 크기도 작게 나왔다.

### 2.2 이전 문장과 새 문장

**이전** (§4.5.1 (iii)의 보조정리 항목):

> The lemma obligations whose proofs are still sketches: L-MB, L-ONCE, L-PWB, L-DERIVE, L-FO or L-PR, L1; L-SCOPE where a SCOPE exclusion is used; L-MAPI-CONE where M_api reach or an M_api witness rests on a run; and, for cells other than the certificate's own, L-STATUS, L-MON, L-CONT and L-TRANS, plus L-MLEVEL where rule 2(i) is used, L-REACH-CONE where σ_U∞ reach is proved by a run, and L-CONE-R1 for bounded σ_R1 transfers.

**새** (요지. 전문은 SPEC §4.5.1):

- 인증서 자신의 셀에는 **항상** L-MB, L-ONCE, L-PWB, L-DERIVE, L-FO(또는 L-PR), L1, **L-STATUS**(상태의 최종성. 불량성 항목과 실행에서 읽는 모든 상태가 기댄다)가 들어간다.
- **도달이나 증인이 실제 실행에 기대는 모든 셀에는, 인증서 자신의 셀을 포함하여 L-CONT (c)가 들어간다.** M_api에서는 L-MLEVEL 또는 L-MAPI-CONE이 함께 들어가며, L-MAPI-CONE도 실행의 LP 단위 사영에서 출발한다.
- 다른 셀에는 L-MON, L-CONT (a)·(b)·(b′)(그 셀의 σ가 요구하는 것), L-TRANS, 조건에 따라 L-MLEVEL, L-REACH-CONE, L-CONE-R1.
- 각 항목은 증명 상태를 갖는다 (C-3).
- (iv) '반증 가능, 증명 아님': L-CONT (c), L-MAPI-CONE, L-REACH-CONE의 실행 쪽 전제는 O1·O2(체커가 받아들이지 않는 행위자 사건열), O4(결과 코드와 계산된 상태의 불일치), O5(닫힘 밖의 행위자·키)로 반증된다. 통과해도 "실행 집합 R의 확인된 경로에서 반증되지 않음"으로만 적고 증명으로 적지 않는다.
- '도달은 관측이 증명하는 유일한 항목'이라는 문장을 '관측이 성립시키는 유일한 항목이며, 그 셀에 이름 붙은 L-CONT (c) 등을 통해서만 그렇다'로 고쳤다.

A.1 step 9a (iii)에도 같은 목록을 반영했다. (iv)에는 위 반증 의무를 한 줄 추가했다.

### 2.3 최소 예제: R2 ES 반쪽 (σ_U, M_impl) 셀의 도달 추적 (기대값 [I])

"결과 하나를 선택하면 사용된 모든 보조정리·가정·추출 자료까지 추적 가능"이라는 완료 기준을 R2에서 확인한다. 아직 S1(자체 기록기)이 없으므로 모든 칸은 SPEC이 기대하는 구조이다.

| 단계 | 내용 | 원장 위치 | 상태 |
|---|---|---|---|
| 결과 | (σ_U, M_impl)에서 ES 반쪽의 `reach = proved` | 셀 레코드 | — |
| ← 실행 기록 | 사용 발생이 있는 실행 하나와 접두 탐침 기록 | (ii) 추출 사실의 `RUN`·`PROBE` 항목 | S1 미구현. 관측 단위는 gdb 3회 [M]가 있으나 Kairos 기록기의 것이 아님 |
| ← 가정 | A2(바인딩), A6(관측 완전성) | (iii) 가정 | 가정 |
| ← 보조정리 | **L-CONT (c)**: 실제 실행의 LP 단위 사영 ∈ `Pref(B, σ_U, M_impl)` | (iii) 보조정리, `proof = DEPENDS` | `depends-on-stated-assumption` |
| ← 가정 (L-CONT (c)의 근거) | A7-impl, A7-api, A-SKEL, A3, A4, A9 | (iii) 가정 | 가정 |
| ← 보조정리 | **L-STATUS**: 실행에서 읽은 발생의 상태 = 그 사영의 상태 | (iii) 보조정리, `proof = UNPROVED` | `unproved` |
| ← 반증 가능 | O1·O2(행위자 사건열이 인증서 뷰 자동자에 받아들여지는가), O4, O5 | (iv), 상태 `not run` (S1 전) | — |

(σ_U, M_api) 셀은 L-MLEVEL이 적용되지 않으므로(cone에 task 생성이 있음) 실행이 도달을 증명하려면 M_api 적합성 검사(L-MAPI-CONE)를 통과해야 하고, 이 보조정리도 같은 실행 사영에서 출발하므로 L-CONT (c)가 함께 적힌다. 이 항목이 없던 이전 원장에서는 이 연결을 원장으로 추적할 수 없었다.

보조정리 항목 수의 변화는 SPEC 문안에서 센 기대값이다 [I]. 도달이 실행으로 증명되는 경우 (σ_U, M_impl) 셀의 보조정리 항목은 이전 6개(L-MB, L-ONCE, L-PWB, L-DERIVE, L-FO, L1)에서 8개(+ L-STATUS, L-CONT (c))가 되고, (σ_U, M_api) 셀은 이전 7개(+ L-MAPI-CONE)에서 9개가 된다. 도달이 `assumed`이면 두 셀 모두 7개(L-STATUS 추가)이고 L-CONT (c)·L-MAPI-CONE은 빠진다.

---

## 3. C-3 증명 상태

### 3.1 문제

ROADMAP §3 셋째 행과 결정문 §2.1: 필요한 증명 의무를 목록으로 작성했다는 이유만으로 '증명됨'이라고 출력하지 않는다. SPEC의 보조정리는 증명 스케치이거나 미완료 의무인데 판정 기록은 어느 쪽인지 알려 주지 않았다. 신뢰 원장이 의무를 '목록으로' 적는 것이 증명 완료처럼 읽힐 수 있었다.

### 3.2 정의 (SPEC §6.2 한 곳. 여기는 요약)

| 값 | 뜻 |
|---|---|
| `proved` | 명세에 완전한 증명이 있다. §1의 정의와 그 자신이 `proved`인 보조정리만 쓴다. §6.1의 가정, 추출 사실, 쓰이지 않은 단계가 없다 |
| `depends-on-stated-assumption` | 논증이 §6.1이 id로 적은 가정(`A-…`, `T-JACM`) 아래에서 완전하다. 쓰는 다른 보조정리는 `proved` 또는 이 값이다. 표의 행이 가정을 이름 붙인다 |
| `unproved` | 그 밖의 모든 경우. 스케치에 주장만 있는 단계가 있거나, 의무·[I]·"to be written"으로 표시되었거나, `unproved` 보조정리를 쓴다 |

- **가장 약한 고리**: 여러 보조정리를 쓰는 진술의 상태는 그중 가장 약한 것이다. 구현 요구(G9, G10, G8의 재생 부분)는 정리가 아니므로 `unproved`이고 시험으로 확인한다. 부분이 있는 보조정리(L-CONT, L-TRANS, L-DEAD)는 부분마다 상태가 있다.
- **PS1** 상태는 이 텍스트의 성질이다. 증명을 쓰는 같은 변경에서 표를 고칠 때만 올라간다. 도구 실행, 통과한 시험, 통과한 의무(O1–O10), 원장 항목은 올리지 못한다. 검토에서 틈이 발견되면 내려간다. 현재 상태는 작성자의 분류이고 독립 검토 기록은 아직 없다 [I]. V0-P가 검토를 기록하는 곳이다.
- **PS2** 의무로만 적힌 보조정리는 `unproved`이다. 어떤 판정·보고·원장 출력·README·논문 문장도, 상태가 `proved`가 아닌 보조정리에 대해 "proved"를, 그 위에 선 진술에 "verified"·"certified"를 쓰지 않는다.
- **PS3** 인증서, 인증서 기반 판정 셀, 원장이 진술의 증명 상태(원장 (iii)의 `LEMMA` 항목 중 가장 약한 것)를 갖고, 보고는 안전성 진술 옆에 보조정리 이름과 함께 찍는다. "kz proofcheck가 받아들임" 뒤에는 항상 "신뢰 기반에 상대적으로"와 이 상태가 붙는다.
- **PS4** `proved`는 보조정리의 상태이지 판정의 상태가 아니다. 모든 보조정리가 `proved`여도 인증서는 추출 사실, 골격, 닫힘 목록, Φ, `libkz-core`, 원장의 가정에 상대적이다. 모델에 대한 진술을 바이너리에 대한 진술로 바꾸지 않는다.

### 3.3 필드가 놓인 곳

| 레코드 | 필드 | 값 |
|---|---|---|
| `KZ_TRUST` (원장 항목, §2.3) | `proof` (기존 예약 바이트 `rsv0`를 대체. 크기 60바이트 불변) | `LEMMA` 항목: PROVED, DEPENDS, UNPROVED. 그 밖: NA |
| `KZ_CERT` (인증서) | `proof_status` (기존 `rsv0` 대체. 128바이트 불변) | 자기 셀 원장 (iii)의 `LEMMA` 항목 중 가장 약한 것. `kz proofcheck`가 받아들일 때 쓴다 |
| `KZ_VERDICT_CELL` (판정 셀) | `proof_status` + `rsv2[3]` (정렬 때문에 3바이트 예약 추가. 468 → 472바이트) | 위와 같음. 해당 항목이 없으면 NA |
| 렌더링 | `lemmas:<p>/<d>/<u>` 표시 이름(비트집합에 저장하지 않음) | (iii) `LEMMA` 항목 중 각 상태의 개수 |

원장 레코드(`KZ_TRUST_LEDGER`)에는 새 필드를 넣지 않았다. 상태는 항목에 있고 셀·인증서의 상태는 항목에서 계산된다.

### 3.4 보조정리·보장별 상태 (SPEC §6.2 끝의 표를 줄여 옮김)

| 상태 | 대상 |
|---|---|
| `proved` | L-CONT (a)(σ_U∞의 유한 실행 = σ_U에서 시간 기반 전이가 없는 유한 실행), 그 따름정리(역방향 접두사 포함), L-DEAD (b)(LP-TW (a)로 증명되는 '더하면 살아나고 빼면 죽는다') |
| `depends-on-stated-assumption` | L-DERIVE (A-EXTRACT, A-TYPE, A2), L-CONT (c)(A7-impl, A7-api, A-SKEL, A3, A4, A9), L-DEAD (c)·(d)(A1, A7-impl), G1, G3, G6 |
| `unproved` | L-MB, L-ONCE, L-PWB, L-SCOPE, L-FO, L-PR, L-CONT (b)·(b′), L-REACH-CONE, L-MAPI-CONE, L-STATUS, L-MON, L-TRANS, L-MLEVEL, L-REF, L-DPOR(`exh`·`dpor`), L-DEAD (a), L1, L2, L3, L-PRED, L-LOOP, G2, G4, G5, G7, G8, G9, G10, G11 |

판단 근거와 상태를 올리는 방법은 SPEC 표의 '왜 이 상태인가' 열과 `plan/proof_status_R2.md`에 적었다. 요점 몇 가지:

- L-MB는 `unproved`이다. T-OBS 사례가 PWB 사실을 쓰는데 그 사실이 같은 MB에서 도출되므로 MB와 PWB 사실의 공동 귀납("앞선 발생에서의 귀납 가정으로 정당화")이 주장만 되고 쓰이지 않았다. 또 L1이 `unproved`이다. L-ONCE, L-PWB, L-SCOPE, L-FO, L-PR이 이 사슬 위에 있어 같이 `unproved`이다.
- L-STATUS는 정의에 가까워 `proved`로 올릴 수 있어 보이지만, 여섯 가지 지평 감시자가 하나씩 최종적이며 ρ의 사건만 읽는지 확인하는 단계가 쓰이지 않았다. 그 확인을 쓰면 올라간다.
- L-CONT (c)는 단계 모의가 "실행 길이에 대한 귀납"이라고만 적혀 있어 `depends-on-stated-assumption`이며, 검토에서 틈이 나오면 `unproved`로 내려간다.

### 3.5 예제 1: LP-DL — 준비 상태 전제는 증명되었는데 사용 전에 교착할 수 있는 경우

SPEC A.8 LP-DL (a)를 손으로 적용한다 (기대값 [I]).

```
loc: INIT에서 VALID. writer·disabler 없음.
T0 (한 번만 실행): 뮤텍스 m1, m2를 만들고 τ_A, τ_B를 띄운다.
τ_A: take m1; take m2; use loc; give m2; give m1.
τ_B: take m2; take m1; give m1; give m2.
OS_MutSemTake에는 timeout이 없다.
```

| 항목 | 값 |
|---|---|
| 인증서 | PROTECTED (a). `loc`이 INIT에서 참이고 방해자가 없다. `¬NR_i` (σ_U, M, b = ∞): 판정된 모든 발생은 `READY` 또는 `NOT_EVALUATED` |
| 인증서 기반 진술의 보조정리 상태 | `unproved` (L-PR, L-STATUS; L-PR 전체가 `unproved`). 이 예제에서 실제로 쓰이는 부분은 '방해자 없음' 경우뿐이지만 상태는 보조정리 단위로 붙으므로 그대로 `unproved`를 찍는다. 더 세분하려면 L-PR을 (a)·(b) 부분으로 쪼개 각각 증명해야 한다 |
| 교착 | τ_A가 m1을, τ_B가 m2를 잡으면 τ_A는 m2를, τ_B는 m1을 기다린다. 두 행위자 모두 죽었고(dead) 사용 `use loc`는 τ_A 앞에 남아 있다 → 사용 전 정체(pre-use stall), 종류 `deadlock-cycle`. 이 정체는 `exh`만 결정한다 |
| 진행 결과 | 인증서만: `unknown(not_checked)`. `exh`: `stall-witness({pre-use})`, `deadlock-cycle`, 확인 가능, 위해 `hang_or_deadlock`. 이 경로의 보조정리 상태: L-DPOR(`exh`), L-DEAD (a), L-MON — 모두 `unproved` |

**이전 문장**(rev 1.0): `protected`는 `S_i = ∅`를 요구한다. 이것은 "사용 전에 교착이 없다"까지 말한다. 그런데 인증서도 L-PR도 `S_i = ∅`를 증명하지 않으므로 보장이 증명보다 강했다. 이 예제는 그 반례이다.

**새 출력**(표시 규칙):

```
protected — every decided occurrence READY or NOT_EVALUATED (certificate, b = ∞); reach assumed;
  lemma status: unproved (L-PR, L-STATUS)
progress (exh, within b): stall-witness({pre-use}), deadlock-cycle, confirmable, hang_or_deadlock;
  lemma status: unproved (L-DPOR exh, L-DEAD (a), L-MON)
```

출력하지 않는 것: "protected, 교착 없음", "no-stall-within(b)"(이 예제에서 `exh`는 이것을 출력하지 못한다. SPEC A.8 완료 조건), "proved".

### 3.6 R2 첫 결과에서의 상태

R2의 FIXED_ORDER 인증서(ES·EVS 반쪽, (σ_U, M_api)·(σ_U, M_impl))가 쓰는 보조정리에는 `unproved`인 L-MB, L-ONCE, L-PWB, L-FO, L1, L-STATUS가 들어 있다. 따라서 모든 셀이 "lemma status: unproved"를 찍는다. 전체 표는 `plan/proof_status_R2.md`.

---

## 4. C-4 C1 기준 구성 (D22)

### 4.1 이전 문장과 새 문장

| 곳 | 이전 | 새 |
|---|---|---|
| §10.1 D22 | "(candidate) … provider, name, startup order and priorities are `unspecified_by_reference` … none chosen; a v7.0.1 option (CF `15a871e`) is listed next to the BP-based and role-derived options" | "decided 2026-10-15 [D]: **public BP basis; candidate pair CF `281a941` → `833fdbb`**." 공개 값과 미확인 값을 나누어 적음 (아래 §4.2) |
| ADR-044 결정 항목 | "V3 activates C1 on a v7.0.1 B′ (D22)" (REVIEW_RESPONSE §5 항목 1이 지적한 '선택되지 않은 것을 선택한 듯한 표현') | V3는 D22가 정한 공개 BP 기반에서 C1을 켠다. 공개 통합의 버전 호환성을 먼저 확인하고, 그대로 구성할 수 없으면 다른 버전으로 옮긴 차이를 모두 기록한다. 원 보고의 CF `2a024d8`을 공개 BP 테이블과 섞지 않는다. 현재 v7.0.1 기본 구성(빈 이름)은 경로 비활성 음성 대조로만 쓴다 |
| §9.3 V3·사례별 C1 행 | "D22, none chosen: the v7.0.1 option with CF `15a871e`, or a derived option with CF `2a024d8`/`281a941`" | D22 결정(공개 BP 기반, 후보 쌍, 공개 값)을 적음 |
| §8.2 C1 행 | "CF `2a024d8` … or `281a941` … on a derived B′; CF `15a871e` on a v7.0.1 B′" | 공개 BP 기반의 `281a941`(수정 전). `2a024d8`은 공개 BP 테이블과 섞지 않음. 기본 B는 음성 대조 |
| A.4 step 2 | "`unspecified_by_reference` at v7.0.1: the provider's build …, name, entry, stack, priority and script position; …" | 이름·순서·우선순위는 공개 BP 값(근거 줄 번호 포함). 원 보고자 환경, 전체 버전 조합, 가변 참조의 당시 내용, 실제 RT 우선순위 적용, CPU 수·affinity, 다른 버전으로 옮길 때의 차이는 미확인 |
| A.4 step 3·6 | 수정 전 구조를 `2a024d8`에서 읽음. `R`을 `15a871e`의 25로 예시 | 수정 전은 선택한 쌍의 `281a941`(단일 조회, `cf_cfdp.c` L993–L1001). `2a024d8`의 구조는 `281a941`에서 다시 읽는다 [U]. `R`은 선택한 버전의 소스에서 읽으며 모든 버전에 고정하지 않는다 (`833fdbb`에서 25 × 100 ms) |
| §0.7, §1.11, §4.10, §7.3, §7.4, §9.3 S5, §10.2, ADR-044 맥락·결과 | D22를 '결정되지 않음'·'기다림'으로 읽히게 하는 문장 | D22는 결정되었고 기반은 아직 빌드·버전 확인 전이라는 문장 |

### 4.2 공개 값과 미확인 값

| 항목 | 값 | 근거 | 상태 |
|---|---|---|---|
| semaphore 이름 | `BP0_tsem`, `BP1_tsem` | nasa/bp `b051eb0`의 `.github/buildconfig/tx/tables/cf_def_config.c` L40·L54 [F, 이번 단계에서 raw 파일을 다시 읽음]. `rx/` 표도 같은 이름이며 로컬 ID만 다름 (`cases/C1.md` F9). `fsw/custom/bp_semcfg.c` L43–L49 | 공개 BP 구성의 값 (from-reference). 원 보고자 환경의 값이라는 뜻이 아님 |
| 시작 스크립트 순서 | CF 다음 BP | `.github/buildconfig/cfe_es_startup.scr` L6 (cf), L7 (bp) [F, 다시 읽음] | 같음 |
| 설정 우선순위 | CF 65, BP 55 | 같은 줄 [F] | 같음. 실제 OS의 실시간 우선순위 적용은 별개이며 미확인 [U] |
| 수정 전후 | CF `281a941` → `833fdbb` | `cases/C1.md` (`833fdbb`의 부모가 `281a941`) | 결정 [D]. 후보 쌍 |
| 재시도 상수 | `833fdbb`에서 `CF_STARTUP_SEM_MAX_RETRIES` 25, `CF_STARTUP_SEM_TASK_DELAY` 100 ms (`cf_platform_cfg.h` L329–L349). `281a941`은 단일 조회 | `cases/C1.md` | 선택한 버전의 소스에서 읽는다. 25나 100 ms를 모든 버전에 고정하지 않는다 |

아직 자동으로 확정되지 않는 것 [U]: cFS·cFE·OSAL·PSP의 전체 버전 조합과 당시 가변 참조(nasa/bp CI가 합친 PSP 브랜치 등)의 실제 내용, 원 보고자의 provider가 공개 BP 구성과 같은지, 실제 실시간 우선순위 적용·CPU 수·affinity, 다른 버전으로 옮길 때 필요한 구성·코드 차이.

**적용 순서** [D]: 공개 통합의 버전 호환성을 먼저 확인한다. 그대로 구성할 수 없으면 다른 버전으로 옮긴 차이를 모두 `deviation`으로 기록한다 (예: v7.0.1의 CF `15a871e`는 25 × 100 ms 재시도와 ENV의 시작 항목 우선순위 70을 가지므로, 그쪽으로 옮기면 공개 값 CF 65와 달라진다). 원 보고의 CF `2a024d8`과 공개 BP 테이블은 형식이 달라 그대로 섞지 않는다 (공개 테이블은 `281a941`과 같은 채널별 표 구성을 쓴다, `cases/C1.md`).

### 4.3 최소 예제

D22 정정은 판정 규칙을 바꾸지 않고 기준 구성의 서술을 바꾼다. 확인 예: ADR-044, §9.3, §10.1, A.4가 모두 같은 구성(공개 BP 기반, `281a941` → `833fdbb`, 공개 값 세 가지)을 가리키는가를 `grep`으로 점검했다 (SPEC에서 D22가 '미결정'이라고 읽히는 문장 없음 확인, 이번 단계). 같은 값을 한국어 미러와 `PI_DECISIONS.md`가 아직 '선택 없음'으로 적고 있으므로 그 일치는 미러 단계에서 확인해야 한다 [U].

---

## 5. C-5 R2 표현

### 5.1 이전과 새

| 곳 | 이전 | 새 |
|---|---|---|
| §1.6 `harm` 어휘, §2.3 K-ID 예시, A.1 step 8·9a | 위해 값 `event_lost_silently` | `event_request_rejected` (이벤트 서비스가 패킷을 만들기 전에 요청을 거부하고 호출자가 상태를 버린다) |
| A.1 step 8 | consequence "status ignored and effect lost silently" | consequence "status ignored"와 "event rejected": EVS가 `cfe_evs.c:185–189`에서 요청을 거부하며 패킷을 만들기 전이므로 잃을 패킷이 없다. 표현은 "event requests rejected" |
| A.1 step 13a | "Observed: 'message lost' and 'event rejected'; … the loss leaves no trace" | "message lost"는 ES의 이벤트 패킷(만들어져 보내졌고 SB의 구독자 없음 규정 결과)에만, "event rejected"는 cSB4305.5의 `SEND_NO_SUBS` 이벤트 요청(패킷이 만들어지기 전에 거부)에만 쓴다 |
| §8.3 시스템 호출 행 | "the records found zero syscalls for an OSAL name create plus lookup ([R06 §6.2 C5])" | OSAL 자신의 `count-sem-test`(OSAL d2d877a, cFS 밖에서 실행)에 대한 탐침이 이름 있는 계수 semaphore의 생성과 이름 조회에 시스템 호출 0개를 냈다 [M, 그 탐침에 한정, `baselines/availability.md` §4.2]. CF의 조회, ES 기록 쓰기, R2의 거부된 요청이 호출하지 않는다는 것은 코드 분석 [I]이며, 기준 cFS를 시스템 호출 관측기로 돌리기 전(`availability.md` 결정 A10)까지는 관측이 아니다. gdb 기록에는 시스템 호출 관측이 없다 |
| §6.4 | 항목 22: 거부된 요청을 '손실 패킷'으로 쓰지 않는다 | 항목 47 추가: 위 표현, 그리고 시스템 호출의 유무를 디버거 기록만으로 주장하지 않는다 |

### 5.2 근거 [F] 이번 단계에서 다시 읽은 것

고정 소스 cFE `c5fb2b4d`의 `cfe/modules/evs/fsw/src/cfe_evs.c`, `CFE_EVS_SendEventWithAppID` (L169부터):

- L185 `AppDataPtr = EVS_GetAppDataByID(AppID);`
- L188 `Status = CFE_EVS_APP_ILLEGAL_APP_ID;` (조회 결과가 NULL일 때)
- 패킷 생성 `EVS_GenerateEventTelemetry(…)` (L204)는 같은 else-if 사슬의 뒤쪽 가지 — AppData가 있고, 등록되어 있고, 필터되지 않은 경우 — 에만 있다.

따라서 `ILLEGAL_APP_ID` 가지는 패킷을 만들기 전에 반환한다. 이벤트 요청 24회가 거부된 것이지 이미 만들어진 텔레메트리 패킷 24개가 손실된 것이 아니다. 24회가 세 번의 실행에서 같았다는 것은 gdb 기록의 측정이다 [M, `results/R2_gdb_v701.md` §2, §7]. 24회는 한 원인에서 나온 반복 호출이며 독립 결함 24개가 아니다 (같은 문서 §6).

### 5.3 SPEC 밖에서 남은 것

`results/R2_gdb_v701.md` L54(“24 SB events were lost silently … no syscall was made”)와 L81(“invisible … at the syscall level”)은 같은 문제의 표현을 담고 있다. 그 파일은 이번 단계의 소유 파일이 아니므로 고치지 않았다. 해당 문서를 맡은 단계에서 '이벤트 요청 24회가 거부됨'으로, 시스템 호출 문장은 '관측하지 않음'으로 고쳐야 한다 [U].

---

## 6. rev 1.1에 이미 있는 두 정정과의 연결 (LP-DL, LP-TW)

이 두 정정은 EXT-20261012 §4.1·§4.2에 대한 것이고 SPEC rev 1.1에 들어 있다. 이번 단계에서 새로 한 일은 증명 상태 필드와의 연결뿐이다.

### 6.1 LP-DL: 안전성과 진행성의 분리 (§3.5의 예제)

- **이전**: `protected`는 `S_i = ∅`(사용 전 정체 없음)를 포함. 인증서는 이것을 증명하지 않았다.
- **새**: 안전성 진술(`¬Bad_i`, `¬OK_i`, `¬NR_i`)과 진행 결과(`stall-witness` / `no-stall-within(b)` / `unknown(reason)`)는 별개의 축이다. 인증서는 진행성에 대해 아무 말도 하지 않고 기본값은 `unknown(not_checked)`이다 (PI 결정 D20 = A).
- **이번 연결**: 두 축이 각자의 증명 상태를 찍는다. 안전성 진술은 L-PR 등에 따라 `unproved`, 진행 결과는 L-DPOR·L-DEAD (a)·L-MON에 따라 `unproved`이다.

### 6.2 LP-TW: 유한 실행 접두사 포함 (maximal 포함은 아님)

SPEC A.8 LP-TW (a)를 손으로 적용한다 (기대값 [I]).

```
W: OS_BinSemTimedWait(s, t)  — s는 아무도 주지 않는다. 그 뒤 loc2를 사용 (loc2는 INIT에서 VALID, 두 결과 모두에서 사용).
```

| 상태 | σ_U에서 W의 활성 전이 | σ_U∞에서 W의 활성 전이 |
|---|---|---|
| W가 대기 중 | timeout 전이 (시간 기반) | 없음 (timeout이 제거되었고 s를 주는 행위자가 없음) |
| 결과 | W는 죽지 않았다. timeout 후 `loc2` 사용 → `READY` | W는 죽었다(dead). 사용 전 정체 |

- **이전**: `Exec(B, σ_U∞, M) ⊆ Exec(B, σ_U, M)`(maximal 실행의 포함)을 주장. LP-TW에서 σ_U∞의 maximal 실행(W가 죽은 채 끝남)은 σ_U에서 maximal이 아니다 (σ_U는 timeout을 켠다). 주장은 거짓이었다.
- **새**: 유한 실행의 접두사 포함만 주장한다: `Pref(σ_U∞, M) = {σ_U의 유한 실행 중 시간 기반 전이를 포함하지 않는 것}`. maximal 포함은 주장하지 않으며 §6.4 항목 27에 적었다. 정체 증인은 σ 사이에 옮기지 않는다 (교착 순환·잠금 누수 예외만).
- **증명 상태 연결**: 이 포함(L-CONT (a))은 §1의 정의만으로 증명된다 → `proved`. 정체가 σ에 상대적이라는 진술(L-DEAD (b))도 위 표 자체가 증명이다 → `proved`. 이 둘을 쓰는 L-TRANS는 L-STATUS와 L-CONT (b)·(b′)에 기대어 `unproved`이다.
- **기대 출력의 일치** (SPEC A.8): σ_U는 `protected`, 도달 `model(b)`, 진행 `no-stall-within(b)`, `timeout-rescued`와 timeout-masked-liveness. σ_U∞는 b = ∞에서 `protected`(규칙 1(a)로 옮겨 옴), 한정 b 안에서 `no-final-status`, 도달 `unreached(b)`, 진행 `stall-witness({pre-use})`. 두 셀이 서로 모순하지 않는다.

---

## 7. 이번 단계에서 건드리지 않은 것 (다음 단계가 처리)

| 항목 | 이유와 위치 |
|---|---|
| 한국어 미러 `Architecture/01_overview.md`–`10_decisions_adr.md`, `PI_DECISIONS.md`의 같은 문장 | 미러 단계. REVIEW_RESPONSE §5 항목 1의 미러 부분과 항목 2–5, 9–11은 미러·`PI_DECISIONS.md`의 불일치이다. 이번에는 SPEC 쪽 항목(7, 8, 12와 항목 1의 SPEC 부분)만 처리했다 |
| T1 이름 변경 | Spec-C 단계 |
| ADR-039·§10.1에서 D20을 '제안'으로 쓴 곳, 2026-10-15의 다른 결정(P-R2CTRL = c2, D13, P-C2, P-C1HOLD 등)의 반영 | 결정 반영 단계. 이번 정정은 SPEC §0.9에서 이를 명시했다 |
| SPEC에 남은 CORD 기록 인용(`[R2c …]` 인용 15개 줄, 버전 테이블이 없을 때의 '11'이라는 수치, §9.3 S1 게이트와 A.1 step 12 포함) | CORD 정리 단계. 이 문서는 그 수치를 쓰지 않았다. 이번 단계에서는 R2 원장의 두 인용(A.1 step 1, 9a (ii))만 gdb 문서의 §6으로 바꿨다 |
| `results/R2_gdb_v701.md`의 '손실'·시스템 호출 문장 | 위 §5.3 |
| REVIEW_RESPONSE §5 항목 6(`background/07_env_feasibility.md` 머리글의 임시 디렉터리 경로 인용)과 항목 13(`STATUS_20261012.md`의 '독립 검증' 표현, 과거 기록) | 각 문서를 맡은 단계. 이번 단계의 소유 파일이 아니다 |
| 보조정리의 실제 증명 작성, 독립 검토 | V0-P. 이번에는 상태 분류만 했다 |

## 8. 근거와 참고

- SPEC: `Architecture/spec/SPEC_FINAL.md` §0.9, §1.8, §1.10, §2.3, §4.5, §4.5.1, §6.1–§6.4, §8.3, §9.3, §10.1, ADR-044, ADR-045, App. A.1, A.4, A.7, A.8.
- 검토 처리표: `Architecture/spec/REVIEW_RESPONSE_20261012.md` §5.
- 결정: `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` §2.1, §3.2, §5, §7. 로드맵: `plan/ROADMAP_20261015.md` §3, §12.
- C1 사례 기록: `cases/C1.md` F6, F9. 공개 BP 파일: nasa/bp `b051eb0` (`cfe_es_startup.scr`, `tx/tables/cf_def_config.c`), 다시 읽음.
- R2: `results/R2_gdb_v701.md` §2, §6, §7. cFE `c5fb2b4d` `cfe_evs.c`, OSAL `d2d877a` `osapi-countsem.c`: 고정 소스에서 다시 읽음.
- 조건 설정 원칙: `CONDITIONS_POLICY.md`. 이 문서의 어떤 값도 임의로 정하지 않았다. 숫자는 SPEC 문안에서 센 항목 수와 레퍼런스의 상수뿐이다.

# Process-Level 기법 × 사례 대응표 — 여덟 도구의 근거 접근, 여섯 질문, 결과 상태, 부족 가설, RacePro 사건 대응 (2026-10-15 작성, 2026-10-16 갱신)

- 위치와 역할: `plan/process_race_method_matrix.md`. [ROADMAP_20261015.md](ROADMAP_20261015.md) §12 작업 6의 산출물이다. §10.2의 여섯 분석 질문, §10.3의 RacePro 첫 산출물, §10.4의 결과 상태, §10.5의 Kairos 기능 후보 연결을 한 문서로 모은다.
- 입력: `plan/methods/` 아래 도구별 분석 파일 여덟 개(RacePro, SimRacer, SIMEXPLORER, RRF, DESCRY, SimEvo, ReDPro, SysPro). **2026-10-16판은 2026-10-16 UTC 00:00–00:40에 보완된 도구별 파일을 다시 합성했다.** 읽은 판본은 §8.1에 크기와 해시로 적었다. 로드맵 §1.4·§3·§10·§12, [PI_DECISIONS_RESOLVED_20261015.md](../Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md), [CONDITIONS_POLICY.md](../CONDITIONS_POLICY.md), `baselines/availability.md` §5(결과 라벨의 정의)를 함께 읽었다.
- 성격: **합성 문서**다. 도구별 파일의 내용을 한 곳에 모으고 대조했다. 새 논문 조사, 도구 실행, 설치, 빌드, 저자 연락, 코드 변경을 하지 않았다. 도구별 파일에 없는 사실을 더하지 않았다. 칸의 문장은 해당 도구별 파일이 쓴 강도를 넘지 않는다. 이 문서의 [M]은 없다. 도구별 파일이 인용한 [M]은 그 파일의 작성 시점(2026-10-15 UTC 08:12–09:50, 보완 2026-10-16 UTC 00:00–01:10) 값이다. OSAL 자체 시험의 strace 기록은 `[M-probe 기록]`으로 구분해 인용하며 이 문서에서 다시 측정하지 않았다(§0.1). 도구별 파일끼리 어긋난 곳은 저장소 기록을 다시 읽어 대조한 경우에만 어느 쪽을 따를지 적었다(§2.5, §7). 2026-10-15 검토 지적 9건의 처리는 §9, 2026-10-16 갱신 내역은 §10에 있다.
- 용어(T1 = (a), 새 문서이므로 처음부터 적용): 판정 대상은 **준비 상태 명세(readiness specification)**, 전제는 **준비 상태 전제(readiness precondition)**, 위반은 **준비 상태 위반(readiness violation)** 이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다. 원출처의 "race", "failure", "order violation"은 원문 용어 그대로 쓰고 도구 class로 바꾸지 않는다(P-CLASSGT [D]).
- 규칙의 출처: 조건·변수·재현 조건을 임의로 정하지 않는다(CONDITIONS_POLICY). 레퍼런스가 없는 값은 `unspecified_by_reference`이다. 이 문서는 N, 임계값, 예산, 노력 수치를 새로 정하지 않는다. 폐기된 기록기의 결과는 어떤 근거로도 쓰지 않는다(D12 [D]). R2의 실행 근거는 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`)뿐이다. 이 gdb 기록은 breakpoint 기록이며 syscall을 관측한 기록이 아니다(§0.1).

## 0. 읽는 법과 결과 요약

### 0.1 태그와 라벨

| 표기 | 뜻 |
|---|---|
| [F] | 인용이 있는 사실. 원출처(논문 쪽·절, 코드 파일·줄) 또는 저장소 기록. 기록은 "기록"을 붙이며 이 문서에서 다시 확인하지 않았다 |
| [M] | 측정. 이 문서에는 없다. 도구별 파일이 쓴 [M]을 인용할 때는 "도구별 파일 [M]"로 쓴다 |
| [M-probe 기록] | OSAL 자체 시험(`count-sem-test`, `osal-core-test`)을 `strace`로 본 저장소 기록(`baselines/availability.md` §4.2, 2026-10-08). 조건: **OSAL `d2d877a`(v7.0.1)**, 경합 없는 실행, 도구 실행 아님, cFS 실행 아님, 사례 revision 아님. 이 문서에서 다시 측정하지 않았다 |
| [I 소스] | 소스 수준 추론. 코드를 읽고 도출했으며 실행하지 않았다(`availability.md` §4.3, §4.5의 "source-level statements"에 해당) |
| [I] | 추론·분석. 읽은 설계 기술에서 도출한 해석이다. 도구를 실행하지 않았다 |
| [D] | 결정. PI 결정(2026-10-15)이며 사실·실험 결과가 아니다 |
| [U] | 미확인 |
| [제안] | 앞으로 할 작업·순서. 이미 했다는 뜻이 아니다 |

근거 등급(도구별 파일의 용어): `전문`(본문 전체를 읽음), `초록`(초록·서지만), `2차`(다른 문헌이 그 도구를 서술한 문장), `기록`(저장소의 이전 보고서 값), `미도달`(열지 못함), `코드 직접 읽음`.

**syscall 흔적("syscall 없음", "0건", "syscall이 아님")을 쓰는 칸의 표기 규칙.** 이 문서에서 syscall이 보이지 않는다는 문장에는 아래 둘 중 하나의 근거 표기를 붙인다. 표기 없는 문장은 쓰지 않는다. 두 표기 모두 사례 revision에서 syscall을 관측한 결과가 아니다.

- `[M-probe 기록]`은 네 가지에만 쓴다. (1) 이름 있는 count semaphore의 생성·조회(C1 생성·조회에 해당하는 `OS_CountSemCreate`, `OS_CountSemGetIdByName`)가 OSAL의 `count-sem-test`에서 syscall 0건이었다. (2) `osal-core-test`에서 `OS_QueueCreate`가 `mq_open`을 69회 냈다(`OS_QueueGetIdByName`·`OS_TaskGetIdByName`은 0건). (3) 같은 시험에서 새 task가 아직 RESERVED인 자기 레코드를 만나 기다린 1회가 task table lock·condvar 주소의 futex 대기로만 보였다(이름·레코드 내용 없음). (4) `count-sem-test`의 stack 귀속 표에서 `OS_TaskCreate` 아래 `clone3`가 run마다 4회 보였고 `clone3`는 flags·stack·TLS만 싣고 이름은 싣지 않았다(`baselines/osal-visibility.md` "Stack attribution" 표의 `OS_TaskCreate` 행. DESCRY.md 표기 규칙 (3)이 이 기록을 쓴다. 2026-10-16 추가, §2.5 항목 9). 네 결과 모두 OSAL `d2d877a`(v7.0.1), cFS 밖, 도구 실행 아님이고, (1)(2)는 경합 없는 실행이다. 경합이 있으면 lock 주소에 `FUTEX_LOCK_PI`와 condvar 대기가 보일 수 있다(`availability.md` §4.5). 지지하는 문장은 "OSAL `d2d877a`의 자체 시험에서 그 호출들은 경합 없이 syscall을 내지 않았다"까지이며 "도구 X가 사례 Y를 놓친다"는 지지하지 않는다(`availability.md` §4.5, §5.4).
- **revision 불일치.** 사례의 OSAL은 C1이 `42af0f73`(v6.0.0-rc4, `availability.md` A12, `osal-visibility.md` D4), C3·C5가 OSAL 4.1.1 시대(6.4.1·6.4.2 릴리스 구성의 OSAL이며 원 관측자의 OSAL은 `unspecified_by_reference`, `cases/C3.md`, `cases/C5.md`)이다. probe는 그 revision에서 실행되지 않았다. R2의 사례 revision은 B `fc8c48ad`, M `12cb84fc`, D `f22e538c`이고 cFS에서 strace를 돌린 기록은 없다(`osal-visibility.md` §0 D1 미결).
- `[I 소스]`는 R2·C3·C5의 AppId 저장·등록 쓰기·조회·반환값처럼 그 밖의 모든 "syscall 없음"에 쓴다. syscall 가시성에 대해 읽은 소스는 번들(cFE `c5fb2b4d`, PSP `c4b3b0b6`, OSAL `d2d877a`)이고 C3·C5의 역사 판본은 읽지 않았다(`osal-visibility.md` §8).
- **gdb 기록은 syscall 근거가 아니다.** R2의 `env/logs/r2_g1..3_*`는 breakpoint 기록이다(이 파일들에 `syscall`·`strace` 문자열이 없음을 작성 중 확인. DESCRY.md §1.3 행 13도 2026-10-16에 같은 grep 결과를 적는다). R2의 "syscall 없음"은 소스 읽기(`cases/mapping_v701.md` §1.2)이며 gdb 기록으로 뒷받침하지 않는다(로드맵 §3 "R2 결과 표현의 과장" 행: 코드 분석과 실제 관측을 구분하고, 관측하지 않은 syscall 전체의 부재를 gdb 로그만으로 주장하지 않는다). `results/R2_gdb_v701.md` §2의 "no syscall was made" 문장도 이 문서는 근거로 쓰지 않는다(§5.5). RRF.md가 2026-10-16에 그 문장을 인용했으나 이 문서는 해당 칸을 `[I 소스]`로 옮긴다(§2.5 항목 10).
- **gdb backtrace frame은 syscall 관측이 아니다.** DESCRY.md는 gdb 위반 스택 24개 중 10개의 가장 바깥 frame이 `start_thread`·`clone3`라고 2026-10-16에 셌다 [도구별 파일 M]. 이것은 task가 thread로 만들어졌다는 stack 모양이며 syscall 추적 기록이 아니다.

질문 라벨(도구별 파일과 같다): **답**은 출처의 쪽·절로 답한 것이다. **분석**은 읽은 설계 기술에서 도출했고 실행하지 않은 것이다(analysis from stated design, not run). **불명**은 읽은 자료로 정해지지 않는 것이다. 라벨은 증거의 종류이며 도구의 능력 판정이 아니다. "분석"을 "못 한다"로 읽지 않는다.

결과 상태(로드맵 §10.4): `NOT_RUN`, `INAPPLICABLE`, `PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`.

질문 번호(로드맵 §10.2): Q1 관측 사건 수집, Q2 이름·ID·필드가 같은 자원·세대를 가리킴을 식별, Q3 생성·등록·초기화 완료와 의존 사용의 관계 표현, Q4 필요한 순서·허용된 오류 처리 정보를 입력받는가 추론하는가, Q5 아직 실행되지 않은 순서의 예측·탐색과 실행 확인, Q6 출력의 종류와 원 사례 현상과의 대응. 한 칸에 관측 가능성, 표현 가능성, 실제 탐지 성공을 합치지 않는다.

### 0.2 결과 요약

1. **다섯 사례 모두에서 여덟 도구의 오늘 상태는 `NOT_RUN`이다(40칸).** 이 수는 도구별 파일이 `NOT_RUN`을 적용한 상태의 수이며, PI 판단 두 가지가 열려 있다. §5.4 항목 1(같은 프로세스 안 task 쌍)과 항목 14(C2의 입력 미충족)다. 항목 14를 채택하면 DESCRY·SysPro·RRF·SimEvo의 C2 최대 4칸이 `INAPPLICABLE`로 바뀐다(DESCRY·SysPro는 파일이 안 (c)로 명시했고 RRF·SimEvo는 같은 사유만 적었다). 어느 칸도 "도구가 사례를 탐지·재현하지 못한다"는 결과가 아니다. 미탐지·미재현 수에 넣지 않는다. 차단 요인은 도구마다 다르다. RacePro는 코드가 공개이고 차단이 환경(i386 Linux 2.6.35 + Scribe 내장 커널을 이 컨테이너에서 부팅할 수 없음)이다. 나머지 일곱은 실행할 구현을 접근 가능한 출처에서 찾지 못했다(§1.2, §3.2).
2. **근거 등급이 도구마다 크게 다르다(§1.1).** 논문 본문 전문을 읽은 것은 RacePro, RRF, DESCRY, SimEvo, SysPro(arXiv v1), SIMEXPLORER(STVR 저자 수락본)이다. SimRacer(ISSTA'13) 본문은 2026-10-16 재시도에서도 열지 못했다. SimRacer 칸은 확장판인 STVR 2017 전문과 같은 해 같은 저자의 Sim-O/C(ITJ 2013, `2차`)에 기대며 일치 범위가 제한된다. ReDPro는 전문을 읽지 못해 초록·서지·인용 문장뿐이고 30칸 중 26칸이 불명이다(Internet Archive 스냅숏 하나가 남은 단서이나 이 세션의 egress 정책이 막는다). SIMEXPLORER(STVR)와 RRF의 쪽 번호 인용은 CPS-VO 사본 한 경로에서 얻었고, 2026-10-16에 SimRacer·SIMEXPLORER·RRF 파일이 둘째 경로를 찾았으나 얻지 못했다(§1.1 취득 경로).
3. **라벨은 질문 수준을 나눠 센다(§2.3).** 도구 수준 질문 Q4(필요한 순서·허용 오류를 입력받는가)는 사례와 무관한 한 번의 답이므로 도구당 1건으로 센다. 답 7건(RacePro, SimRacer, SIMEXPLORER, RRF, DESCRY, SimEvo, SysPro. SimRacer·SIMEXPLORER가 같은 STVR 근거를 쓰므로 한 계보로 묶으면 6건)과 불명 1건(ReDPro)이다. Q4를 뺀 사례별 칸은 8도구 × 5사례 × 5질문 = 200칸이며 **분석 132, 불명 68, 답 0**이다(2026-10-15판은 140/60). 바뀐 것은 SimRacer.md가 C1·C3·C5·R2의 Q1·Q2 8칸을 "분석"에서 "불명"으로 바꾼 것 하나다. SimRacer 자체가 메모리를 SR로 보았는지부터 정해지지 않기 때문이다(SimRacer.md §1.3 항목 8–10). SimRacer·SIMEXPLORER의 Q3·Q5·Q6 칸은 여전히 같은 STVR 근거에서 나온 것이라 독립 칸이 아니다. 이 수는 점수가 아니다. 증거의 종류를 센 것이다.
4. **이전 저장소 서술 하나는 SIMEXPLORER에 쓸 수 없고 SimRacer에는 단정할 수 없다.** "syscall 모델이라 다섯 사례의 의존성은 shared-resource 사건이 아니다"는 SIMEXPLORER에는 쓸 수 없다. STVR 전문은 shared memory를 shared resource로 열거하고 메모리 접근을 Simics 중단점으로 잡는다(STVR p.8, p.14). SimRacer(ISSTA'13)에는 이 문장을 쓸 수도 반박할 수도 없다. 본문을 읽지 못했고, STVR p.8은 메모리 관측의 선행 연구로 SimRacer([16])와 SimTester([33])를 함께 인용해 SimRacer 자체에 있었는지 정하지 않는다. 같은 해 같은 저자의 Sim-O/C(ITJ p.188 `2차`)는 여러 user process 사이의 시험을 "system calls that can potentially race with each other"의 순서 바꾸기로 서술하고, SCMiner p.11 `2차`도 system call interleaving으로 서술한다. 그러나 Sim-O/C는 메모리 관측을 배제하는 문장을 갖지 않고 메모리 중단점의 예는 응용과 ISR 사이(ITJ p.185–187, VEE'12 p.54 `2차`)에서만 나온다(§2.5 항목 7). 남는 질문은 cFS 내부 상태를 그 SR로 삼는 경로와 입도이며, 서술이 없다. DESCRY와 SimEvo는 사건·자원 모델을 RacePro·SimRacer에 위임하므로 "서술된 사건 집합에 없다"로 한정해야 하고 구현이 메모리를 보는지는 불명이다(§2.5, §4.2).
5. **로드맵 §10.5의 "관측된 실행만 다룸 → 예측·탐색 및 실행 확인" 행은 여덟 도구 어디에도 근거가 없다(§4.2).** 관측되지 않은 순서의 예측과 실행 확인은 RacePro, SimRacer 계열, DESCRY, SimEvo에 이미 있다. RRF는 HB 예측이 아니라 관련 syscall 쌍의 열거와 sleep 강제 후 실행 확인이고, SysPro는 예측이 아니라 순위 탐색과 재실행 확인이다. ReDPro 초록에도 순서 제어와 재생성 단계가 서술돼 있다. 이 행을 차별점으로 쓰지 않는다.
6. **"확인된 부족"은 오늘 0건이다(§4.1).** 확인된 부족은 `OBSERVATION_GAP`·`ANALYSIS_GAP`처럼 도구를 실행한 뒤의 상태로 정의한다. 그래서 로드맵 §10.5의 여섯 행 모두를 비교 결과로 주장하지 않는다. §4.2는 도구별 파일이 올린 가설과 그 강도를 "후보 연결"로만 옮겼다.
7. **RacePro 사건 대응(§6).** R2의 네 사건(AppId 설정, 값 전달, 이벤트 요청, 오류 반환)과 2026-10-16에 더한 ⓪ core task 생성을 RacePro의 기록·micro-op·HB·후보 클래스에 대었다. AppId 설정, 값 전달, 오류 반환 셋은 읽은 설계 기술과 코드 안에서 대응 경로를 찾지 못했다. 이벤트 요청은 2026-10-15판의 "불명"에서 바뀌었다. RacePro.md가 Scribe 커널 코드를 읽어 `mq_open`의 fd 할당·설치가 FILES_STRUCT 쓰기가 되고 큐 이름·큐 객체에는 Scribe 자원 잠금이 없다는 것을 정리했다(RacePro.md §2.5 K1–K4, 코드 직접 읽음, 미실행). 그래서 ③은 RESOURCE 후보 쌍이 될 여지가 있으나 그 쌍은 **fd table 접근 순서**이며 AppId 설정·사용이 아니다. ⓪은 `clone`이 HB 간선의 원천이 되는 부분 대응이고, RUNNING 폴링은 공유 메모리 의존이라 간선이 되지 않는다. 코드의 네 후보 클래스(RESOURCE, EXIT-WAIT, SIGNAL, TOCTOU) 중 ①②④에 적용되는 것은 없다. 또 논문의 보고 조건이 "녹화 때 없던 실패"(p.363)라서, gdb 3회 모두 나타난 R2 현상은 RacePro 녹화에도 들어 있다면 branch의 새 실패로 보고되는 경우가 아니라는 분석(H-RP-5)이 더해졌다. C1은 생성·조회·재시도 대기 모두 대응 경로를 찾지 못했다. C3은 대응 경로 없음 셋과 조건부 분석 하나(TIME의 `CreatePipe`가 `mq_open`이라면 R2 ③과 같음. C3의 OSAL 4.1.1 시대 queue 구현은 확인하지 않음)이다. C5는 `clone`만 HB 노드·간선에 닿는 부분 대응이고 나머지는 경로가 없다. C2는 공개된 조건(시작 동기화 polling과 상태 전이)만 쓰며 대응 경로를 찾지 못했다. 이것은 설계 기술과 코드 읽기의 분석이며 RacePro 실행 결과가 아니다. "커널에서 보이는 흔적" 열의 "syscall 없음"은 칸마다 `[M-probe 기록]`(OSAL `d2d877a`의 경합 없는 자체 시험, 사례 revision 아님) 또는 `[I 소스]`(소스 읽기, 미실행)를 붙였고, R2·C3·C5의 의존 상태 접근은 `[I 소스]`뿐이다(§0.1, §6). C5의 조회는 ES lock과 겹치면 lock 주소의 futex가 보일 수 있어 "0 syscalls"로 쓰지 않는다(§6.5).
8. **결정이 필요한 항목은 §5.4에 모았다.** 저자 연락 정책은 PI 사안이므로 목록만 적었고 연락하지 않았다. 2026-10-16에 공개 보관본(Internet Archive) 사용 여부(항목 17)를 더했다.
9. **도구별 파일끼리 어긋난 사실 기술이 둘 있다(§2.5 항목 9·10, §7).** (a) `OS_TaskCreate` → `clone3`의 근거: DESCRY.md는 OSAL 자체 시험(`[M-probe 기록]`)으로, SysPro.md·RacePro.md·SimRacer.md는 에이전트가 쓴 작은 C 프로그램의 기록으로 적는다. 저장소 기록을 다시 읽으니 둘 다 있다(`osal-visibility.md`의 stack 귀속 표, `availability.md` §4.5 끝). 이 문서는 OSAL 시험의 기록을 `[M-probe 기록]` (4)로 쓴다. (b) RRF.md는 R2 ④ 칸에 `results/R2_gdb_v701.md`의 "no syscall was made"를 인용했다. 이 문서는 그 문장을 근거로 쓰지 않는다(§0.1).

### 0.3 공통 전제: 실행 단위의 정의 차이

[D] 이 연구의 Process-Level은 cFS 애플리케이션·task를 작업 단위로 쓰는 운용상 정의이며 OS 프로세스로 한정하지 않는다(로드맵 §1.2). [F 기록] native cFS는 한 Linux 프로세스이고 OSAL task는 `pthread_create` thread이다. 여덟 도구의 실행 단위는 아래와 같다. 이 차이는 비교 논문에서 처음에 밝히고, 이 차이로 생기는 결과를 도구의 결함처럼 쓰지 않는다. 같은 프로세스 안의 task 쌍을 도구의 입력 단위로 줄 수 있는지는 대부분 서술이 없다.

| 도구 | 실행 단위 서술(도구별 파일의 인용) |
|---|---|
| RacePro | 프로세스와 thread를 기록하나, 같은 프로세스의 thread 사이 OS 자원 race에는 "less effective"(SOSP p.362) |
| SimRacer / SIMEXPLORER | 사건 원천 4종: 사용자 프로세스, software signal·handler, hardware interrupt·ISR, kernel thread. thread는 PID를 가진 lightweight process이므로 "just like a process" 추적 가능하다는 한 줄(STVR p.7, p.13). 입력은 PuT와 짝 프로그램·handler(p.16). STVR 서론은 기존 기법 대부분이 "multithreaded applications within single processes"에 맞춰져 있다고 적고(p.1) thread-level 기법을 process-level race에 바로 쓸 수 없는 이유를 든다(p.4–5). 한 process 안의 thread 사이 race를 범위에서 뺀다는 문장은 찾지 못했다(SIMEXPLORER.md §1.3 항목 11). Sim-O/C(ITJ p.185 `2차`)는 Simics 콜백만으로는 process와 thread를 구별하지 못하고 process tracker가 user-space process를 구별한다고 적는다(SimRacer.md §2.3) |
| RRF | OS 프로세스 쌍(PuD). process-level race를 "multiple processes access operating system resources"로 정의하고 "operating at the thread level"인 탐지기가 이런 race를 다루지 못할 수 있다고 적는다(p.1). 같은 프로세스 안 thread를 단위로 다루는 서술 없음, 범위에서 뺀다는 명시 문장도 찾지 못함(p.3, p.6–7, p.10. RRF.md §1.3 항목 8) |
| DESCRY | OS 프로세스와 software signal. thread-level은 초점 밖(p.1), 프로세스당 thread 하나를 설명 편의로 가정(p.3) |
| SimEvo | OS 프로세스(PuT)와 signal·interrupt handler. thread-level은 대비 대상(p.1, p.7) |
| ReDPro | 초록의 "multiple processes". 같은 프로세스의 thread 사이를 포함하는지는 [U] |
| SysPro | OS 프로세스와 signal. thread-level은 구분(p.2–3). 입력에 "프로세스 집합"이 있음(p.4–5, p.32) |

## 1. 근거 접근표

### 1.1 도구 × 근거 등급

"본문"은 그 도구를 소개한 논문의 본문이다. 쪽 번호의 기준 판본을 함께 적었다. 해시는 도구별 파일이 적은 sha256의 앞부분이다.

| 도구 (논문) | 역할(도구별 파일) | 본문 등급 | 읽은 판본·쪽 기준 | 보조 출처와 그 등급 | 열지 못한 것 |
|---|---|---|---|---|---|
| RacePro (SOSP 2011) | 탐지(후보를 재실행으로 검증) | **전문** | 저자 호스팅 PDF 15쪽, sha256 `645a21ba…`. 세 호스트가 같은 파일(2026-10-16 재확인, 인용 10개 재대조). SOSP 인쇄 쪽 353–367 기준 | HotOS'11 예비 설계 전문(SOSP와 다르면 SOSP를 따름). 저자 슬라이드 `2차`. **코드 직접 읽음**: `racepro` @`681f94e3`의 일부 파일(`racecore.py`, `syscalls.py`, `toctou.py`, `session.py` 등), 커널 @`b29e9afb`의 sparse checkout(`ipc/`, `scribe/`, `fs/` 일부, `kernel/fork.c`, `include/linux/scribe*.h`·`file.h`, 2026-10-16), py-scribe @`56d6187e`(2026-10-16), 번들 `kernel_config`·`deploy.rb`. 커널 diff 목록 등은 `기록` 재사용 | ACM DL 403. `racepro-bundle`·`libscribe-racepro`는 HEAD 확인과 개별 파일만. 커널 트리 전체는 받지 않음 |
| SimRacer (ISSTA 2013) | 탐지(testing-based) | **초록** (서지·초록만). 본문 `미도달`(2026-10-16에 14개 경로를 더 시도) | 본문 없음. 도구 서술은 확장판 STVR 2017 `전문`에 기대고(CPS-VO 사본, 단일 취득 경로 — 아래 취득 경로 단락), 일치 범위는 SimRacer.md §2.3에 항목별로 구분됨. 2026-10-16부터 사례별 Q1·Q2 라벨은 SimRacer 자체에 대해 매기고 STVR 설계 기준 분석은 라벨 없이 병기 | STVR 2017 `전문`(저자가 SimRacer의 "revised and expanded version"이라 밝힘, STVR p.2 각주·p.24). Sim-O/C(ITJ 17(2) 2013, pp.178–197, sha256 `24080f8e…`) 전문 `2차`, SimTester(VEE'12) 관측기 부분 `2차`(2026-10-16 추가). SimEvo·DESCRY·RRF·SysPro·SCMiner의 SimRacer 서술 `2차` | ACM·ResearchGate·Wiley(am-pdf 포함) 403, Wiley TDM API는 토큰이 없어 400(토큰을 만들지 않음), 학위논문(Yu 2014) PDF Cloudflare. 저자 공개본·기관 저장소·arXiv·NSF PAR·Unpaywall·OpenAlex 사본 없음. Yu의 2016년 UNL 홈 Wayback 스냅숏은 이 세션에서 열리지 않음 |
| SIMEXPLORER (STVR 2017) | 탐지(testing-based) | **전문** (저자 수락본) | CPS-VO 레코드에 첨부된 26쪽 PDF `Yu_SimExplorer_JA.pdf`, sha256 `af805231…`. 수락본 쪽 기준이며 출판본과의 일치는 미확인. **단일 취득 경로, 독립 재확인 안 됨**(아래 취득 경로 단락. 2026-10-16 재시도에서도 둘째 경로 없음). 재배포 조건은 [U](아래 취득 경로 단락) | Sim-O/C (ITJ 2013) 전문 `2차`. SimTester (VEE'12) 부분 `2차`. SimEvo `2차`. 학위논문 `초록` | Wiley 출판본·am-pdf 403(Cloudflare). 학위논문 PDF. Scholar Gateway 의미 검색 `ACCESS_DENIED`(2026-10-16) |
| RRF (ISSRE 2016) | 재현(보고된 race) | **전문** | CPS-VO 사본 11쪽, sha256 `e8c26e2f…`. IEEE 출판본과의 동일성 미확인. **단일 취득 경로, 독립 재확인 안 됨**(아래 취득 경로 단락. 2026-10-16에 같은 사본의 추출 텍스트로 인용 위치를 다시 찾았고 "reasonable to assume the availability of source code"는 p.5로 고침) | SysPro·DESCRY의 RRF 서술 `2차`. 학위논문(Rattanasuksun, UNL 2018) `초록`. 내부 oracle 논문(ISSRE 2013, RRF의 [36]) `초록`(2026-10-16) | IEEE 출판본(AWS WAF 도전). 학위논문 PDF. [36] 본문(UNL PDF Cloudflare, IEEE WAF, Semantic Scholar `CLOSED`) |
| DESCRY (ESEC/FSE 2017) | 재현(로그 기반) | **전문** | 저자 공개본 11쪽, sha256 `f9d7b57b…`. NSF PAR 사본과 텍스트가 거의 같음(2026-10-16 두 사본 해시 재확인). 쪽은 PDF 쪽 | SysPro·SCMiner·STVR(p.2, p.8, p.18, p.24만, 단일 취득 경로)의 서술 `2차`. Zaman 학위논문(UKy 2022) 기관 레코드의 초록 전체 `초록`(2026-10-16). 프로젝트 페이지는 원 서버 403, Wayback 가용성 API에 스냅숏 없음 | ACM 출판본 Cloudflare. 학위논문 PDF |
| SimEvo (ICSME 2017) | 회귀 시험(변경 영향 범위) | **전문** | PAR 사본 12쪽, sha256 `143f7351…`. 쪽은 PDF 쪽(인쇄 쪽 = N+203). 2026-10-16 핵심 인용(p.3, p.6, p.7, p.8) 재대조, 변경 없음 | 위임 출처인 STVR(단일 취득 경로)·RacePro 일부 `2차`. Sim-O/C ITJ p.188 `2차`(2026-10-16). 피인용 3건의 인용 문맥 `2차`(실행·artifact 언급 없음) | IEEE 출판본. 위임 모델의 원문인 SimRacer ISSTA'13. 학위논문(Yu 2014) PDF 403 |
| ReDPro (ACMSE 2022) | 초록 수준에서 탐지 + 실패 재생성. **역할 표의 확정 행으로 넣지 않음**(로드맵 §10.1) | **미도달** | 본문 없음 | 초록(서지 집계 서비스 사본) `초록`. 서지·참고문헌 24건·프로그램·저자 CV·제1저자 ORCID 작품 목록 `서지`. SysPro p.8·21–23·32·34–35, Gender Dynamics p.3, LLPut p.1의 인용 문장 `2차` | ACM Cloudflare. Unpaywall·OpenAlex·Semantic Scholar·저자 홈·NSF PAR·arXiv·Internet Archive 전문 검색 어디에도 사본 없음. Internet Archive availability API는 ACM PDF 주소의 2022-05-07 스냅숏(기록 상태 200)이 있다고 답했으나 `web.archive.org`는 이 세션의 egress 정책이 막아 내용(PDF인지 도전 페이지인지)을 모름(2026-10-16) |
| SysPro (arXiv 2601.09616v1) | 재현(버그 보고서 기반) | **전문** (arXiv v1 47쪽) | sha256 `8fe5daa4…`(2026-10-16 재다운로드, 같은 해시). HTML·TeX 원본 확인. 쪽은 PDF 쪽 | NSF 과제 2518445(RepSON) 공개 초록 `2차`, 저자 홈페이지·CV `2차`(구현 링크 없음, 2026-10-16) | JSS 학술지판 본문(ScienceDirect 403. NSF PAR 레코드는 본문을 2027-06-01에 공개한다고 적음). 저자 목록이 arXiv v1과 다르므로 내용이 같다고 보지 않음 |

**CPS-VO 사본의 취득 경로.** SIMEXPLORER(STVR, `archive.cps-vo.org/file/36388/download/194033`)와 RRF(`archive.cps-vo.org/file/30362/download/194493`)의 전문은 같은 호스트에서 받았다. 이 호스트의 WAF는 기본 `curl`에 245 B "Request Rejected"를 돌려줬다(SIMEXPLORER.md 경로 13, SimRacer.md 경로 18, RRF.md 경로 1). STVR PDF는 SIMEXPLORER.md에서는 클라이언트 문자열을 `Mozilla/5.0 (X11; Linux x86_64) research-fetch`로 바꿔 요청해 받았고(경로 14, 로그인·쿠키·Referer 없음), SimRacer.md·SimEvo.md·DESCRY.md에서는 WebFetch 도구로 받았다. RRF PDF는 WebFetch 도구로 받았고 파일 URL을 `curl`로 직접 시도하지는 않았다(RRF.md 경로 2). 네 파일이 STVR 사본에 같은 sha256 `af805231…`를 적었으므로 바이트는 일치하지만, 취득 방식은 모두 "WAF 거부 뒤 다른 클라이언트·도구로 재요청"이라는 같은 종류이고 출판본과의 일치는 미확인이다. SimRacer.md는 2026-10-16에 두 번 받은 파일의 해시가 같아도 같은 호스트의 같은 파일이므로 **독립된 둘째 취득 경로가 아니라고** 적는다. 그래서 STVR 기반 쪽 번호 인용(STVR p.8, p.14 등. SimRacer, SIMEXPLORER, DESCRY, SimEvo 칸에 쓰임)과 RRF 쪽 번호 인용(RRF p.1–10)은 **단일 취득 경로, 독립 재확인 안 됨**으로 표시한다. 2026-10-16에 SimRacer.md(Unpaywall, OpenAlex, Semantic Scholar, Crossref, Wiley am-pdf·TDM), SIMEXPLORER.md(Wiley, UNL, Scholar Gateway), RRF.md(IEEE, Semantic Scholar, OpenAlex, Scholar Gateway, 저자 홈 두 곳, CV, GitHub)가 둘째 경로를 찾았으나 얻지 못했다. 이 문서는 두 파일을 다시 열지 않았다. 표시는 둘째 경로가 확보되거나 PI가 확인할 때까지 유지한다(§5.1 A10, §5.4 항목 16). §5.1 A5가 우회하지 않는다고 적은 것은 Cloudflare·AWS의 도전 페이지이고 이 WAF 거부는 그와 다른 종류의 응답이다. 다만 클라이언트 문자열을 바꾼 재요청이 P-PAPERS [D]의 취지에 맞는지는 PI가 확인할 사항이다. ReDPro.md는 2026-10-16 재시도에서 일부 curl 호출에 브라우저형 클라이언트 문자열을 처음부터 붙였고 거부된 뒤에 바꾼 일은 없다고 적는다.

**CPS-VO 사본의 라이선스 표기(2026-10-16).** SIMEXPLORER.md는 2026-10-15판의 "Creative Commons 2.5" 표기를 지웠다. 저장한 **레코드 페이지**(`node/36387`) 텍스트에서 그 문구를 찾지 못했기 때문이다. SimEvo.md는 **파일 페이지**(`node/36388`)를 다시 받아 "License: Creative Commons 2.5"를 확인했다고 적는다 [도구별 파일 M]. 두 파일은 서로 다른 페이지를 본 것이다. 이 표기가 PDF의 재배포 조건을 확정하는지는 어느 파일도 판단하지 않았다. 어느 파일도 PDF를 저장소에 넣지 않았고 쪽 번호와 짧은 인용만 쓴다.

사례 이슈 댓글은 여덟 파일 어디에서도 읽지 못했다(`nasa/CF`, `nasa/cFE` 접근이 세션에서 막힘). 2026-10-16에 SysPro.md가 GitHub MCP `issue_read`를 시도했고, 응답은 저장소가 세션에 구성되지 않았다며 사용자가 요청하지 않은 저장소를 추가하지 말라고 안내했다. 저장소를 추가하지 않았다. R2 댓글 2건은 저장소 기록에 검색 snippet 조각으로만 남아 있고 그 조각에 로그가 없다(DESCRY.md §1.3 행 14). DESCRY의 입력 조건(실패 로그)과 SysPro의 입력 범위 판단이 이 경로에 걸려 있다.

### 1.2 구현·실행 환경 접근

| 도구 | 구현·artifact | 실행에 필요한 것(도구별 파일) | 오늘 이 컨테이너(도구별 파일 [M]) | 차단 요인 분류 |
|---|---|---|---|---|
| RacePro | **공개**. `columbia/racepro` 외 4개 저장소. 고정 커밋은 RacePro.md §7 | i386 Linux 2.6.35 + Scribe 내장 커널(번들 `kernel_config`), Python 2, Cython ≥ 0.13, networkx, pygraphviz, replay용 파일 시스템 체크포인트. userland 배포판은 `unspecified_by_reference` | 커널 6.18.44-fc-v114(2026-10-15), `fc-v139`(2026-10-16T00:07Z). 두 시각 모두 `CONFIG_KEXEC`·`CONFIG_KEXEC_FILE`·`CONFIG_MODULES` 없음, `/dev/kvm` 없음, vmx/svm 0, `/boot` 비어 있음. python2·cython·qemu-system-i386 없음 | **환경**(자료는 열려 있음) |
| SimRacer | 찾지 못함. GitHub 저장소 검색 34건은 모두 sim racing 프로젝트, 코드 검색 1건은 BibTeX 색인. 저자 홈·CV에 소프트웨어 항목 없음. 2026-10-16에 `simics race`(32건), Sim-O/C 제어 모듈 이름(`"RaceController" simics`, 0건)으로 다시 찾았으나 없음 | Simics(버전·머신·스크립트 `unspecified_by_reference`), guest(STVR p.18 서술), 제어용 dummy device·driver | Simics 없음, `/dev/kvm` 없음(2026-10-16 재측정 같음) | **자료** + 환경 |
| SIMEXPLORER | 찾지 못함. GitHub 저장소·코드 검색 0건(2026-10-16 재검색: 저장소 0건, 코드 2건은 다른 주제의 문헌 메모), Zenodo·figshare·arXiv·NSF PAR 해당 없음. 객체 프로그램 24개는 first author에게 요청(STVR p.15) | 위와 같음 | 위와 같음 | **자료** + 환경 + 결정(cFS용 SR·oracle 입력) |
| RRF | 찾지 못함. 논문에 위치 서술 없음 | Scribe(패치 Linux 커널, 판본 서술 없음), 정적 분석 도구(이름 없음), 내부 oracle 저장소, 소스 기반 `#ifdef` 빌드 | 커널을 바꿀 수 없음(RacePro와 같음). 정적 분석 도구 선택 불가 | **자료** + 환경 |
| DESCRY | 찾지 못함. 프로젝트 페이지 `cs.uky.edu/~tyu/research/descry`는 원 서버가 403 | LLVM 로그 분석기, KLEE(LLVM 3.4 기반), Simics | `klee`·`llvm-3.4`·`clang-3.4` 패키지 없음(LLVM은 18.1.3), Simics 없음. 실패 로그는 다섯 사례 본문에 없음 | **자료** + 환경 |
| SimEvo | 찾지 못함. **기반인 SimRacer 구현도 찾지 못함** | Simics, CodeSurfer(상용, 현재 가용성 불명), C++ GA | Simics·CodeSurfer 없음. cFS용 테스트 입력 T와 오라클의 레퍼런스가 없음 | **자료** + 환경 + 결정 |
| ReDPro | 찾지 못함 | 전문을 못 읽어 요구 사항 자체가 불명. PIN 버전은 `unspecified_by_reference` | PIN 없음. 이 컨테이너 커널에서 PIN이 동작하는지 미확인 | **자료**(구현·전문·PIN 판본) + 환경 |
| SysPro | ReproSys(`github.com/tarannumzaman/ReproSys`)는 익명 접근에 인증을 요구함. 2026-10-16에 `add_repo`(읽기)가 "not found 또는 접근 권한 없음"을 돌려줌. 저자 홈페이지·CV에도 구현 링크 없음. 존재하지 않는 것과 비공개를 구분할 수 없음 | PIN, srcML, NLTK·Gensim, SPMF와 Java, system call 이름 목록·man 페이지. sleep 길이와 재현 판정 방법은 논문에 서술 없음 | PIN·srcml·nltk·gensim·SPMF 없음, java·strace·gdb 있음(2026-10-16 재측정) | **자료** |

"찾지 못했다"는 접근 가능한 출처에서 찾지 못했다는 뜻이다. 존재하지 않는다는 확인이 아니다. 도구별 파일 여덟 개가 모두 이 구분을 지킨다.

**디스크 여유 공간 (도구별 파일 [M]).** 2026-10-15에는 `/` 여유가 약 16 GB였다. 2026-10-16의 측정은 파일마다 시각이 달라 1.3–2.7 GB였다(RacePro.md 2.7, SIMEXPLORER.md 2.3, SimRacer.md 2.0, RRF.md 1.9, DESCRY.md·SimEvo.md 1.4, ReDPro.md·SysPro.md 1.3 GB). SimRacer.md와 SIMEXPLORER.md가 옮긴 벤더 문서의 Simics 설치 크기(약 10 GB)보다 작다. 이것은 컨테이너 상태의 기록이며 어떤 설치도 하지 않았다. 설치 여부는 PI 결정이다(§5.4 항목 6).

## 2. 도구 × 사례 여섯 질문 행렬

### 2.1 읽는 법

- 아래 라벨과 칸 문장은 도구별 파일의 §3을 옮긴 것이다. 칸 문장은 압축했으나 강도를 올리지 않았다. 원문 근거(쪽·절)는 도구별 파일의 같은 칸에 있다.
- **"분석" 칸의 공통 전제**는 도구마다 도구별 파일이 따로 적었다. 이 문서는 칸마다 붙이지 않고 §2.4의 도구 머리글에 모았다.
- Q4는 도구 수준의 답이 사례와 무관하므로 다섯 사례 모두 같은 라벨이다(ReDPro 제외). 집계에서는 도구당 1건으로 센다(§2.3).
- 같은 STVR 근거를 쓰는 SimRacer와 SIMEXPLORER는 별개 행으로 두고 계보를 표시했다. 2026-10-16판에서 두 행은 Q4를 뺀 50칸 중 44칸이 같은 라벨이고, 다른 6칸(C1·C5의 Q1, C3·R2의 Q1·Q2)은 SimRacer.md가 SimRacer 자체에 대해 "불명"으로, SIMEXPLORER.md가 STVR이 서술하는 도구에 대해 "분석"으로 둔 칸이다(§2.5 항목 1). 같은 44칸 중 Q3·Q5·Q6의 "분석"은 같은 STVR 근거에서 나왔다. §2.3의 합계는 이 두 행을 독립 근거로 세지 않는다고 표시했다. 계보 묶음 여부는 PI 판단 후보다(§5.4의 항목 7).
- SimRacer 행의 Q1·Q2 칸은 SimRacer.md를 따라 두 층으로 적는다. 라벨은 SimRacer(ISSTA'13) 자체에 대한 것이고, 뒤의 "STVR 설계 기준" 문장은 라벨을 정하지 않는 분석이다.
- STVR 기반 쪽 번호 인용(SimRacer, SIMEXPLORER, DESCRY·SimEvo가 기대는 서술)과 RRF 쪽 번호 인용은 단일 취득 경로이며 독립 재확인이 없다(§1.1 취득 경로).

### 2.2 도구 수준의 답 (사례와 무관)

| 도구 | Q1 관측 사건 | Q2 자원·세대 식별 | Q3 생성·등록 vs 의존 사용 | Q4 필요 순서·허용 오류 입력 | Q5 예측·탐색, 확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| RacePro | **답** — syscall, signal, Table 2의 커널 객체 접근 순서, 공유 메모리 page ownership 전이(재생용)를 기록. 경합 분석 입력은 syscall의 커널 객체 load/store micro-op과 4종 HB 간선. 응용의 함수 호출·반환값·메모리 값은 기록 대상으로 열거되지 않음. 다만 논문은 recorder가 "응용과 OS 사이의 모든 비결정적 상호작용"을 기록한다고 쓰므로(p.356) syscall 결과는 재생용으로 기록되는 것으로 읽힌다 [I] (§4 pp.356–357, §5 pp.357–361) | **답** — 커널 객체 단위, 경로별 위치(부모 inode 안), 스트림 offset, 신호별 위치, pid 가상화. 세대 서술 없음("generation" 0건) (Table 2 p.357, §5.2 p.358, §6.2 p.362) | **답** — 커널 객체의 store(생성)와 load(조회·사용)의 동시 쌍. 공유 메모리·파일 내용에 의한 의존은 HB에 넣지 않음 (§5.2 p.358, §5.3.1 p.359) | **답** — 필요 순서·허용 오류 명세 입력 없음. 순서는 syscall 의미에서 추론한 HB. 실패 판정은 checker. "retry" 0건 (§5.1 p.357, §6.3 p.363) | **답** — 있음. 기록 실행에서 동시 pair/triple 후보 → 앵커 syscall 순서를 바꾼 branch → replay → go-live → checker. 3개 초과 syscall에 걸친 race는 기본 검출 안 함(p.361) (§5.3, §6.1–6.3) | **답** — 후보를 Diverged/Benign/Harmful로 가르고 checker가 녹화 때 없던 실패를 볼 때만 버그로 보고. 단위는 "race 후보(앵커 syscall 2~3개) + 검증 결과" (p.354, §6.3 p.363) |
| SimRacer (STVR 근거) | **답**(STVR이 서술하는 도구 기준) — SR 접근(shared memory, fd, inode, file data, `/proc` process data, hardware)과 동기화 사건. 메모리는 Simics 중단점. 반환값·쓰인 값은 열거 안 됨 (STVR p.8, p.14). **SimRacer 자체:** 메모리 관측이 있었는지는 불명(STVR p.8이 [16,33]을 함께 인용). 같은 해 Sim-O/C는 process 사이 사건을 system call로 서술(ITJ p.188 `2차`). 그래서 사례별 Q1은 불명(§2.3) | **답** — SR 종류별로 같은 SR 접근을 짝지음. "같은 SR" 기준, 이름·ID·슬롯 대응, 세대 서술 없음 (p.8, p.10). SimRacer 자체는 전문 불명. Sim-O/C의 예는 "두 process가 모두 `write`로 같은 파일에 쓰는" 경우(ITJ p.188 `2차`) | **답** — "같은 SR, 하나 이상 write, HB 없음" 쌍(Fig.10). 등록·초기화 완료 상태 개념 없음. lock·semaphore는 모델 밖 (p.9, p.10) | **답** — 입력 아님. race 정의에 "intended order"가 있으나 입력 필드 없음. HB는 syscall 의미에서 추론. oracle은 출력 기반. "retry" 0건 (p.3, p.9, p.16–18) | **답** — 있음. Phase 1이 trace에서 잠재 쌍을 계산, Phase 2가 쌍마다 순서를 뒤집어 실행 확인. 한 실행에 한 쌍, 두 접근 사이 order violation에 한정 (p.7, p.10–11, p.13–14). process·signal 부분의 이 구조는 ISSTA 초록과 일치해 SimRacer에도 해당한다고 봄 [I] | **답** — PRaceSet(잠재 쌍), RaceSet(real race), oracle 불일치 fault를 구분. 출력에 전파되지 않은 real race도 따로 보고. 같은 명령어 위치 반복은 한 쌍으로 합침 (p.10, p.13, p.19, p.22) |
| SIMEXPLORER | **답** — SR 접근 + 동기화 사건 + syscall 23종의 read/write 효과. 메모리는 Simics 중단점. 함수 진입·반환은 PuT 선별용 call stack이며 반환값을 사건으로 기록한다는 서술 없음 (p.8, p.14) | **답(부분)** — "SR_i equals SR_j" 기준 서술 없음. 파일 계열은 객체 단위로 효과를 모델. 세대 서술 없음. 사용자 SR 지정 인터페이스의 형식 서술 없음 (p.8, p.10) | **답** — "생성·등록·초기화 완료" 상태 개념 없음. 표현 단위는 같은 SR 접근 쌍과 HB 여부 (p.8–10) | **답** — 입력 아님. 사용자가 SR·syscall을 추가 지정하는 인터페이스는 있음. 재시도·반환값 서술 없음 (p.8, p.9, p.16–18) | **답** — 있음(위와 같음). 한계는 STVR p.10–14, p.19 | **답** — PRaceSet / RaceSet / oracle 출력 차이. race가 확인돼도 출력에 영향이 없으면 fault로 세지 않음 (p.7, p.11, p.13, p.19) |
| RRF | **답** — syscall·signal을 Scribe로 기록. `resource lock` 기록(type inode/files_struct/file 등). 사용자 공간 메모리 접근·반환값·futex·mutex·semaphore 사건 서술 없음 (p.1–4, Fig.2) | **답** — 엔지니어가 bug report에서 자원을 지정(설정 파일의 shared resource name)하고 로그를 거름. symbolic link는 Scribe가 실제·symbolic 이름 병기. ID·세대 서술 없음 (p.4–5, p.7–8) | **답** — "두 프로세스의 syscall 쌍이 같은 자원에 접근"하는 모양. 두 순서를 모두 만듦. 등록·초기화 완료 상태 개념 없음 (p.5–7) | **답** — 필요 순서·허용 오류 필드 없음. 순서는 열거(쌍 × 두 순서 × 네 패턴). oracle 저장소는 확장 가능. "retry" 0건 (p.2, p.3, p.6, p.7) | **답** — 관련 syscall 쌍 전체를 열거해 sleep으로 순서를 강제하고 실제 실행에서 oracle로 확인. HB 분석으로 후보를 계산하지 않음. 반대 순서를 만들 수 없는 경우의 처리 서술 없음 (p.5–6, p.8) | **답** — race report(발생 여부, targeted/incidental/new 분류, 이벤트 쌍 위치). 후보 목록이 아니라 "겨냥한 race가 재현됐는가". 반환값·순서 근거는 출력 항목 아님 (p.3, p.6, p.8–10) |
| DESCRY | **답** — 실패 실행의 default log message, PuD 실행의 syscall 사건. 로그 → 소스 대응은 printing API(`printf`, `sprintf` 등)의 format string 일치이며, "customized logging facilities"를 어려움으로 들고 printing API가 라이브러리 안에 있으면 inter-procedural CFG의 data·control flow로 PuD 소스 위치까지 되짚는다고 씀(p.5). 메모리 접근·반환값·futex·등록·초기화 상태를 사건으로 쓴다는 서술 없음. 사건·자원 모델링 상세는 [21,38]에 위임 (p.3, p.5, p.7) | **답** — 시스템 전역 자원을 syscall로 접근하는 것으로 봄. 사건은 자원 이름 표기. PuD별 입력이 같은 자원을 가리키게 파일 이름을 통일. ID·slot·세대 서술 없음 (p.3–4) | **답** — write 후 read 순서가 뒤집히면 실패하는 경우로 다룸. 등록·초기화 완료·readiness 개념 없음. 관계는 5종으로 닫혀 있음 (p.2–3, p.7) | **답** — 필요 순서·허용 오류 필드 없음. 순서는 탐색으로 만듦. "무엇이 잘못인가"는 로그 실패 메시지(failure point)가 정함. "retry" 0건 (p.1–5, p.7) | **답** — 있음. 첫 구체 실행 trace의 POG에서 순서 관계 없는 pair를 뒤집어 schedule 생성(predictive), Simics replay로 확인. infeasible은 건너뜀. 뒤집을 수 없는 이유를 보고한다는 서술 없음 (p.4, p.7) | **답** — ⟨T, S⟩(입력과 interleaving schedule)와 재현 성공 여부. 성공은 failure point 도달. 목표가 infeasible이면 failure predicate로 완화 (p.3–4, p.7–8) |
| SimEvo | **답** — syscall의 R/W 효과 + fork, wait, exit, pipe, signal. 탐색이 맞추는 Def-Use 패턴도 "a definition is a system call write access and use is system call read access"로 정의(p.6). 메모리·lock·반환값은 사건으로 서술되지 않음(load/store 0건) (p.1–3, p.6) | **답(파일 이름 통일만), 나머지 불명** — 이름·ID·슬롯·세대 서술 없음. 세부는 RacePro [26]·SimRacer [57]에 위임 (p.3, p.5) | **답** — 다섯 partial order(program, fork-return, wait-exit, pipe-read, signal). 상태 관계 정의 없음 (p.3, p.6) | **답** — 입력은 S, S′, 기존 테스트 T, 커버리지 기준 PT. 허용 오류·재시도 서술 없음. 오류 없이 출력이 바뀌는 문제는 미해결 (p.3, p.4, p.8) | **답** — 있음. 관측 trace에서 변경으로 영향받은 접근을 포함하는 쌍을 뒤집은 schedule을 offline 생성(PTA)하고 Simics 재생으로 확인. 대상은 Def-Use 쌍 (p.4, p.6, p.7) | **답** — 선택된 테스트 T∆, 영향받은 Def-Use 쌍 커버리지 C, 결함 F(오라클 불일치) (p.4, p.9) |
| ReDPro | **불명** — PIN으로 run-time monitoring. 감시하는 사건 종류는 초록에 없음. 다만 초록은 failure가 "system call의 실행 순서"에 민감하다고 문제를 정의하고 "specific system-call interleaving"을 언급한다(ReDPro.md §0 항목 5, §2.1 `초록`). 이것은 failure가 무엇에 민감한가의 서술이지 PIN이 무엇을 계측했는지의 서술이 아니다. PIN의 일반 기능은 메모리 접근도 계측할 수 있으나 ReDPro가 무엇을 계측했는지의 증거가 아님 | **불명** — 초록은 "a shared data item"만 말함 | **불명** — 초록의 실패 정의는 "read and write a shared data item without proper synchronization". readiness precondition을 표현하는지 쓰여 있지 않음 | **불명** — 초록에 입력 서술 없음. SysPro p.32가 ReDPro를 "실행 입력을 개발자가 안다고 가정"하는 작업으로 묶음(`2차`) | **분석** — 초록: PIN으로 potential race pair의 실행 순서를 제어하고 failure를 regenerate. pair 생성 방식은 불명 | **분석** — 초록: race 탐지와 failure 재생성. 보고 형식과 failure 정의는 불명 |
| SysPro | **답** — 실행 중 사건을 수집한다는 서술이 없음. 입력은 보고서 텍스트와 소스의 정적 구조. 런타임은 PIN으로 선택한 syscall 호출 지점에 sleep을 넣는 것 (p.16, p.20–21, p.24) | **답** — 알고리즘은 자원을 식별하지 않음. 출력은 ⟨syscall 이름, 줄, 함수, 파일⟩. 같은 자원임은 보고서와 사용자 입력이 전제 (p.3, p.14) | **답** — syscall 쌍의 순서와 사이에 끼는 다른 프로세스의 호출. 쌍은 파일 시스템 syscall. 초기화·등록·readiness·시작 동기화 개념 서술 없음 (p.6, p.11, p.22, p.25) | **답** — 필요 순서·허용 오류 입력 없음. 후보는 보고서 동시 출현 syscall 쌍의 빈도와 소스 호출 순서에서 추론. "retry" 0건 (p.5, p.10, p.14–19) | **답** — 탐색. 순위가 높은 후보 위치부터 한 번에 하나씩 sleep을 넣고 실제 재실행으로 알려진 실패의 재현을 확인. trace에서 아직 실행되지 않은 순서를 만드는 예측 서술은 없음 (p.19–21, p.24, p.26–27) | **답** — 후보 syscall 위치의 순위 목록, 재현 성공 시 schedule. 재현 판정 방법 서술 없음. 후보 race 목록도 readiness violation 판정도 아님 (p.3, p.14, p.24–25) |

### 2.3 라벨 요약 (도구 × 사례 × 질문)

| 도구 | 사례 | Q1 | Q2 | Q3 | Q4 | Q5 | Q6 |
|---|---|---|---|---|---|---|---|
| RacePro | C1 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| RacePro | C2 | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| RacePro | C3 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| RacePro | C5 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| RacePro | R2 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| SimRacer | C1 | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |
| SimRacer | C2 | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| SimRacer | C3 | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |
| SimRacer | C5 | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |
| SimRacer | R2 | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |
| SIMEXPLORER | C1 | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| SIMEXPLORER | C2 | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| SIMEXPLORER | C3 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| SIMEXPLORER | C5 | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| SIMEXPLORER | R2 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| RRF | C1 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| RRF | C2 | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| RRF | C3 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| RRF | C5 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| RRF | R2 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| DESCRY | C1 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| DESCRY | C2 | 불명 | 불명 | 분석 | 답 | 불명 | 불명 |
| DESCRY | C3 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| DESCRY | C5 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| DESCRY | R2 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| SimEvo | C1 | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| SimEvo | C2 | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| SimEvo | C3 | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| SimEvo | C5 | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| SimEvo | R2 | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| ReDPro | C1 | 불명 | 불명 | 불명 | 불명 | 분석(조건부) | 불명 |
| ReDPro | C2 | 불명 | 불명 | 불명 | 불명 | 불명 | 불명 |
| ReDPro | C3 | 불명 | 불명 | 불명 | 불명 | 분석(조건부) | 불명 |
| ReDPro | C5 | 불명 | 불명 | 불명 | 불명 | 분석(조건부) | 불명 |
| ReDPro | R2 | 불명 | 불명 | 불명 | 불명 | 분석(조건부) | 불명 |
| SysPro | C1 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| SysPro | C2 | 불명 | 불명 | 분석 | 답 | 불명 | 불명 |
| SysPro | C3 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| SysPro | C5 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| SysPro | R2 | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |

도구별 라벨 수(이 표의 집계). Q4는 도구 수준 답이라 다섯 사례 칸이 같다. 그래서 Q4와 나머지 질문(Q1·Q2·Q3·Q5·Q6)을 나눠 센다.

| 도구 | Q4 (도구 수준, 사례 5칸 동일) | Q1·Q2·Q3·Q5·Q6 25칸: 분석 | 불명 |
|---|---|---:|---:|
| RacePro | 답 | 20 | 5 |
| SimRacer † | 답 | 12 | 13 |
| SIMEXPLORER † | 답 | 18 | 7 |
| RRF | 답 | 20 | 5 |
| DESCRY | 답 | 21 | 4 |
| SimEvo | 답 | 16 | 9 |
| ReDPro | 불명 | 4 | 21 |
| SysPro | 답 | 21 | 4 |
| **합** | **답 7, 불명 1** (도구 수준 8건) | **132** | **68** (합 200칸) |

† SimRacer와 SIMEXPLORER는 같은 STVR 근거를 쓴다. Q4 답 7건은 이 둘을 한 계보로 묶으면 6건이다(§5.4 항목 7). 두 행의 Q4를 뺀 50칸 중 44칸이 같은 라벨이다. 다른 6칸(C1·C5의 Q1, C3·R2의 Q1·Q2)은 SimRacer.md가 2026-10-16에 SimRacer 자체를 대상으로 "불명"으로 바꾼 칸이고, SIMEXPLORER.md는 STVR이 서술하는 도구를 대상으로 "분석"을 유지했다(§2.5 항목 1). 이 문서는 두 라벨을 합치거나 통일하지 않았다. 따라서 200칸은 독립 근거의 수가 아니다. 위 라벨 요약 표(40줄)의 Q4 열에는 "답"이 35칸 있으나(나머지 5칸은 ReDPro의 불명) 이는 도구 수준 답 7건이 사례 수만큼 반복된 것이므로 "답 35"로 세지 않는다.

2026-10-15판 대비 변화: SimRacer 분석 20 → 12, 불명 5 → 13. 나머지 일곱 도구의 라벨은 바뀌지 않았다(각 도구별 파일 §3.4와 대조. DESCRY.md·RRF.md·ReDPro.md·SimEvo.md는 라벨 불변을 명시한다). 합은 분석 140 → 132, 불명 60 → 68이다.

### 2.4 도구별 사례 칸

각 표의 칸은 `라벨 — 내용`이다. 내용은 도구별 파일 §3.5의 압축이다. "probe"는 OSAL 자체 시험을 syscall 관측자로 본 것으로, 어느 도구의 결과도 아니고 cFS 실행도 아니며 OSAL `d2d877a` 기준이다(`availability.md` §4.2 기록). 칸 안의 `[M-probe]`는 그 기록(§0.1), `[I 소스]`는 소스 수준 추론이다. 두 표기 모두 사례 revision(C1의 OSAL `42af0f73`, C3·C5의 OSAL 4.1.1 시대)의 syscall 관측이 아니다. 사례 쪽 사실은 모두 저장소 기록이며 이 문서에서 다시 확인하지 않았다.

#### 2.4.1 RacePro (근거: 논문 전문 + 코드 직접 읽음)

분석 칸의 공통 전제(RacePro.md §3.1·§3.6): "경로가 확인되지 않는다"는 읽은 설계 기술과 코드 안에서 그 쌍을 후보 입력 사건으로 만드는 경로를 찾지 못했다는 뜻이다. 어느 칸도 "RacePro가 사례를 탐지하지 못한다"는 뜻이 아니다. 논문은 go-live 뒤 자연 발생한 실패를 checker가 보고할 수 있다고 인정한다(p.363). "RacePro는 공유 메모리를 기록하지 못한다"는 문장은 틀리다. 기록은 하되 HB와 후보에 쓰지 않는다(p.357–358). 2026-10-16 코드 읽기(RacePro.md §2.5 K5): py-scribe는 `mem_owned_*` 사건을 파싱할 수 있고(주소는 `SCRIBE_MEM_EXTRA` 때만) `racepro` explorer는 쓰지 않는다. 이 사건을 후보 생성에 쓰는 것은 `EXTENSION`이다. 2026-10-16 코드 읽기(K6): Scribe 상태는 task마다 붙고 explorer는 로그의 PID 사건 값으로 Process를 나눈다. 그 값이 thread id인지는 실행해야 확인된다.

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | 분석 — 생성·조회가 syscall 0건 `[M-probe]`(OSAL `d2d877a`, 경합 없음, C1의 OSAL `42af0f73` 아님)이라 경합 분석 입력(커널 객체 접근)에 안 나타남. 메모리 page ownership 전이는 재생용 기록이나 경합 분석이 쓴다는 서술·코드 참조 없음 | 분석 — 이름 → OSAL ID → 테이블 항목이 도구의 식별 장치(경로·파일 핸들·pid)에 대응하지 않음. 코드는 FUTEX를 RESOURCE 후보에서 제외. 세대 서술 없음 | 분석 — 생성·조회가 커널 객체 접근이 아니라 micro-op 쌍이 만들어지지 않음. '등록 완료' 상태를 구분하는 장치 없음 | 답 — 허용 오류(재시도는 `OS_ERR_NAME_NOT_FOUND`만)를 넣을 인터페이스 없음. 실패 기준(CF 초기화 abort)은 사용자 checker | 분석 — branch는 기록된 동시 syscall 앵커 쌍을 뒤집은 것이고 생성↔조회는 앵커가 아님. 다른 후보의 go-live 뒤 C1의 실패가 자연 발생하면 checker가 볼 수 있으나 보고는 그 후보 이름으로 나가 원인 쌍을 짚지 않음 | 분석 — 출력은 "race 후보 + 검증 결과". 회복과 최종 실패를 구분하는 출력 항목 없음. 대응하려면 checker가 abort를 실패로 보고하고 보고된 앵커가 생성↔조회여야 하는데 후자는 경로 없음 |
| C2 | 불명 — P·R 비공개 | 불명 | 불명 — barrier 통과와 앱 고유 초기화 완료를 구분하는 장치는 논문 도구 모델에 없으나 의존 자원이 비공개라 불명 | 답 — 실패 기준은 원 보고가 구체화하지 않아 연구 쪽 선택이 됨 | 불명 — 앵커 쌍을 알 수 없음 | 불명 — 원 현상이 구체 출력으로 공개되지 않음 |
| C3 | 분석 — AppId 저장·`EVS_AppID`·등록 플래그·TIME의 이벤트 요청·`EVS_IsFiltered` 접근은 사용자 공간 함수 호출과 전역 메모리. TIME의 `CreatePipe`가 부르는 `mq_open`은 syscall이나 의존 상태 자체가 아님(`CreatePipe` → `mq_open` 연결은 `[I 소스]`, `mq_open` 자체는 `[M-probe]`). C3의 원 판본(OSAL 4.1.1 posix) queue 구현이 `mq_open`을 쓰는지는 확인하지 않았고, 쓴다면 R2 ③과 같이 fd table(FILES_STRUCT) 쓰기만 자원 접근이 됨(RacePro.md §2.5 K1–K3, 분석·조건부) | 분석 — 의존 상태는 AppId 정수(ES가 부여)와 EVS 배열 항목이며 커널 객체가 아님 | 분석 — 공개(저장)와 사용(읽기)이 모두 메모리라 store/load 쌍으로 표현되지 않음 | 답 — 오류 지점이 사례 기록에서 미확인이라 어떤 checker가 필요한지 지금 정할 수 없음 | 분석 — 의존 쌍이 앵커가 아니므로 해당 순서를 뒤집은 branch가 만들어지지 않음. go-live 뒤 자연 발생한 실패는 다른 후보 이름으로 보고될 수 있음 | 분석 — built-in checker에 segfault가 있어 crash는 오류 결과로 볼 수 있으나 잘못된 상태 사용과 crash를 구분하는 출력은 없음. crash 자체가 사례 기록상 미확인 |
| C5 | 분석 — `pthread_create`의 `clone`은 기록되고 HB 노드가 됨. `TaskTable` 쓰기와 `GetAppID` 조회는 메모리라 입력에 없음 | 분석 — task ↔ AppId 연결은 ES 테이블 안에 있고 커널 쪽 task 식별(pid 자원, 가상화)은 AppId와 연결되지 않음 | 분석 — "clone → 자식의 첫 syscall" HB는 표현되나 "등록 공개 완료" 사건은 없음 | 답 — 내부 API 반환값을 보는 사용자 checker가 필요. 원 보고에 없는 앱 crash는 기대 결과로 넣지 않음 | 분석 — 등록 쓰기를 `clone` 앞뒤로 옮기는 branch는 만들 수 없음 | 분석 — built-in checker 4종(segfault, timeout, 시스템 로그 오류 메시지, 비정상 종료 코드)은 내부 API 반환값을 보지 않음 |
| R2 | 분석 — 네 사건(AppId 설정, 값 전달, 이벤트 요청, 오류 반환)이 함수 호출과 메모리. `CreatePipe`의 `mq_open`만 syscall(연결은 `[I 소스]`, `mq_open` 자체는 `[M-probe]`). 2026-10-16 코드 읽기: `mq_open`의 fd 할당·설치는 FILES_STRUCT 쓰기로 기록되고 큐 이름·큐 객체에는 Scribe 자원 잠금이 없음(K1–K3). ⓪ core task 생성의 `clone`은 기록됨 (§6.2) | 분석 — SB AppId의 값(0 대 유효값)은 메모리의 정수 | 분석 — 설정(저장)과 사용(읽기)이 메모리라 store/load 쌍으로 표현되지 않음 | 답 — R2의 요구 순서를 넣을 입력이 없음 | 분석 — HB 그래프는 core 시작 직렬화(ES가 각 core task가 RUNNING이 될 때까지 기다리는 폴링) 같은 사용자 수준 순서를 담지 않아(p.358) 도구 안에서 순서의 이유가 설명되지 않음. AppId 설정·사용 쌍은 커널 객체 syscall이 아니어서(`[I 소스]`) 그 순서를 뒤집은 branch는 만들어지지 않음. ③ `mq_open`이 만드는 FILES_STRUCT 쌍은 앵커가 될 수 있으나 그 branch가 뒤집는 것은 fd table 접근 순서이고, replay에서 diverge하는지, go-live 뒤 AppId 설정 순서가 달라지는지는 읽은 자료로 정할 수 없음(K2·K4) | 분석 — 사례 현상은 "이벤트 요청 거부 24회"이고 프로세스 실패가 없음. built-in checker는 내부 반환값을 보지 않음. 대응하려면 사용자 checker가 내부 상태를 볼 수 있어야 함. 또 보고 조건은 checker가 "a failure that did not occur during recording"을 보는 것(p.363)이라, RacePro 녹화에도 현상이 들어 있다면 branch의 같은 현상은 "녹화 때 없던 실패"가 아님. RacePro 녹화에서 같은 순서가 나오는지는 미확인(H-RP-5) |

#### 2.4.2 SimRacer (근거: 초록 + STVR 2017 전문 + Sim-O/C·SimTester `2차`)

분석 칸의 공통 전제(SimRacer.md §3.5): 사례의 접근이 SR로 관측되어야 한다. 한 프로세스 안의 task 쌍을 PuT 쌍으로 줄 수 있는지는 서술이 없다. 이 칸들의 도구 서술은 STVR의 SIMEXPLORER 서술이며 ISSTA'13의 SimRacer와 같다고 확인된 범위는 SimRacer.md §2.3에 적힌 만큼이다(Phase 1·2 구조는 초록과 일치하므로 해당한다고 봄 [I]. 메모리 관측이 SimRacer에 있었는지는 [U]). **2026-10-16부터 Q1·Q2 칸은 두 층이다.** 라벨은 SimRacer 자체에 대한 것이고(C1·C3·C5·R2 모두 불명), 뒤의 "STVR 설계 기준" 문장은 라벨을 정하지 않는 분석이다. 같은 분석은 §2.4.3 SIMEXPLORER가 그 도구의 칸으로 다룬다. Q3·Q5·Q6은 쌍의 모양·순서 뒤집기·출력 종류에 기대므로 조건부 "분석"으로 남는다. 어느 칸도 "SimRacer가 사례를 탐지하지 못한다"는 뜻이 아니다. "SimRacer는 메모리를 관측하지 못한다"도, "SimRacer는 메모리를 SR로 관측한다"도 쓰지 않는다.

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | **불명** — SimRacer 자체: 메모리 쓰기·읽기가 SimRacer의 SR 사건이었는지 정해지지 않음. STVR 설계 기준(라벨 아님): 이름 등록(쓰기)·조회(읽기)는 syscall이 아님 `[M-probe]`(OSAL `d2d877a`, 경합 없음). OSAL 테이블 주소가 SR로 지정되면 메모리 중단점으로 사건이 될 수 있음. 어떤 주소가 SR이 되는지(자동/사용자 지정)와 입도 서술 없음 | **불명** — SimRacer 자체: "같은 SR" 판정 기준도, 메모리가 SR이었는지도 서술 없음. STVR 설계 기준(라벨 아님): 이름 → ID → 슬롯 → 객체를 잇는 장치 서술 없음. 같은 슬롯의 쓰기·읽기를 짝지으려면 "같은 SR 판정 기준 = 주소" 가정이 필요 [I] | 분석 — "같은 SR, 하나 이상 쓰기, HB 없음" 쌍으로 표현될 수 있음. 예약/공개 완료/재시도 중/최종 실패 구분 개념 없음. OSAL 이름 테이블 lock은 HB 규칙에 없음 | 답 — 허용 오류를 넣을 인터페이스 없음. 실패 기준은 출력 기반 oracle로 써야 함 | 분석 — 쌍이 SR로 관측되면 Phase 1 → 2가 적용될 수 있는 모양. 어느 쪽이 정상 순서인지 도구는 모름. 수정 버전(25회 재시도)에서도 같은 쌍이 real race가 될 수 있고 최종 실패 여부는 oracle이 정함 [I] | 분석 — 출력은 잠재 쌍·real race·oracle fault. 회복과 최종 실패를 구분하는 출력 항목 없음. 사례의 실패 출력이 `unspecified_by_reference`라 oracle이 볼 출력이 미정 |
| C2 | 불명 | 불명 | 불명 — 상태 개념은 STVR 본문에 없음(키워드 0건)이나 의존 자원이 비공개라 불명 | 답 | 불명 | 불명 |
| C3 | **불명** — SimRacer 자체: 위와 같음. STVR 설계 기준(라벨 아님): 전역 메모리 읽기·쓰기, SR로 지정되면 사건. `mq_open`이 모델링된 syscall에 드는지 서술 없음. 원 플랫폼 Microblaze는 STVR의 시험 플랫폼(x86 Linux)과 다름 | **불명** — SimRacer 자체: 위와 같음. STVR 설계 기준(라벨 아님): 세 필드를 메모리 위치로 구별한다는 해석은 주소 기반 동일성 가정 필요 [I]. AppId 값 0이 EVS 배열 인덱스로 쓰이는 관계 추적 서술 없음 | 분석 — 공개(쓰기)·사용(읽기)이 같은 SR 쌍으로 표현될 수 있음. core 시작 순서를 정하는 ES task 대기와 우선순위는 HB 규칙에 없음 | 답 — 오류 지점·주 결과가 사례 기록상 미결이라 oracle이 볼 것을 정할 수 없음 | 분석 — 순서가 우선순위 때문이라 관측 trace에 이미 있을 수 있고 그러면 Phase 2는 반대(공개 → 사용)를 시도. 정상 순서를 도구는 모름 [I] | 분석 — real race와 oracle fault를 분리하는 구조는 있으나 crash를 별도 검출한다는 서술 없음("segfault" 0건). crash 자체 사례 기록상 미확인 |
| C5 | **불명** — SimRacer 자체: 위와 같음. STVR 설계 기준(라벨 아님): `clone` 계열은 모델링된 syscall 예시(`/proc`에 process inode를 만드는 쓰기). `TaskTable` 쓰기와 `GetAppID` 조회는 SR 지정이 있어야 사건 | **불명** — SimRacer 자체: 위와 같음. STVR 설계 기준(라벨 아님): 커널 쪽 task 식별은 PID(thread도 PID를 가진 lightweight process, p.13). OSAL task ID·`TaskTable` 인덱스·AppId 연결 서술 없음 | 분석 — thread 생성(`clone`)이 fork 규칙 대상인지 서술 없음. 어느 쪽이든 등록 쓰기는 task 생성 뒤라 자식 조회와 HB 없는 잠재 쌍. "등록 공개 완료" 상태 없음. 6.4.2의 lock 범위 확장은 lock이 모델 밖이라 Phase 2에서 뒤집기 가능 여부로 가려질 것 [I] | 답 — 내부 API 반환값은 앱 출력에 나타나야 oracle이 볼 수 있음 | 분석 — 이 쌍은 설계가 드는 예(부모가 fork한 자식과의 race, Fig.1)와 모양이 가장 가까움 [I] | 분석 — 반환값 `CFE_ES_ERR_APPID`는 출력 항목이 아님 |
| R2 | **불명** — SimRacer 자체: 위와 같음. STVR 설계 기준(라벨 아님): 네 사건 중 앞 둘은 전역 메모리 쓰기·읽기, `mq_open`만 syscall(모델링 여부 서술 없음 → 사건 ③ 불명), 오류 반환은 반환값 | **불명** — SimRacer 자체: 위와 같음. STVR 설계 기준(라벨 아님): `CFE_SB_Global.AppId` 한 메모리 위치. 쓰기·읽기 짝은 주소 동일성 가정으로 가능한 해석. EVS의 값 전달(이벤트 원천 ID로 자기 테이블 조회)은 추적 안 함 | 분석 — 설정(쓰기)과 사용(읽기)이 같은 SR 쌍. core 시작 직렬화(S1)는 HB 규칙에 없음 | 답 — R2의 요구 순서를 넣을 입력 없음 | 분석 — 쌍이 관측되면 Phase 2는 기록 순서의 반대를 시도. 시작 직렬화로 반대 순서를 만들 수 없으면 race로 세지 않음(p.11). R2 순서가 뒤집힐 수 있는지는 사례 기록상 미결이므로 "설계상 시도하고 불가능하면 세지 않는다"까지. 고정된 이유를 담는 출력 항목 없음. 2026-10-16 추가 [I, 미실행]: S1이 사용 task를 RUNNING에 이를 때까지 기다린 뒤에야 SB task를 만들므로, 사용 직전에 사용 task를 멈추면 SB task가 생기지 않는 "한쪽을 멈추면 다른 쪽이 기다리는" 교착 모양(STVR p.13)이 도출됨. 교착을 풀면 사용이 먼저 일어나 그 쌍은 race로 세지 않고(p.11), 그 전에 S1의 시간 상한이 지나면 ES panic이라는 다른 결과(`cases/R2.md` A7)가 됨. 어느 쪽인지, 쌍이 애초에 관측되는지는 정해지지 않음 | 분석 — 반환값은 출력 항목이 아니고 실패 메시지도 없어 출력 기반 oracle에 차이가 안 나타날 수 있음. 같은 명령어 위치의 반복 접근은 한 쌍으로 합쳐지므로 "24"는 이 도구의 출력 수량이 아님 |

#### 2.4.3 SIMEXPLORER (근거: STVR 2017 저자 수락본 전문)

분석 칸의 공통 전제(SIMEXPLORER.md §3.5): cFS 내부 상태 접근이 관측 대상이 되려면 그 위치가 SR로 지정되어야 하는데 지정 방법과 입도는 서술이 없다("SR 지정이 전제"). 한 process 안의 task 쌍이 입력 단위가 되는지도 서술이 없다. 논문은 단일 process 안의 multithreaded 응용을 기존 thread-level 기법의 영역으로 두지만(p.1, p.4–5) 그런 race를 범위에서 뺀다는 문장은 없다(SIMEXPLORER.md §1.3 항목 11). 어느 칸도 "SIMEXPLORER가 사례를 탐지하지 못한다"는 뜻이 아니다.

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | 분석 — 이름 등록·조회는 OSAL 전역 테이블에 대한 사용자 공간 메모리 쓰기·읽기. SR 목록에 shared memory가 있고 Simics 중단점으로 잡으므로 SR 지정이 전제인 관측 대상. syscall 0건 `[M-probe]`(OSAL `d2d877a`)는 메모리 관측에는 적용되지 않음. 반환값(`OS_ERR_NAME_NOT_FOUND`)은 사건 목록에 없음 | **불명** — 이름 `BP0_tsem`(공개 BP 구성의 값, D22 기반. 원 보고자 환경의 이름은 미확인) → OSAL ID → 테이블 슬롯 연결과 삭제·재생성 세대 구분 서술 없음. "SR_i equals SR_j" 기준 서술 없음 | 분석 — 같은 SR의 쓰기→읽기 쌍 순서로 표현되는 형태. "이름 공개 완료"·재시도·허용 오류는 표현 요소에 없음. 테이블 접근은 OSAL 잠금 아래에 있는데 HB 규칙에 lock이 없음 | 답 — 허용 오류를 넣을 입력 없음 | 분석 — trace에 쓰기·읽기가 있고 인식되는 HB가 없으면 Phase 1이 쌍으로 계산하고 Phase 2가 두 접근을 멈춰 뒤집어 봄. 수정 버전의 100 ms 재시도는 설계가 서술한 "기다리는 쪽이 멈춘 쪽을 기다려 교착"(p.13) 구조와 닿음. 실제 결과는 미실행 | 분석 — 출력은 race 쌍과 oracle 오류. 회복과 최종 실패를 구분하는 출력 항목 없음 |
| C2 | 불명 | 불명 | 불명 — 시작 상태(OPERATIONAL)와 앱 고유 초기화 완료를 구분하는 개념은 설계 서술에 없음(키워드 0건). 그것만으로 표현 가능 여부는 정해지지 않음 | 답 | 불명 | 불명 |
| C3 | 분석 — 모두 사용자 공간 메모리 접근이고 SR 지정이 전제. `mq_open`이 syscall 23개에 있는지 불명. 원 플랫폼 Microblaze 대 STVR의 x86 환경 | 분석 — SB AppId·EVS AppId·등록 플래그는 서로 다른 변수. SR을 메모리 위치로 구별한다면 서로 다른 SR. 값의 의미와의 연결 서술 없음 | 분석 — "공개(저장)가 사용(읽기)보다 먼저"는 쓰기·읽기 쌍의 형태. 사례의 순서는 우선순위가 만들며 syscall 의미에서 추론한 HB(p.9)가 아님 | 답 — 오류 지점 미확인이라 어떤 oracle이 필요한지 정할 수 없음 | 분석 — 관측 trace의 순서를 뒤집어 실행하고 oracle과 비교. 의도한 순서를 입력받지 않으므로 어느 쪽이 요구 순서인지 모름 | 분석 — "잘못된 상태 사용"과 "실제 crash"를 구분하는 항목 없음. crash 자체가 사례 기록상 미확인 |
| C5 | 분석 — `pthread_create`는 syscall로 보임(기록: `clone3`). 논문의 syscall 예시는 `clone`이고 `clone3`에 대한 서술은 없음. `TaskTable` 쓰기와 `GetAppID` 조회는 메모리, SR 지정이 전제 | **불명** — task ↔ AppId 연결은 ES 테이블 안에 있음. thread를 PID로 추적한다는 한 줄(p.13)이 OS 수준 task 식별을 뜻할 수 있으나 AppId와의 연결은 서술 없음 | 분석 — 등록 쓰기와 조회 읽기 쌍의 형태. `clone`이 HB 규칙 ② fork와 같은 간선을 만드는지 서술 없음. "등록 공개 완료" 사건 없음. 6.4.2 수정은 lock이라 HB 규칙에 들지 않음 | 답 — 내부 API 반환값을 보는 oracle이 필요 | 분석 — 쌍이 계산되면 Phase 2가 두 접근 직전에 프로세스를 멈추고 뒤집음. 6.4.2의 lock 범위가 뒤집기를 막는지, 도구가 그것을 어떻게 다루는지는 서술 없음 | 분석 — 출력 기반 oracle은 콘솔·파일 출력을 비교하며 내부 API 반환값은 보지 않음 |
| R2 | 분석 — 모두 메모리 접근과 함수 호출. SR 지정이 전제. 반환값은 사건 목록에 없음. `mq_open`은 불명 | 분석 — SB AppId 필드와 EVS 등록 정보는 서로 다른 변수 = 서로 다른 SR. 값의 의미와의 연결 서술 없음 | 분석 — 설정(쓰기)→사용(읽기) 쌍의 형태. R2의 순서는 코어 시작 직렬화(상태 polling)가 정하며 syscall 의미에서 추론한 HB에 들지 않음. "바꿀 수 없는 위반"을 뜻하는 출력 항목은 어휘에 따로 없음 | 답 — R2의 요구 순서를 넣을 입력 없음 | 분석 — 직렬화를 HB로 인식하지 못하면 Phase 1이 쌍으로 계산할 수 있고 Phase 2는 순서를 뒤집어 보며 불가능하면 race로 세지 않음. 순서가 고정됐는지 실행으로 시험하는 방식이며 고정된 이유를 설명하는 방식이 아님 | 분석 — 실행당 "이벤트 요청 거부 24회"이고 호출자가 반환 상태를 버려 출력도 카운터 변화도 없음(기록: v7.0.1 `c5fb2b4d` 기준 `cfe_evs.c` L185–188, gdb breakpoint는 L188. 근거는 gdb 원시 기록과 `results/R2_gdb_v701.md` §2). race가 확인돼도 출력에 영향이 없으면 fault로 세지 않음(p.19) |

#### 2.4.4 RRF (근거: ISSRE 2016 전문)

분석 칸의 공통 전제(RRF.md §3.1): **전제 1** 사례의 접근이 RRF의 이벤트 로그(syscall·resource lock 기록)에 들어와야 후보 쌍이 된다. **전제 2** 한 프로세스 안의 task(thread) 쌍을 PuD 쌍으로 줄 수 있어야 한다(서술 없음 → 불명. p.1은 thread 수준 탐지기와 대비하지만 thread 사이 race를 범위에서 뺀다는 명시 문장은 없음). 어느 칸도 "RRF가 사례를 재현하지 못한다"는 뜻이 아니다. "RRF는 순서위반을 못 다룬다"는 문장은 틀리다(p.2의 race 정의가 order violation 포함).

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | 분석 — 이름 등록·조회가 syscall 0건 `[M-probe]`(OSAL `d2d877a`, 경합 없음, C1의 OSAL `42af0f73` 아님). RRF의 관련 이벤트는 syscall·resource lock 기록에서 정해지므로 후보가 되는 경로를 서술에서 찾지 못함. Scribe의 다른 기록에 들어오더라도 RRF가 쓴다는 서술 없음 | 분석 — 이름 → ID → 슬롯 장치 서술 없음. 설정 파일의 shared resource name은 Scribe가 보고하는 파일 이름이며 OSAL 이름은 커널 자원 이름이 아님. 세대 서술 없음 | 분석 — 생성·조회가 syscall로 안 보여 "같은 자원의 syscall 쌍"으로 표현되지 않음. 예약/공개 완료/재시도 중/최종 실패 구분 개념 없음 | 답 — 허용 오류 입력 없음. 실패 기준은 oracle에 둠. 사례의 실패 출력이 `unspecified_by_reference` | 분석 — 쌍이 로그에서 나오지 않으면 열거 대상이 되지 않음. 사람이 소스 위치를 직접 지정하는 입력 서술 없음. 위치가 주어진다면 sleep으로 두 순서를 시도하고 oracle로 확인하는 구조가 적용될 수 있음 [I] — 그것은 확장 | 분석 — 출력은 race report와 분류. 회복과 최종 실패를 구분하는 출력 항목 없음. 수정 버전이 재시도로 회복하는 경우의 보고 형식 서술 없음 |
| C2 | 불명 — RRF는 PuD 소스를 필요로 함(p.5) | 불명 | 불명 | 답 | 불명 | 불명 |
| C3 | 분석 — 전역 메모리 읽기·쓰기(syscall 없음 `[I 소스]`). `mq_open`은 현재 OSAL(`d2d877a`)에서 보였으나 `[M-probe]`, 6.4.1 시대(OSAL 4.1.1) 호출은 측정하지 않았고 그 queue 구현이 `mq_open`을 쓰는지도 확인하지 않음. 쓴다면 R2 ③과 같은 분석. 원 플랫폼 Microblaze의 OS 미확인, RRF의 구현은 Linux의 Scribe | 분석 — 세 필드를 구분해 서로 다른 자원으로 취급하는 경로 서술 없음. 값 → 슬롯 추적 서술 없음 | 분석 — 공개·사용이 메모리라 syscall 쌍으로 표현되지 않음. ES task 대기와 우선순위는 RRF 모델에 없음 | 답 — oracle이 무엇을 볼지 정할 수 없음 | 분석 — 순서는 우선순위가 만든 것이라 관측된 실행에 이미 있을 수 있음. 쌍이 주어지면 sleep으로 반대 순서(공개 → 사용)를 시도하는 구조가 적용될 수 있음 [I]. 정상 순서는 oracle이 정함 | 분석 — 출력은 race 분류. crash를 따로 검출한다는 서술 없음 |
| C5 | 분석 — `pthread_create`는 clone 계열(RacePro가 추적하는 syscall의 예, p.2). `TaskTable` 쓰기·`GetAppID` 조회는 메모리 접근이며 syscall이 없는 것으로 읽힘 `[I 소스]`. ES lock을 쥔 생성 task와 겹치면 lock 주소의 futex가 보일 수 있음(`availability.md` §4.5) | 분석 — 커널 쪽 task 식별은 Scribe의 process 번호. OSAL task ID·`TaskTable` 인덱스·AppId 연결 서술 없음 | 분석 — 등록 쓰기는 task 생성 뒤라 자식 조회와 같은 자원 쌍이 되지만 둘 다 syscall이 아니라 RRF의 쌍 정의에 들어오는 경로를 찾지 못함. "등록 공개 완료"와 lock 범위 확장은 RRF 모델에 없음 | 답 — 내부 API 반환값은 앱 출력에 나타나야 oracle이 볼 수 있음 | 분석 — 부모-자식 두 사건. RRF의 쌍은 서로 다른 프로그램의 두 프로세스 사이에서 정의되므로 전제 2에 걸림. 쌍이 주어지면 sleep으로 순서를 시도할 수 있음 [I] | 분석 — 반환값 `CFE_ES_ERR_APPID`는 출력 항목이 아님 |
| R2 | 분석 — AppId 설정·값 읽기는 전역 메모리, `mq_open`과 ⓪ task 생성만 syscall(`[I 소스]`, 사건 ③ 불명), 오류 반환은 반환값. 어느 것이 RRF의 관련 이벤트 로그에 들어오는지 서술에서 정해지지 않음. 메모리·반환값이 syscall이 아니라는 것은 `[I 소스]`(RRF.md는 이 칸에 `results/R2_gdb_v701.md`의 "no syscall was made"를 인용하나 이 문서는 그 문장을 근거로 쓰지 않음, §0.1) | 분석 — `CFE_SB_Global.AppId` 한 메모리 위치. EVS의 값 전달을 RRF가 추적한다는 서술 없음 | 분석 — 설정·사용이 메모리라 syscall 쌍이 아님. core 시작 직렬화는 RRF 모델에 없음 | 답 — R2의 요구 순서를 넣을 입력 없음 | 분석 — 쌍마다 두 순서를 sleep으로 시도. S1이 EVS·ES 초기화를 SB보다 앞세우므로 한 task에 sleep을 넣어 SB 설정이 먼저인 순서를 만들 수 있는지는 정해지지 않음 [I]. 이 칸이 답하는 범위는 "설계상 두 순서를 시도한다. 한 순서만 나오는 경우의 처리는 서술에 없다"까지 | 분석 — 반환값은 출력 항목이 아니고, 기록상 호출자가 반환 상태를 버려 출력도 카운터 변화도 없어(`cfe_evs.c` L188, v7.0.1) 출력 기반 oracle에 차이가 안 나타날 수 있음. RRF는 race가 실행마다 나타나는지를 쌍·순서별 실행 수로 세는데(Table II, p.8), 기록상 3회 모두 나타난 현상에서 targeted/incidental/new 분류가 무엇이 될지는 서술로 정해지지 않음. "24"는 이 도구의 출력 수량이 아님 |

#### 2.4.5 DESCRY (근거: FSE 2017 전문)

분석 칸의 공통 전제(DESCRY.md §3.1): **전제 1(사건)** 사례의 사건이 DESCRY의 사건 집합(syscall과 signal)에 들어와야 하며, 사건·자원 모델링 상세가 [21,38]에 위임돼 있어 syscall 밖의 사건도 보는지는 DESCRY 논문으로 정해지지 않는다(이 문서의 분석 칸은 서술된 사건 집합에 대한 것). **전제 2(단위)** 한 프로세스 안의 task 쌍을 PuD로 줄 수 있어야 하며 서술이 없다. **전제 3(입력)** 실패 실행의 default log와 PuD 소스가 있어야 한다. 어느 칸도 "DESCRY가 사례를 재현하지 못한다"는 뜻이 아니다.

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | 분석 — 필요한 사건은 이름 등록(`OS_CountSemCreate` 반환 시점)과 `OS_CountSemGetIdByName` 조회·반환값. OSAL 시험에서 생성 3회·조회 4회가 syscall 0건 `[M-probe]`(OSAL `d2d877a`, 경합 없음, C1의 OSAL `42af0f73` 아님). 서술된 사건 집합에서 후보가 되는 경로를 찾지 못함 | 분석 — 이름 → ID → slot 장치와 삭제·재생성 세대 장치 서술 없음 | 분석 — "생성 → 사용"은 write 후 read로 표현되는 모양이나 두 사건이 syscall이 아님 `[M-probe]`. 예약/공개 완료/재시도/최종 실패 구분 개념 없음 | 답 — 필요 순서와 허용 오류를 넣을 입력이 없음. 무엇이 실패인지는 로그 메시지가 정함 | 분석 — 뒤집을 pair는 첫 구체 실행의 trace에서 나오므로 두 사건이 trace에 없으면 후보가 아님. P-C1HOLD(생성 직전 보류)에 해당하는 서술 없음 | 분석 — 수정 전 소스에 실패 문장(`CFE_EVS_SendEvent`)이 있으나(기록) logging point 식별은 printing API의 format string 일치(p.5)다. p.5는 "customized logging facilities"를 어려움으로 들고 라이브러리 안의 printing API는 data·control flow로 되짚는다고 씀. cFE에서는 앱의 format string이 `CFE_EVS_SendEvent`의 가변 인자로 넘어가 `vsnprintf`로 이벤트 패킷이 되고 출력 port가 켜져 있으면 `OS_printf`로 나감(v7.0.1 번들 `cfe_evs_utils.c` L437–L493, L546–L599 `[I 소스]`, C1 구성의 cFE revision에서는 읽지 않음). 이 경로가 되짚기 대상인지, `OS_printf`가 printing API 목록에 드는지는 서술 없음. 재시도로 회복하는 경로는 failure point를 지나지 않음. 입력 로그 부재는 별개 조건 |
| C2 | 불명 — P·R 비공개. DESCRY는 PuD 소스를 요구(p.1, p.3) | 불명 | **분석** — 도구 쪽 서술만으로: 관계가 5종으로 닫혀 있고 "초기화 완료"·시작 동기화 통과 개념이 없음. P의 late init 끝에 syscall 표지가 있는지는 비공개라 사례 쪽은 불명 | 답 — barrier 통과와 앱 고유 초기화 완료를 구분하는 입력 없음 | 불명 | 불명 — 원 보고에 실패 현상 출력·로그 없음. 역할 재구성(P-C2)으로만 다룸 |
| C3 | 분석 — 의존 사건은 메모리 쓰기·읽기. pipe 생성의 `mq_open`은 보이나 `[M-probe]`(`CreatePipe` 연결은 `[I 소스]`) 의존 상태가 아님. message queue를 자원 접근 사건으로 모델링하는지 서술 없음("mq_" 0건) | 분석 — 세 필드의 필드 단위 식별 서술 없음. 자원은 syscall의 대상 | 분석 — SB·EVS 초기화 완료 뒤에야 TIME의 이벤트 요청이 유효하다는 관계를 표현할 장치 없음. 순서는 우선순위가 만들고 관계 5종이 이를 담지 않음 | 답 | 분석 — 우선순위로 순서가 정해지는 점은 "kernel scheduler 제어"가 필요한 문제(p.3)와 가까우나 뒤집을 pair가 trace에 있어야 한다는 조건이 먼저. 원 플랫폼 Microblaze와 Simics 설정 서술 없음 | 분석 — 현상은 segfault이고 메시지가 없어 failure point 성공 기준을 세울 수 없음. core dump를 쓰는 기법은 별도 부류로 서술(p.10). crash 전후 로그 메시지 유무 불명(댓글 미독) |
| C5 | 분석 — `OS_TaskCreate`의 `clone3`는 syscall로 보임 `[M-probe]`(OSAL `d2d877a`, C5의 OSAL 4.1.1 시대 아님). DESCRY는 `clone`을 `/proc` 아래 process inode 생성(write)으로 모델링하고 fork-return을 child **process**에 정의(p.3, p.7). thread 생성(`CLONE_THREAD`) 서술 없음(전제 2). 등록 쓰기·조회는 ES lock 아래 메모리 접근이고 경합하면 lock 주소의 futex가 보일 수 있음 `[I 소스]`. DESCRY 논문에 futex·mutex 사건 서술 없음(키워드 0건) | 분석 — OS task ID ↔ AppId ↔ `TaskTable` slot 대응과 "등록 전" 상태를 식별하는 장치 서술 없음 | 분석 — 생성(`clone3`)만 syscall. readiness violation은 "등록 전 조회"이므로 등록 사건과 조회의 반환 결과가 필요 | 답 | 분석 — pair를 뒤집으려면 등록 쓰기와 조회가 trace의 사건이어야 함. `clone3`는 사건이 되더라도 순서가 의미를 갖는 두 접근이 아님 | 분석 — 현상은 새 task의 첫 조회 실패(`CFE_ES_ERR_APPID`). 원 보고에 앱과 로그가 없어 failure point가 앱 의존 |
| R2 | 분석 — AppId 저장(메모리 쓰기), SB API 경로 호출, `ILLEGAL_APP_ID` 반환. `mq_open`은 보이나(`[I 소스]`) AppId 저장·반환값은 안 보임(`[I 소스]`). 서술된 사건 집합에 들어오는 경로를 찾지 못함 | 분석 — `CFE_SB_Global.AppId`와 EVS 등록 정보를 서로 다른 상태로 구분해야 하나 서술 없음 | 분석 — SB가 AppId를 저장한 뒤에야 이벤트 요청이 유효하다는 관계를 표현할 장치 없음 | 답 | 분석 — R2의 순서가 core 시작 직렬화(S1)가 만든다는 것은 코드 읽기의 예측이며 실행 확인 안 됨. 뒤집을 수 없으면(replay가 다른 분기를 타면) infeasible로 보고 건너뛰어 다음 pair로 감. 뒤집을 수 없는 이유를 결과로 보고하는 서술 없음. 2026-10-16 추가 [I, 미실행]: S1 아래에서 ①을 ③보다 앞세우는 schedule은 아직 생기지 않은 SB task를 기다리며 사용 task를 멈춰 두어야 함. p.7의 replay 서술은 분기가 달라진 경우만 다루고 진행이 막히는 경우의 처리는 서술 없음. 그 전에 ①과 ③이 사건 pair가 되는지부터 정해지지 않음 | 분석 — 이벤트 요청이 거부되고 출력이 없어(v7.0.1 `c5fb2b4d` 기준 `cfe_evs.c` L185–188 `[I 소스]`. gdb 기록의 L188 breakpoint에 run마다 24회 도달, DESCRY.md 2026-10-16 재계수) failure point 성공 기준을 세울 수 없음. 거부 분기는 패킷 생성(`EVS_GenerateEventTelemetry`, L204)보다 앞이라 C1 Q6의 port 출력 경로도 실행되지 않음 `[I 소스]`. 목표 완화(p.4)는 메시지 출력 줄을 그 앞 검사 줄로 바꾸는 장치이고 처음부터 메시지가 없는 경우의 처리는 서술 없음. pair 기준 e_f("실패 지점에 가장 가까운 사건", p.7)도 고를 기준이 서술상 없음 |

#### 2.4.6 SimEvo (근거: ICSME 2017 전문. 사건·자원 모델 세부는 RacePro·SimRacer에 위임)

분석 칸의 공통 전제(SimEvo.md §3.1): (a) 사례의 접근이 SimEvo의 사건 집합(syscall 효과와 다섯 동기화 연산)에 들어 있어야 한다. (b) 사례의 task들은 한 Linux 프로세스 안의 pthread이고 SimEvo의 PuT는 프로세스이다. 한 프로세스 안의 cFS app/task 쌍을 PuT 쌍으로 줄 수 있는지는 서술이 없다. 구현이 서술보다 넓거나 좁을 수 있다. 어느 칸도 "SimEvo가 사례를 탐지하지 못한다"는 뜻이 아니다. 회귀 버전 쌍 후보(SimEvo.md §3.2): C1 CF `281a941` → `833fdbb`는 결정 D22 [D]이고 사실이 아니다. R2 `b9bec567^` → `b9bec567`은 결정 P-R2CTRL (c2) [D]이다. C2 cFE 6.5.0a → 6.6.0a는 수정과 #198의 연결이 추론이다(기록). C3·C5 cFE 6.4.1 → 6.4.2는 공개 git 객체가 없고 tarball 차이로만 관측된다(기록). **Q1을 "분석"으로 두는 이유(SimEvo.md §3.1, 2026-10-16):** SimRacer.md는 SimRacer 본문이 없어 같은 칸을 "불명"으로 바꿨지만, SimEvo는 본문을 전부 읽었고 본문이 사건 종류(p.3)와 탐색이 맞추는 Def-Use 패턴(system call write·read, p.6)을 스스로 정의한다. 그래서 "사례의 공개·사용이 그 정의에 들어 있는가"는 분석할 수 있다. 구현의 범위는 위임돼 있어(p.3) 서술보다 넓을 수 있다. Q2는 본문이 식별 기준을 정하지 않으므로 불명이다.

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | 분석 — 필요한 사건은 이름 등록 완료, `OS_CountSemGetIdByName` 호출·반환값, 재시도 지연, CF 초기화의 최종 결과. 서술된 사건 종류에 이름 등록·조회가 없음(해당 호출은 syscall 0건 `[M-probe]`, OSAL `d2d877a`·경합 없음). 재시도 지연은 syscall로 보이나 SimEvo가 sleep을 사건 종류로 서술하지 않음. 기반 SimRacer의 메모리 관측 여부 불명 | **불명** — 이름 ↔ ID ↔ 슬롯, 세대 서술 없음. 본문의 식별 장치는 test input 파일 이름 통일뿐 | 분석 — 표현 수단은 다섯 partial order뿐이고 생성 → 조회는 그 안에 없음. 쌍이 사건으로 관측된다면 접근 쌍으로 다뤄질 수 있으나 예약/공개 완료·재시도 중/최종 실패 구분 개념 없음 | 답 — readiness precondition·허용 오류를 넣을 입력 없음. 재시도는 S′의 코드 변경으로만 들어감 | 분석 — 구조는 맞음: 수정 전·후 소스 쌍, 변경 문장의 forward slice로 영향 syscall 선택, 관측 trace에서 쌍 뒤집기·재생. 단 생성·조회가 영향 집합과 trace의 사건이어야 함. 회복과 최종 실패를 구분하는 서술 없음 | 분석 — 출력은 T∆, C, F. 현상이 어떤 출력으로 나타나는지는 cFS 실행에서만 정해짐 |
| C2 | 불명 — 앱 소스 비공개 | 불명 | 불명 — 공개된 cFE 쪽 장치(`WaitForStartupSync`, `APPS_INIT`, `LATE_INIT`)와 같은 종류의 상태 관계는 다섯 관계에 없음. 이 사실은 대상 앱의 관계를 표현하는지에 대한 답이 아님 | 답 | 불명 — 소스 쌍은 줄 수 있으나 P·R이 없어 impacted 접근 trace가 정해지지 않음 | 불명 |
| C3 | 분석 — AppId 저장, 등록 플래그, `CreatePipe` 안의 이벤트 전송은 메모리 연산·함수 호출. task 생성은 `clone3`. 서술된 사건 종류에 AppId·등록 플래그 없음 | **불명** — SB AppId, EVS 자신의 AppId, 등록 플래그, ES `TaskTable`은 서로 다른 필드이며 구분하거나 잇는 서술 없음 | 분석 — 수정의 "publish-last"와 core task별 시작 barrier는 다섯 partial order 밖. 설정·사용 쌍이 관측된다면 접근 쌍으로 다뤄질 수 있음 | 답 | 분석 — S = 6.4.1, S′ = 6.4.2의 소스 쌍은 공개 git 객체 없이 tarball로만 얻을 수 있음. 입력 모양은 맞음. 적용은 사건이 영향 집합에 들어가는지에 달림 | 분석 — 현상은 범위 밖 접근과 이어지는 crash(오류 지점 `unspecified_by_reference`). crash를 오라클이 어떻게 받는지 서술 없음 |
| C5 | 분석 — `OS_TaskCreate`(`clone3`), ES lock 재획득 후 `TaskTable` 기록, 새 task의 `GetAppID` 조회·반환값. task 생성이 SimEvo가 다루는 fork와 같은 사건인지 서술 없음. ES 레코드 기록·조회와 lock은 사건 종류에 없음 | **불명** — OSAL task 인덱스와 AppId를 잇는 서술 없음 | 분석 — fork-return order는 fork와 자식 return 사이의 순서이고, 부모가 task 생성 뒤에 하는 등록을 자식의 조회보다 앞세우는 관계는 다섯 가운데 없음. 수정은 lock 범위 확장이고 lock은 사건 종류에 없음 | 답 | 분석 — 소스 쌍은 tarball 차이뿐. 변경은 mutex 사용 범위인데 동기화 연산 규칙은 fork, wait, pipe, signal뿐(p.5). lock 변경이 영향 집합에 들어가는지 서술 없음 | 분석 — 현상은 `GetAppID`가 `CFE_ES_ERR_APPID`를 돌려주는 "등록 전 조회". SimEvo의 출력은 오라클 불일치 결함이며 이 반환값과의 대응 경로 서술 없음 |
| R2 | 분석 — 모두 메모리 연산과 함수 호출이며 서술된 사건 종류에 없음 | **불명** — `CFE_SB_Global.AppId`와 EVS 등록 정보를 서로 다른 상태로 구분하는 서술 없음 | 분석 — 시작 직렬화와 "저장 후 사용"은 다섯 partial order 밖. 저장·사용 쌍이 관측된다면 접근 쌍으로 다뤄질 수 있음 | 답 | 분석 — 자연 회귀 쌍은 core 시작 순서를 바꾼 변경(CMake 목록). SimEvo의 변경 집합은 소스 diff·AST 비교로 서술되며 CMake 목록이 들어가는지 서술 없음. 동적 단계의 DSC는 다섯 관계의 차이만 비교하므로 시작 직렬화의 변화는 그 관계에 담기지 않음 | 분석 — 이벤트 요청 거부(`ILLEGAL_APP_ID`)이고 출력 메시지가 없음. 오라클이 콘솔·파일 출력 비교인 구조(p.8)와의 대응 경로 서술 없음. 탐지 결과가 아님 |

#### 2.4.7 ReDPro (근거: 초록과 2차 문장뿐. 본문 미도달)

ReDPro.md §3.1·§3.6: 이 문서는 전문을 읽지 못했으므로 "답"은 0칸이다. "분석" 네 칸(Q5)도 초록의 설계 진술에 근거한 **조건부** 서술이며 "ReDPro가 이 사례를 다룬다"가 아니라 "설계에 순서 제어와 재생성 단계가 있다"를 뜻한다. 사례의 의존 pair가 ReDPro의 "potential race pair"가 되는지는 Q1–Q3가 불명이라 알 수 없다. 쓸 수 없는 문장: "ReDPro는 syscall만 관측한다", "ReDPro는 readiness precondition을 표현하지 못한다", "ReDPro는 같은 프로세스 안의 thread를 다루지 못한다". ReDPro를 탐지 표·재현 표의 확정 행으로 넣지 않는다. 2026-10-16에 더한 2차 서술(SysPro p.22, pp.34–35)은 ReDPro를 자동 **재현** 접근들의 묶음에 넣고 그 묶음이 버그 보고서를 주 입력으로 삼지 않는다고 적는다. 라벨은 바뀌지 않았다. Q6이 도구 수준(§2.2)에서는 "분석", 사례별로는 "불명"인 이유는 ReDPro.md §3.4가 적는다. 도구 수준은 "출력의 종류"만 묻고, 사례별은 "원 사례의 현상과 대응하는가"까지 묻는데 failure 정의와 보고 형식이 자료에 없기 때문이다.

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | 불명 — 감시 대상이 syscall뿐이라면 이름 등록·조회(syscall 0건 `[M-probe]`, OSAL `d2d877a`·경합 없음)는 입력에 안 나타나고 메모리 접근까지 감시한다면 나타날 수 있음 [I]. 어느 쪽인지 정해지지 않음 | 불명 | 불명 | 불명 — 허용 오류와 필요 순서를 입력하는 인터페이스가 있는지 알 수 없음 | 분석(조건부) — 설계에 순서 제어와 failure 재생성 단계가 있음(초록). 생성↔조회가 potential race pair가 되는지는 알 수 없음 | 불명 — ReDPro의 failure 정의와 출력 형식을 알 수 없음 |
| C2 | 불명 | 불명 | 불명 | 불명 | 불명 — 앵커 pair를 알 수 없고 대상 코드가 비공개 | 불명 |
| C3 | 불명 — 함수 호출과 전역 메모리 읽기·쓰기. 감시 대상 종류가 불명이라 수집 여부를 판단할 수 없음 | 불명 | 불명 | 불명 | 분석(조건부) | 불명 — crash가 일어나는지 자체가 사례 기록상 미확인 |
| C5 | 불명 | 불명 | 불명 | 불명 — 반환값을 보는 failure 기준을 입력으로 받는지 알 수 없음 | 분석(조건부) | 불명 |
| R2 | 불명 | 불명 | 불명 | 불명 | 분석(조건부) — R2의 순서가 고정되는 이유는 core 시작 직렬화(사례 기록)이며, pair를 뒤집을 여지가 있는지는 R2가 race인지 고정 순서인지를 실행으로 가려야 알 수 있음 | 불명 — ReDPro의 "failure"가 프로세스 실패인지 사용자가 정한 oracle인지에 따라 대응 여부가 달라짐 [I] |

#### 2.4.8 SysPro (근거: arXiv v1 전문)

분석 칸의 공통 전제(SysPro.md §3.1): **전제 1(제어·관측 단위)** SysPro가 다루는 위치는 syscall의 소스 호출 지점이며, 구현(ReproSys)을 읽지 못해 실제 계측 지점이 syscall 이름 목록에 한정되는지는 확인하지 못했다. 이 문서의 분석 칸은 논문에 서술된 단위에 대한 것이다. **전제 2(실행 단위)** 한 프로세스 안의 task 쌍을 줄 수 있는지는 서술이 없다. **전제 3(입력)** bug report(subject, body), 대상 소스, system call manual, 응용 이름·프로세스 집합에 대한 사전 지식, test case가 있어야 한다. 다섯 보고서 본문에는 Linux system call 이름이 없다(기록). 어느 칸도 "SysPro가 사례를 재현하지 못한다"는 뜻이 아니다.

| 사례 | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| C1 | 분석 — SysPro는 사건을 수집하지 않음. 사례의 사건(이름 등록·조회와 반환값)은 syscall 0건 `[M-probe]`(OSAL `d2d877a`, 경합 없음, C1의 OSAL `42af0f73` 아님)이라 SysPro가 제어하는 syscall 호출 지점이 직접 계측 지점이 되지 않음. 보고서가 주는 사람용 방법("provider의 시작을 지연")과 닮은 지연을 어느 syscall 앞뒤에 둘 수 있는지, 1단계가 그 위치를 고르는지는 불명 | 분석 — 이름 → ID → slot 장치와 세대 장치 서술 없음 | 분석 — "생성 → 사용"은 syscall 쌍의 순서로 표현될 수 있는 모양이나 두 사건이 syscall이 아님 `[M-probe]`. 예약/공개 완료/재시도 구분 개념 없음("retry" 0건) | 답 — 필요 순서와 허용 오류를 넣을 입력 없음 | 분석 — 제어는 후보 위치에 sleep을 넣는 것. C1의 나쁜 순서는 provider 쪽 지연으로 만들 수 있는 종류(보고서 본문도 같은 방법을 적음)이나 SysPro가 그 위치를 계측 대상으로 고를 수 있는가는 불명. P-C1HOLD(생성 직전 보류, 사건 기반 해제)에 해당하는 서술 없음. sleep 길이 서술이 없어 시간 기반으로 읽힘 | 분석 — 출력은 후보 위치 순위와 재현 성공 시 schedule. 재현 판정 방법을 논문이 쓰지 않아 C1 현상(초기화 실패 이벤트·`APP_ERROR` 종료)과의 대응은 불명. 수정 후 재시도 회복 경로가 재현으로 세어지는지 서술 없음 |
| C2 | 불명 — 대상 응용 소스가 비공개 | 불명 | **분석** — 도구 쪽 서술만으로: 관계가 syscall 쌍의 순서이고 "초기화 완료"·시작 동기화 통과 개념이 없음. 사례 쪽은 불명 | 답 — barrier 통과와 앱 고유 초기화 완료를 구분하는 입력 없음 | 불명 — 후보 위치가 정해지지 않음 | 불명 |
| C3 | 분석 — 사건 수집 없음. AppId 저장·등록 플래그·TIME의 이벤트 요청·범위 검사 없는 접근은 메모리 `[I 소스]`. `mq_open`은 보이나 `[M-probe]` 의존 상태가 아님. 보고서의 함수·변수 이름은 파일 순위 질의에 쓰일 수 있으나 그 파일에서 syscall 이름이 계측 지점이 되는지는 불명 | 분석 — 필드 단위 식별 서술 없음 | 분석 — 초기화 완료 뒤에야 이벤트 요청이 유효하다는 관계를 표현할 장치 없음. 순서는 우선순위가 만듦. SysPro의 순서 모델은 syscall 쌍 | 답 | 분석 — 우선순위로 정해진 순서와 sleep 삽입이 맞물리는 서술 없음("priority", "scheduler", "kernel" 0건). 지연이 SB thread 쪽(AppId 저장 전)에 놓이면 나쁜 순서 쪽으로 작용한다는 소스 분석이 있으나(기록) 미실행. 원 플랫폼 Microblaze와 PIN 대상 ISA의 관계 불명. 실행 가능한 형태는 파생 구성(32-bit x86, `deviation`) | 분석 — 현상은 segfault(cFE core의 crash). 재현 판정 방법 서술이 없어 crash를 재현으로 세는지 불명 |
| C5 | 분석 — 사건 수집 없음. syscall로 읽히는 것은 `clone3`뿐(SysPro.md는 `pthread_create` → `clone3`를 에이전트 C 프로그램의 기록으로, `OS_TaskCreate` → `pthread_create`를 `[I 소스]`로 적음. OSAL 시험 기록은 §0.1 (4)). 등록 쓰기·조회는 메모리 `[I 소스]`, ES lock 경합 시 futex 가능. 보고서 본문은 `clone`, `fork`, `pthread_create`를 이름 붙이지 않음. thread 생성의 취급은 서술 없음(전제 2) | 분석 — OS task ID ↔ AppId ↔ `TaskTable` slot 대응과 "등록 전" 상태를 식별하는 장치 서술 없음 | 분석 — 생성 한 조각만 syscall. "등록 전 조회" 표현에는 등록 사건과 조회의 반환 결과가 필요한데 SysPro의 순서 모델은 syscall 쌍 | 답 | 분석 — 참조 OSAL에서 `clone3` 직후 지연이 child를 막고 일정 지연을 넘으면 child가 `OS_ERR_OBJECT_IN_USE`로 시작하지 못한다는 소스 분석이 있음(기록, 미실행, 수치는 SysPro.md §3.5). 사례가 필요로 하는 과거 OSAL·cFE 판본은 검토하지 않아 호출 앞·뒤 지연이 이 창에 닿는지 불명 | 분석 — 현상은 새 task의 첫 조회 실패(`CFE_ES_ERR_APPID`). 재현 판정 방법 서술이 없어 대응 불명 |
| R2 | 분석 — 사건 수집 없음. `mq_open`만 보이고(`[I 소스]`) AppId 저장·반환값은 안 보임(`[I 소스]`). 보고서 본문의 빌드 명령·breakpoint·gdb 실행 단계를 SysPro의 정규식이 command/options/input data로 읽는지 불명 | 분석 — 식별 서술 없음 | 분석 — 관계를 표현할 장치 없음. 저장은 syscall이 아님 `[I 소스]` | 답 | 분석 — 고정 순서라면 기본 실행에서 이미 나쁜 순서가 나오므로 지연 삽입이 새 순서를 만드는 역할이 없고, SysPro의 서술은 그런 경우를 따로 다루지 않음. 스케줄 의존이라면 SB 쪽 지연이 나쁜 순서를 늘리는 방향(기록). 어느 쪽인지 정해지기 전에는 불명 | 분석 — 현상은 거부 + 무출력 + 반환값 미사용. 재현 판정 방법 서술이 없고 이 현상에는 crash·출력·종료 상태가 없어 대응 불명 |

### 2.5 도구 간 라벨 불일치와 읽을 때 주의

| # | 항목 | 내용 |
|---|---|---|
| 1 | SimRacer와 SIMEXPLORER의 Q1·Q2 | 2026-10-15판에서는 C1·C5의 Q2만 달랐다(SimRacer 분석, SIMEXPLORER 불명). 2026-10-16에 SimRacer.md가 C1·C3·C5·R2의 Q1·Q2를 모두 "불명"으로 바꿔 그 차이는 없어졌다(SimRacer.md §1.3 항목 9). 대신 C1·C5의 Q1과 C3·R2의 Q1·Q2, 모두 6칸이 다르다(SimRacer 불명, SIMEXPLORER 분석). 대상이 다르기 때문이다. SimRacer.md의 라벨은 ISSTA'13의 SimRacer 자체에 대한 것이고 그 본문이 없으며, SIMEXPLORER.md의 라벨은 STVR이 서술하는 도구 자체에 대한 것이다. SimRacer.md는 같은 STVR 설계 분석을 라벨 없이 병기한다. 이 문서는 두 라벨을 그대로 두고 합치지 않는다 |
| 2 | SimEvo의 Q2 | C1·C3·C5·R2 모두 불명이다. 자원 식별 모델을 RacePro [26]·SimRacer [57]에 위임했기 때문이다. 다른 도구의 "분석"과 같은 강도의 비교가 아니다 |
| 3 | DESCRY·SysPro의 C2 Q3 | 두 도구만 분석이다. "관계가 5종/syscall 쌍으로 닫혀 있고 초기화 완료 개념이 없다"는 도구 쪽 서술만으로 말했기 때문이다. 사례 쪽은 두 파일 모두 불명이라고 적는다. 다른 도구의 C2 Q3 불명과 모순이 아니라 쓴 근거가 다르다 |
| 4 | ReDPro의 Q6 | ReDPro.md §3.3(도구 수준)은 Q6을 분석으로, §3.4(사례별)는 모든 사례의 Q6을 불명으로 둔다. 2026-10-16에 ReDPro.md §3.4가 이유를 적었다. 도구 수준 Q6은 "출력의 종류"만 묻고 초록이 race 탐지와 failure 재생성을 서술하므로 분석이다. 사례별 Q6은 "원 사례의 현상과 대응하는가"까지 묻는데 failure 정의와 보고 형식이 자료에 없어 불명이다. 이 문서는 사례별 라벨을 쓴다 |
| 5 | Q1의 "분석" | 도구마다 뜻이 다르다. RacePro·RRF·DESCRY·SysPro는 "그 사건이 입력 사건 집합에 들어오는 경로를 찾지 못했다"이고, SIMEXPLORER는 "SR로 지정되면 메모리 중단점으로 잡히는 관측 대상이며 지정 경로·입도는 서술이 없다"이고, SimEvo는 "본문이 정의한 사건 종류(syscall 효과와 다섯 동기화 연산)에 들어 있는가"이다. 라벨이 같아도 같은 내용이 아니다 |
| 6 | Q4의 "답" | 다섯 사례에 같은 라벨이다. 도구 수준의 답이 사례와 무관하기 때문이다. ReDPro만 불명이다 |
| 7 | SimRacer 칸의 한계 | ISSTA'13 본문을 읽지 못했으므로 SimRacer 행은 "STVR 2017의 서술이 SimRacer에 해당하는 범위"로 한정된다. 세 가지가 불명이다. (a) 메모리 관측이 SimRacer에 있었는지(STVR p.8은 선행 연구로 SimRacer [16]과 SimTester [33]를 함께 인용. SimTester는 응용과 ISR이 함께 쓰는 공유 변수에 메모리 중단점을 건다, VEE'12 p.54 `2차`). (b) 탐색이 결정적 뒤집기인지(SimEvo p.8은 "randomly explores"라고 서술). (c) 사건의 층위: SCMiner p.11 `2차`와 같은 해의 Sim-O/C(ITJ p.188 `2차`)는 process 사이 시험을 system call 쌍의 순서 바꾸기로 서술한다. 이는 STVR이 shared memory와 hardware를 SR로 열거하는 서술과 층위가 다르다. Sim-O/C는 메모리 관측을 배제하지 않고(관측기가 BasicConfig의 공유 메모리 주소를 다른 프로그램도 접근할 수 있다고 적음, ITJ p.185) 메모리 중단점의 예는 응용과 ISR 사이다. 어느 쪽이 SimRacer를 더 정확히 말하는지 정해지지 않는다(SimRacer.md §2.3). Sim-O/C와 SimRacer가 같은 도구의 서술이라는 판단 자체가 [I]다(Sim-O/C 본문에 SimRacer라는 이름이 없음) |
| 8 | 이전 저장소 서술과의 충돌 | `baselines/availability.md` §5.4의 허용 문구 "각 도구의 관측 모델 분석에서 다섯 사례의 의존성은 그 모델의 사건이 아니다"는 SIMEXPLORER에는 쓸 수 없고(SIMEXPLORER.md §0 항목 4), SimRacer에는 쓰지 않는다. 반박도 단정도 하지 않는다(SimRacer.md §0 항목 6 보정, §3.6). DESCRY·SimEvo에는 "서술된 사건 집합에 없다"로 한정해야 한다(DESCRY.md §1.3 행 4, SimEvo.md §1.3). 쓰려면 도구별로 한정한다 |
| 9 | `OS_TaskCreate` → `clone3`의 근거 (2026-10-16) | DESCRY.md는 이것을 `[M-probe 기록]` (3)으로 적는다(OSAL 자체 시험, `osal-visibility.md` probe 표). SysPro.md(§1.3 항목 17), RacePro.md(C5 보충표), SimRacer.md(C5 보충표)는 에이전트가 쓴 작은 C 프로그램의 기록(`availability.md` §4.5 끝)으로 적고, SysPro.md는 "OSAL 시험이 아니다"라고 바로잡았다고 적는다. 이 문서는 저장소 기록을 다시 읽었다. `osal-visibility.md`의 "Stack attribution" 표에 `OS_TaskCreate` 행이 있고 run마다 `clone3` ×4(이름 없음)를 적는다. `availability.md` §4.5 끝은 DESCRY·SysPro 보고서의 C 프로그램도 `pthread_create` → `clone3`를 보였고 OSAL probe가 그것을 대신한다고 적는다. 둘 다 기록이다. 이 문서는 §0.1 (4)로 OSAL 시험의 기록을 쓴다. 어느 쪽이든 OSAL `d2d877a`이고 C5의 OSAL 4.1.1 시대가 아니며 cFS 실행이 아니다. SysPro.md·RacePro.md·SimRacer.md의 해당 문장은 그 파일 담당이 고칠지 정한다(§5.5) |
| 10 | R2 ④의 "syscall 없음" 근거 (2026-10-16) | RRF.md는 R2 ④와 Q1 칸에 `results/R2_gdb_v701.md` §2의 "Nothing was printed, no counter changed, and no syscall was made"를 인용했다. DESCRY.md와 SysPro.md는 같은 날 그 문장을 근거로 쓰지 않는다고 적는다. 이 문서는 §0.1과 §5.5의 규칙대로 gdb 기록을 syscall 근거로 쓰지 않으며 R2의 "syscall 없음"을 `[I 소스]`로 쓴다. "출력 없음·카운터 변화 없음"은 gdb 기록이 관측한 범위에 드는 진술로 인용할 수 있다 |
| 11 | SimRacer Q1 불명과 SimEvo Q1 분석 (2026-10-16) | 같은 계보인데 라벨이 다르다. SimEvo.md §3.1이 이유를 적는다. SimEvo는 본문을 전부 읽었고 본문이 사건 종류와 Def-Use 패턴을 정의하므로 분석할 수 있고, SimRacer는 본문이 없다. 모순이 아니라 근거의 차이다 |

## 3. 결과 상태표 (로드맵 §10.4)

### 3.1 상태 정의와 오늘의 칸 수

| 상태 | 의미 | 결과 집계 | 오늘 부여된 칸 |
|---|---|---|---:|
| `NOT_RUN` | 환경·자료·결정 등으로 실행하지 못함 | 미탐지 수에 넣지 않음 | **40** |
| `INAPPLICABLE` | 원 기법의 명시된 입력·대상 범위에 해당하지 않음 | 이유·원문 위치를 보고 | 0 |
| `PATH_NOT_EXECUTED` | 도구는 실행했으나 목표 현상이 실행되지 않음 | 탐지 능력 판정 보류 | 0 |
| `OBSERVATION_GAP` | 목표 사건이 실행됐지만 필요한 정보가 기록되지 않음 | 관측의 부족으로 보고 | 0 |
| `ANALYSIS_GAP` | 필요한 사건이 기록됐지만 의존성·후보를 만들지 못함 | 해당 입력·기준에서 분석의 부족으로 보고 | 0 |
| `REPORTED` | 관련 후보·재현·오류를 출력함 | 원 사례·출력의 정확한 대응과 실제 근거 확인 | 0 |

`NOT_RUN` 40칸은 도구별 파일이 `NOT_RUN`을 적용한 상태의 수다. §5.4 항목 1(같은 프로세스 안 task 쌍)과 항목 14(C2 입력 미충족)가 열려 있어 채택에 따라 일부 칸이 `INAPPLICABLE`로 바뀔 수 있다.

`PATH_NOT_EXECUTED` 이하 네 상태는 도구를 실행한 뒤에만 쓸 수 있다. 이 문서도 어느 것도 예측하지 않는다. 사건 기록의 기준이 부족한 경우를 `ANALYSIS_GAP`으로 서둘러 분류하지 않는다(로드맵 §10.4).

### 3.2 도구 × 사례 오늘 상태

| 도구 | C1 | C2 | C3 | C5 | R2 | 차단 요인(도구별 파일) |
|---|---|---|---|---|---|---|
| RacePro | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **환경** — Scribe 내장 i386 Linux 2.6.35 커널을 이 컨테이너에서 부팅할 수 없음(2026-10-16 재측정에서도 같음). cFS도 그 userland에서 빌드된 적이 없음. 자료(논문·코드)는 열려 있음 |
| SimRacer | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **자료** — 구현 없음. Simics와 원 구성 없음 |
| SIMEXPLORER | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **자료** — 구현 없음. Simics 없음. 결정 — cFS용 SR·oracle 입력, 저자 요청 미정 |
| RRF | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **자료** — 구현 없음. **환경** — Scribe 환경 차단 |
| DESCRY | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **자료** — 구현 없음. KLEE·Simics 없음. 실패 로그 미확인 |
| SimEvo | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **자료** — 구현 없음(SimRacer도). Simics·CodeSurfer 없음. 입력(T, 오라클) 미정 |
| ReDPro | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **자료** — 구현·전문 없음, PIN 판본 미정. `availability.md`의 verdict로는 `material_unavailable`. 역할 표에 행을 넣지 않음. PIN 설치를 정하더라도 2026-10-16의 디스크 여유(1.3 GB)가 먼저 문제가 됨(ReDPro.md §4.1) |
| SysPro | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | **자료** — 구현(ReproSys) 미도달(2026-10-16 `add_repo` 거절 포함). PIN 등 도구 미설치·판본 미정. sleep 길이·판정 방법 서술 없음 |

사례별 추가 사유(도구별 파일):

| 사례 | 추가 사유 |
|---|---|
| C1 | 실패 출력과 빈도가 `unspecified_by_reference`. 공개 BP 구성이 있어 소스는 마련될 수 있으나(D22 [D]) 과거 전체 버전 조합은 별도 |
| C2 | 앱 P·R 소스가 비공개(EVA CWS). 가능한 것은 역할 재구성뿐이며 그 결과는 "C2 메커니즘의 역할 기반 검증"이고 원 비공개 시스템의 재현이 아님(P-C2 [D]). RRF·DESCRY·SimEvo·SysPro는 입력 미충족 사유가 추가됨 |
| C3 | 원 플랫폼 Microblaze의 OS 미확인. 파생 구성(32-bit x86 pc-linux)은 `deviation`. 6.4.1→6.4.2는 공개 git 객체 없이 tarball 차이로만 관측됨 |
| C5 | C3과 같은 tarball 사정. 과거 OSAL·cFE 판본 대응 실패와 도구 실패를 구분함(D13·Q16 [D]) |
| R2 | 병합된 수정이 없고 자연 쌍(`b9bec567^` → `b9bec567`)을 씀(P-R2CTRL (c2) [D]). 고정 순서인지 스케줄 의존인지 사례 기록상 미결 |

### 3.3 `INAPPLICABLE` 후보 근거 위치 (PI 판단. 이 문서가 붙이지 않았다)

`INAPPLICABLE`은 이유와 원문 위치를 보고해야 한다. 도구별 파일은 모두 이 상태를 붙이지 않았다.

| 도구 | 근거 후보(원문 위치) | 반대 근거 | 파일의 기본값 |
|---|---|---|---|
| RacePro | 같은 프로세스 thread 사이 OS 자원 race에 "less effective"(p.362) | 논문이 thread를 기록 대상에 넣음(p.354, p.356). PI의 Process-Level 정의가 앱·task 단위 | 아니오(`NOT_RUN` 유지) — RacePro.md P-2 |
| SimRacer | 대상 원천(STVR p.7), PuT 쌍 입력(p.16). Sim-O/C(ITJ p.185 `2차`)는 Simics 콜백만으로 process와 thread를 구별하지 못하고 process tracker가 user-space process를 구별한다고 적음(thread 단위 구별 방법은 그 쪽에 없음). SimRacer.md는 이것을 판단 자료로만 올리고 근거로 쓰지 않음 | thread를 PID로 추적한다는 한 줄(p.13) | 아니오 — SimRacer.md P-2 |
| SIMEXPLORER | 범위(p.2–3, p.7). 단일 process 안의 multithreaded 응용을 기존 thread-level 기법의 영역으로 둠(p.1, p.4–5, 2026-10-16 추가) | thread를 PID로 추적(p.13), SR에 shared memory(p.8). thread 사이 race를 범위에서 뺀다는 명시 문장은 없음 | 아니오 — SIMEXPLORER.md P-S2(반대 방향 서술도 판단 자료로 함께 올림) |
| RRF | 대상은 "두 프로세스가 OS 자원에 접근하는 race"(p.1–2), "operating at the thread level"인 탐지기는 이런 race를 다루지 못할 수 있다고 씀(p.1), thread 수준 기법을 다른 범주로 서술(p.1, p.10), 평가 범위 file·socket(p.6) | 로드맵 §1.2의 운용 정의. thread 사이 race를 범위에서 뺀다는 명시 문장은 없음(2026-10-16 재확인) | 기본값 선언 없음("보지 않으면 `NOT_RUN`") — RRF.md §4.3-1. 이 파일은 오늘 상태에 `NOT_RUN`을 적용. C2의 PuD 소스 요구(RRF p.5)와 앱 소스 비공개를 적었으나 (c)안으로 올리지 않음 |
| DESCRY | 범위 서술(p.1), PuD 규칙(p.4) | 단일 thread 가정이 설명 편의라는 서술(p.3) | (a) `NOT_RUN` 적용 — DESCRY.md §4.3-1. 같은 항목에 (c) C2만 입력 미충족(PuD 소스 비공개, DESCRY p.1, p.3)으로 별도 처리하는 안이 있음 |
| SimEvo | 명시 대상은 multi-process와 handler(p.1, p.7) | 계보 STVR p.13이 thread를 PID로 추적한다고 서술(SIMEXPLORER 서술이며 SimEvo 구현은 불명) | 문서가 정하지 못함 — SimEvo.md §4.3-1. 오늘 상태에는 `NOT_RUN`을 적었다. C2의 앱 소스 비공개를 적었으나 (c)안으로 올리지 않음 |
| ReDPro | **붙일 수 없음**. 전문을 읽지 못해 대상 범위의 서술과 원문 위치를 보고할 수 없음. 초록의 "multiple processes"만으로 범위 밖이라 결론짓지 않음 | — | `NOT_RUN` — ReDPro.md §4.2 |
| SysPro | inter-process 범위(p.2–3), 프로세스 집합 입력(p.4–5) | thread 처리가 서술되지 않았다는 점, 로드맵 §1.2 | (a) `NOT_RUN` 적용 — SysPro.md §4.3-1. 같은 항목에 (c) C2만 입력 미충족(대상 응용 소스 비공개, SysPro.md Y8)으로 별도 처리하는 안이 있음 |

표의 "파일의 기본값"은 같은 프로세스 안 task 쌍을 `INAPPLICABLE`로 볼지(§5.4 항목 1)에 대한 값이다. C2의 입력 미충족(앱 소스 비공개)을 `INAPPLICABLE`로 볼지는 별도 PI 판단이다(§5.4 항목 14).

## 4. 부족 표 — Kairos 기능 후보 연결 (로드맵 §10.5)

### 4.1 "확인된 부족"의 정의와 오늘의 결과

로드맵 §10.5는 "실제 기존 도구의 부족이 확인된 행에 대해서만 비교 결과와 연결한다. 이미 가능한 기능은 재발명한 차별점으로 주장하지 않는다"고 한다. 이 문서는 **확인된 부족**을 `ORIGINAL-RUN` 또는 `DOCUMENTED-SETUP-RUN`에서 `OBSERVATION_GAP` 또는 `ANALYSIS_GAP`에 도달한 칸으로 정의한다([제안]). `ANALYSIS_GAP`은 로드맵 §10.4대로 "해당 입력·기준에서" 보고하므로 입력과 기준을 함께 적는다. `DEVIATION-RUN`(문서화된 요구를 대체한 실행)에서 나온 두 상태는 "deviation 조건 한정 부족"으로, `REIMPLEMENTATION`·`EXTENSION`에서 나온 것은 "재구현 한정 부족"·"확장 한정 부족"으로 따로 표기하고 원 도구의 부족으로 승격하지 않는다. 대체한 요구(예: RacePro의 QEMU TCG 에뮬레이션이나 `-m32` userland, `availability.md` §5.2)가 부족의 원인일 수 있기 때문이다(§5.3 S6). 두 상태는 도구를 실행한 뒤에만 쓴다(§3.1). **오늘 확인된 부족은 0건이다.** 따라서 §10.5의 여섯 행 모두 비교 결과로는 주장하지 않는다. 도구별 파일이 올린 가설은 "후보 연결"로만 옮기며, 가설은 읽은 설계 기술에서 도출한 분석이고 "X가 못 한다"는 결과가 아니다(도구별 파일이 같은 취지를 적는다).

원출처가 스스로 밝힌 범위·한계는 [F]이다. 예를 들어 RacePro는 HB 그래프가 공유 메모리에 의한 프로세스 간 의존을 담지 않는다고 밝히고(p.358), STVR은 semaphore와 lock을 HB 규칙에 넣지 않았다고 밝힌다(p.9). 그러나 이것이 다섯 사례에서 부족으로 나타나는지는 실행 전에 알 수 없다. 그래서 [F]인 서술과 확인된 부족을 구분한다.

도구별 파일의 가설 ID가 겹친다. SimEvo.md의 H-SE-1~4와 SIMEXPLORER.md의 H-SE-1~4가 같은 번호이고 내용이 다르다. 이 문서는 `SIMEXPLORER/H-SE-n`, `SimEvo/H-SE-n`으로 구분해 쓴다. RRF.md는 H-RRF-1, 3, 4, 5만 있고 H-RRF-2는 결번이다(이름·ID·세대 항목은 §5.3에서 가설로 올리지 않음).

### 4.2 §10.5 행 × 도구 (후보 연결과 강도)

칸의 표기: **가설 ID (대상 사례) — 도구별 파일이 쓴 강도·한계**. "올리지 않음"은 도구별 파일이 근거 부족을 이유로 가설을 올리지 않았다는 뜻이다. "근거 없음"은 파일이 그 행을 차별점으로 쓸 수 없다고 적은 것이다.

| §10.5 행 → Kairos 기능 후보 (확인할 사례) | RacePro | SimRacer | SIMEXPLORER | RRF |
|---|---|---|---|---|
| **1** cFS 내부 사건이 후보 생성 입력에 없음 → 내부 API·상태 변화·값 전달을 관측하고 소스와 연결 (R2·C3·C5) | H-RP-1 (R2, C3, C5, C1) — 분석. OSAL 레지스트리를 자원으로 추가하면 메울 수 있음(EXTENSION). "Kairos만 가능"이 아니라 "원 도구의 입력 모델 밖" | H-SM-1 (R2, C3, C5, C1) — 2026-10-16 보정: SIMEXPLORER 설계에서는 메모리가 관측되므로 "관측 불가능"이 아니고, 지정 인터페이스로 메울 수 있다면 부족은 입력 작성 비용. SimRacer 자체에는 SR에 메모리가 있었는지부터 [U]. Sim-O/C(ITJ p.188 `2차`)가 process 사이 사건을 system call로 적지만 메모리 관측을 배제하지 않으므로 "SimRacer가 메모리를 보지 못한다"로 강하게 쓰지 않음 | `SIMEXPLORER/H-SE-1` (C1, C5, R2, C3) — **근거가 약함**(부재 확인). SR 지정 인터페이스가 있음(p.8) | H-RRF-1 — **부분 근거**. 구현 상세(학위논문)를 읽지 못함 |
| **2** 이름·ID·등록 정보를 같은 자원으로 연결하지 못함 → 이름과 객체 ID·세대, task와 AppId 관계 추적 (C1·C5) | H-RP-2 (C1, C5) — **근거가 약함**. 경로 이름 기반 식별은 이미 있고 세대는 "본문에 없다"는 부재 확인 | H-SM-2 (C1, C5; R2는 값 0 대 유효값) — **근거가 약함**. 서술 없음이 근거 | `SIMEXPLORER/H-SE-1`에 포함 — **근거가 약함** | 올리지 않음(§5.3: 서술 없음이 기능 없음의 근거가 아님) |
| **3** 시작 동기화와 앱 고유 초기화 완료를 혼동 → 시작 상태와 앱 고유 의존 상태를 구분하는 규칙 (C2) | 올리지 않음(C2 자원 비공개. 논문에 startup/readiness 개념이 없다는 것만 확인) | 올리지 않음(같은 이유) | 올리지 않음(설계에 시작 상태·readiness 개념이 서술되지 않았다는 것만) | H-RRF-3이 이 행을 연결(조건부)하나 §5.3은 "C2에서 RRF에 부족한 것은 사례 자료가 없어 말할 근거 없음" → **C2로는 올리지 않음** |
| **4** 오류 반환과 회복 처리를 구분하지 못함 → 반환값·재시도·초기화 최종 결과 추적 (C1) | H-RP-3 (C1; R2, C5) — "부족이라기보다 도구 안에 입력 형식이 없다"에 가까움. 사용자 checker로 구분할 수 있을 수 있음 | H-SM-3 (C1, C5, R2) — 사용자가 내부 oracle을 쓰면 구분할 수 있음(p.18) | `SIMEXPLORER/H-SE-3` (C1, C5, R2) — 능력 부족이 아님 | H-RRF-3 (C1) — **조건부**. oracle 저장소가 확장 가능하므로 사용자 oracle이 하면 Kairos 고유 부족이 아님. H-RRF-1과 묶임. 조건을 가를 내부 oracle 논문([36])은 2026-10-16에 초록만 읽었고("monitoring aspects of internal program and system states") 입력 형식은 초록에 없어 조건은 그대로 |
| **5** 관측된 실행만 다룸 → 관련 작업·자원으로 범위를 좁힌 예측·탐색 및 실행 확인 (C1·C3·C5) | **근거 없음** — 예측·go-live 확인이 이미 있음. 쓰려면 앵커 종류(커널 객체 syscall 쌍)에 한정 | **근거 없음** — 쓰려면 앵커 종류(두 접근 쌍)에 한정 | **근거 없음** — 쓰려면 앵커 종류와 탐색 범위(한 실행 한 쌍)에 한정 | **근거 없음**. H-RRF-4는 제어 방식의 차이(사건 기반 대 sleep)이며 **결핍이 아님** |
| **6** 잘못된 순서가 고정된 이유를 설명하지 못함 → 소스 기반 순서 근거·전제·신뢰 범위를 결과에 연결 (R2) | H-RP-4 (R2) — divergence 필터링도 일종의 근거이고 실제 동작은 미실행. 참고: H-RP-5 (R2, 2026-10-16) — 보고 조건이 "녹화 때 없던 실패"(p.363)라서 녹화에 이미 있는 위반은 그 조건이 겨냥하는 대상이 아니라는 분석. RacePro.md는 §10.5 표에 같은 행이 없고 가장 가까운 행이 이 행이라고 적음. **결함이 아니라 목적(해로운 다른 순서 찾기)의 차이**이며, 원 녹화에 checker를 돌리는 일은 race 검증 절차 밖에서도 할 수 있고, RacePro 녹화에서 현상이 나타나는지는 미실행. C1·C3·C5에는 적용하지 않음 | H-SM-4 (R2, C3) — 서술의 부재 확인. R2 순서가 뒤집힐 수 있는지 사례 기록상 미결. 2026-10-16 보충 [I]: R2 Q5의 도출대로 뒤집기가 교착 해소 뒤 실패하면 그 쌍은 race로 세지 않아 출력에 나타나지 않을 수 있음(실행 결과 아님) | `SIMEXPLORER/H-SE-2` (R2, C5, C3, C1) — lock·시작 직렬화가 HB 규칙에 없음(HB 누락이 곧 실패는 아님). `SIMEXPLORER/H-SE-4` (R2) — 고정 순서 출력 항목 없음(도구 어휘에 대한 것) | H-RRF-5 (R2) — **부분 근거**. R2 순서 성격이 미결이며 반대 순서가 가능하면 약해짐 |

| §10.5 행 | DESCRY | SimEvo | ReDPro | SysPro |
|---|---|---|---|---|
| **1** | H-DESCRY-1 (R2, C3, C5, C1) — "서술된 사건 집합에 없다"까지. "DESCRY가 메모리를 못 본다"로 쓰지 않음 | `SimEvo/H-SE-1` (C1, C3, C5, R2) — 위임 모델과 SimRacer 구현이 어디까지 보는지 불명 | H-RD-1 (R2, C3, C5, C1) — **조건부, 약함**. "관측이 syscall로 한정된다면"이라는 조건이 읽은 자료로 정해지지 않고 PIN은 메모리 접근도 계측할 수 있음. 초록의 문제 정의는 failure가 system call 실행 순서에 민감하다는 것이나(ReDPro.md §0 항목 5) 이것은 PIN이 계측한 사건 종류의 서술이 아님 | H-SYSPRO-1 (R2, C3, C5, C1) — "서술된 제어·입력 단위에 없다"까지. H-SYSPRO-5 (C1, C3, C5, R2; API 수준 앵커) — **약함** |
| **2** | H-DESCRY-2 (C1, C5) — "서술이 없다"가 "기능이 없다"의 확인은 아님 | `SimEvo/H-SE-1`에 포함. Q2는 불명 | 올리지 않음(초록이 자원 식별 방식을 말하지 않음) | H-SYSPRO-3 (C1, C5) — **약함** |
| **3** | H-DESCRY-5 (C2 역할 재구성, C1, C5) — **약함**. C2는 역할 재구성의 메커니즘 검증에만 | `SimEvo/H-SE-2` (C2, C5, C1) — 근거는 p.3뿐. C2 칸은 대부분 불명 | 올리지 않음 | H-SYSPRO-2 (C1, C3, C5, R2, C2 역할 재구성) — 강도 **중간**. publish·use가 syscall이 아닌 경우에 한정 |
| **4** | H-DESCRY-3 (R2, C3; C5는 앱 의존) — "서술된 성공 기준과 현상이 맞지 않는다"까지 | `SimEvo/H-SE-3` (C1) — 근거 p.4, p.8 | 올리지 않음 | H-SYSPRO-4 (R2, C3, C5, C1) — **약함**. 재현과 탐지는 목적이 다름 |
| **5** | **근거 없음** — 차이는 입력 사건과 순서 근거에서 찾아야 함 | **근거 없음** — 좁히는 대상이 "변경으로 영향받은 접근"이라는 점이 SimEvo의 방식 | **근거 반대** — 초록이 순서 제어와 재생성 단계를 서술 | **근거 없음** — 예측이 아니라 순위 탐색이지만 재실행 확인이 있음. "새롭다고 쓰지 않는다" |
| **6** | H-DESCRY-4 (R2) — 근거 **중간**. 재현이 목적인 도구에서 설명은 요구 사항이 아닐 수 있음. 2026-10-16 [I]: S1 아래의 뒤집기 schedule은 진행이 막힐 수 있고 그 처리는 p.7에 서술 없음(실행 결과 아님). DESCRY.md는 사례 쪽 확인 뒤에 쓴다고 적음 | 올리지 않음(회귀 시험이라 설명 항목이 없는 것을 부족으로 읽을 근거 없음). 참고: `SimEvo/H-SE-4` (R2, 구성 수준 변경)는 이 행에 직접 대응하지 않음 | 올리지 않음 | 가설 없음. R2 Q5 칸에 "고정 순서 경우를 따로 다루지 않음"이라는 서술 부재 관찰만 있음 |

### 4.3 도구별로 이미 가능한 기능 — 차별점으로 주장하지 않는다

| 도구 | 파일이 확인한 기존 기능(근거) |
|---|---|
| RacePro | 기록 실행에서 실행되지 않은 순서를 예측하고 branch를 go-live로 확인. 순서위반 겨냥 설계(연구한 race 109건 중 79건이 execution order violation, p.355). 경로 이름 → 부모 inode의 경로별 위치 연결. 벡터 시계 HB. 후보 우선순위. 실패가 확인될 때만 보고하고 결정적 재생. 사용자 정의 checker (§5.1) |
| SimRacer | Phase 1 예측 + Phase 2 실행 확인(초록 `초록`). system call 쌍의 순서를 바꿔 실행으로 확인하는 process 사이 시험(같은 해 같은 저자의 Sim-O/C, ITJ p.188 `2차`, 2026-10-16 추가). 아래 나머지는 STVR의 SIMEXPLORER 서술이며 SimRacer와의 일치는 SimRacer.md §2.3의 범위에서만 말할 수 있다: 거짓 양성을 Phase 2로 걸러냄. race와 fault 구분. 같은 명령어 위치의 반복 접근을 한 쌍으로 합침. 순서위반 계열 겨냥 (§5.1). SR·syscall 추가 지정 인터페이스는 SIMEXPLORER의 서술이고 SimRacer에 있었는지 서술이 없다. 공유 메모리 SR 기록은 SimRacer 자체에 있었는지 불명이라 이 표에서 뺐다(아래 표) |
| SIMEXPLORER | 위와 같음(STVR이 SIMEXPLORER를 서술하므로 그대로 해당) + 공유 메모리 SR load/store 기록(STVR p.8, p.14의 SIMEXPLORER 자신의 서술) + 이벤트 기준 실행 제어(프로세스 멈춤·재개, signal·interrupt 발생; 고정 sleep이 아님), 쌍을 뒤집을 수 없을 때 race로 세지 않는 처리와 기다림 교착 해소 (§5.1) |
| RRF | 보고된 race의 재현. order violation을 race 정의에 포함. 관련 syscall 쌍을 두 순서·네 패턴으로 열거해 실제 실행으로 확인. 한 실행의 관측 순서에 갇히지 않음. 자원으로 거르기. targeted/incidental/new 구분. oracle 확장. 소스 위치 지목 (§5.1) |
| DESCRY | 예측 + replay 확인. POG를 지키며 뒤집고 infeasible을 건너뜀. 로그 메시지를 소스 logging point에 자동 대응. 관련 프로세스 자동 선택. 목표 완화. 사건 수준 우선순위 (§5.1) |
| SimEvo | 버전 쌍 변경 영향 분석(정적·동적). offline 예측 + 재생 확인. 영향받은 쌍으로 탐색 범위 좁힘 (§5.1). 메모리·하드웨어 접근 관측은 SimEvo 구현이 하는지 불명이라 이 표에서 뺐다(아래 표) |
| ReDPro | 초록 수준: 순서를 바꾸는 단계, 실행에서 failure를 다시 만드는 단계, binary instrumentation 기반 run-time 감시 (§5.1) |
| SysPro | 보고서에서 위치 후보를 뽑아 순위화. 순위대로 지연을 넣고 실제 실행해 재현 확인. test case 정규식 추출. syscall 이름이 없을 때 man 페이지 설명으로 후보를 메움. 위치와 schedule 출력 (§5.1) |

**이미 가능하다고 쓰지 않는 것 (파일이 "불명"이라고 적은 것).** 위 표에 넣지 않는다. 가능하다고도 불가능하다고도 쓰지 않는다.

| 도구 | 항목 | 상태 |
|---|---|---|
| SimRacer | 공유 메모리 SR load/store 기록 | STVR p.8이 관측 지원의 선행 연구로 SimRacer [16]과 SimTester [33]을 함께 인용해 SimRacer 자체에 있었는지 [U]. SCMiner p.11 `2차`는 SimRacer를 system call interleaving을 시험하는 도구로 서술(§2.5 항목 7) |
| SimEvo | 메모리·하드웨어 접근 관측 | SimEvo.md §1.4: "SimEvo 구현이 메모리 접근을 관측한다/관측하지 않는다"는 [U]이며 "어느 쪽도 쓰지 않는다". SimEvo 본문에서 load/store 0건. 계보(SIMEXPLORER)에는 서술이 있으나 SimEvo 구현에 해당하는지 불명 |
| ReDPro | PIN이 무엇을 계측하는지 | 초록에 없음 [U] (§2.4.7) |

### 4.4 §10.5 행별 판정

| 행 | 비교 결과로 주장 | 후보 연결을 둘 도구 | 주의 |
|---|---|---|---|
| 1 | **주장하지 않음** (확인된 부족 0건) | 여덟 도구 모두에 가설이 있음. 강도는 부분·약함·조건부가 대부분 | "원 도구의 입력 정의 밖"이지 "Kairos만 가능"이 아님. 확장으로 메울 수 있는 경우가 있음 |
| 2 | **주장하지 않음** | RacePro, SimRacer, SIMEXPLORER, DESCRY, SimEvo, SysPro. 올리지 않음: RRF, ReDPro | 대부분 "서술이 없다"가 근거이며 약함. 이름으로 식별하는 것 자체는 RacePro에 이미 있음 |
| 3 | **주장하지 않음** | DESCRY, SimEvo, SysPro(중간). RRF는 연결하나 C2 자체로는 올리지 않음. 올리지 않음: RacePro, SimRacer, SIMEXPLORER, ReDPro | 모든 파일이 C2 앱 비공개를 이유로 C2에 대한 도구 부족 가설을 세우지 않음. C2는 역할 재구성의 메커니즘 검증에만 쓸 수 있음 |
| 4 | **주장하지 않음** | RacePro, SimRacer, SIMEXPLORER, RRF(조건부), DESCRY, SimEvo, SysPro. 올리지 않음: ReDPro | 사용자가 반환값·최종 결과를 내는 oracle을 쓰면 구분할 수 있을 수 있음. 능력 부족보다 입력 형식 부재에 가까움 |
| 5 | **주장하지 않음**. 후보 연결도 두지 않음 | 없음 | 여덟 도구에 근거가 없거나 반대임. 로드맵 §10.5의 이 행은 사용하지 않거나 앵커 종류·탐색 범위로 한정해야 함 |
| 6 | **주장하지 않음** | RacePro(부분, H-RP-4. H-RP-5는 목적 차이로 이 행에 가장 가깝다는 참고), SimRacer, SIMEXPLORER, RRF(부분), DESCRY(중간). 올리지 않음: SimEvo, ReDPro, SysPro | R2의 순서가 고정인지 스케줄 의존인지 사례 기록상 미결이다. 반대 순서가 실제로 불가능한지는 gdb 3회 기록만으로 확정되지 않는다(`results/R2_gdb_v701.md` §3 "not a proof of determinism"). 일부는 재현·회귀가 목적인 도구라 목적 차이일 수 있음. 2026-10-16에 SimRacer.md·DESCRY.md가 S1 구조에서 뒤집기 schedule이 교착·진행 막힘에 이를 수 있다는 [I] 도출을 더했으나 실행 결과가 아니다 |

### 4.5 가설이 확인된 부족으로 바뀌려면

| 도구 | 도구별 파일이 적은 확인 항목 | 필요한 것 |
|---|---|---|
| RacePro | 코드 읽기 항목(Scribe의 POSIX mqueue 추적, py-scribe의 page ownership 읽기)은 2026-10-16에 코드 수준에서 정리됨(K1–K5). 남은 것은 실행으로만 확인되는 것: 실제 로그에 `mq_open`의 FILES_STRUCT 사건이 찍히는지, PID 사건 값이 thread id인지, thread 종료의 explorer 전달, RacePro 녹화에서 R2 순서가 나오는지 (RacePro.md §6) | 실행은 P-3(별도 머신) |
| SimRacer | ISSTA'13 본문: 메모리 관측이 SimRacer에 있었는지, SR 지정 인터페이스의 입도, 모델링된 syscall 23종의 목록, 탐색 방식 (SimRacer.md §6). Sim-O/C와 SimRacer가 같은 도구의 서술인지, Yu의 2016년 UNL 홈 Wayback 스냅숏 안에 ISSTA'13 PDF 링크가 있는지(이 세션에서 열 수 없음) | 본문 접근 또는 저자. 스냅숏은 `web.archive.org`가 열리는 환경 |
| SIMEXPLORER | 구현 또는 저자 설명: 메모리 SR 지정 방법, "SR equals" 기준, syscall 23종 목록, 여러 thread를 가진 프로세스의 PuT 구분 (SIMEXPLORER.md §6) | 구현·저자 |
| RRF | 학위논문 본문(oracle 인터페이스 입력, Scribe 판본), 내부 oracle 논문 [36] 본문(초록만 읽음) (RRF.md §6) | 학위논문·[36] 본문 접근 |
| DESCRY | 사건·자원 모델링 위임 범위(SimRacer ISSTA'13와 학위논문), 사례 이슈 댓글의 실패 로그 (DESCRY.md §6) | 본문·댓글 접근 |
| SimEvo | SimRacer 원문, 구현 (SimEvo.md §6) | 위와 같음 |
| ReDPro | 전문의 확인 항목(PIN 계측 사건, 자원 식별, pair 정의, 입력, 순서 제어 방식, 출력, 대상 범위, 구현 위치), 2022-05-07 Internet Archive 스냅숏이 ACM PDF인지 (ReDPro.md §6 항목 10) | 전문. 스냅숏은 `web.archive.org`가 열리는 환경과 공개 보관본의 출처 인정(§5.4 항목 17) |
| SysPro | ReproSys, JSS판, sleep 길이·판정 방법 (SysPro.md §6) | 구현·저자·JSS판 |

## 5. 계획

이 절의 항목은 모두 [제안]이다. 일정, 횟수, 예산, 노력 수치를 정하지 않는다. 작업의 의존만 적는다.

### 5.1 지금 적용할 수 있는 것 (읽기와 사건 대응. 설치·실행·저자 연락 없음)

| # | 작업 | 상태·근거 | 비고 |
|---|---|---|---|
| A1 | 여섯 질문 × 다섯 사례 분석을 도구별 파일에 기록하고 이 문서에 통합 | 일곱 도구는 본문 전문(SimRacer는 확장판 경유)으로 완료. ReDPro는 초록 수준. 2026-10-16판으로 재합성 | 본 문서 §2 |
| A2 | RacePro 사건 대응 R2·C1·C3·C5·C2(공개 조건) | 본 문서 §6. 2026-10-16에 ⓪ 행과 코드 후보 클래스별 대조를 더함 | 로드맵 §10.3 첫 산출물 |
| A3 | RacePro 공개 코드를 읽어 §6의 "불명" 사건을 해소 | **코드 수준에서 처리됨(2026-10-16, RacePro.md §2.5 K1–K7).** `ipc/mqueue.c`에 Scribe 호출이 없고, `mq_open`의 fd 할당·설치는 FILES_STRUCT 쓰기가 되며, 큐 이름 조회와 `mq_timedsend`/`mq_timedreceive`의 큐 객체 접근에는 잠금 경로가 보이지 않음(받은 디렉터리만 검색). py-scribe는 `mem_owned_*` 사건을 파싱하나 explorer는 쓰지 않음. 남은 것은 실행으로만 확인되는 항목(실제 로그, PID 값이 thread id인지, thread 종료 전달) | 실행 항목은 P-3에 묶임 |
| A4 | 다른 도구에 §6과 같은 형식의 사건 대응표 | 2026-10-16 기준: RacePro.md, SimRacer.md, RRF.md, DESCRY.md, SysPro.md가 ⓪–④의 R2 사건표를 가짐(DESCRY.md·SysPro.md는 새로 넣음, RRF.md·SimRacer.md·RacePro.md는 ⓪ 행을 더함). RacePro.md·SimRacer.md·RRF.md·DESCRY.md·SysPro.md는 C1·C3·C5 보충표가 있음. SIMEXPLORER.md와 SimEvo.md는 사례 칸으로만 있음. ReDPro는 전문이 없어 불가 | §6.8에 반영. 같은 STVR 근거인 SimRacer·SIMEXPLORER는 SimRacer.md가 "SIMEXPLORER 설계" 열과 "SimRacer 자체" 열을 나눠 이미 맞춤 |
| A5 | 접근 가능한 원문 경로를 다시 시도 | P-PAPERS [D]. 2026-10-16에 여덟 파일이 모두 다시 시도했다. 새로 얻은 것: SimRacer.md·SimEvo.md의 Sim-O/C(ITJ 2013)와 SimTester(VEE'12) `2차`, SimEvo.md의 피인용 3건 문맥, RRF.md의 [36] 초록, DESCRY.md의 Zaman 학위논문 초록 전체, ReDPro.md의 ORCID 목록과 Internet Archive 스냅숏 존재, SysPro.md의 JSS 서지·PAR 엠바고(2027-06-01)·저자 홈페이지·NSF 과제 초록. 새 전문은 없다. Cloudflare·AWS 도전 페이지와 egress 차단은 우회하지 않음 | 사용자 PDF 제공을 착수 조건으로 만들지 않음 |
| A6 | RRF가 인용한 내부 oracle 논문(ISSRE 2013, 논문 [36])을 읽어 RRF의 oracle 입력 형식 확인 | **미해결.** 2026-10-16에 초록만 읽음. 초록은 internal oracle이 "monitoring aspects of internal program and system states"로 탐지한다고만 적고 입력 형식은 적지 않음. UNL PDF Cloudflare, IEEE WAF, Semantic Scholar `CLOSED` | H-RRF-3의 조건은 그대로 |
| A7 | 사례 쪽 입력이 확정되는 대로 같은 분석을 갱신 | C1 구성(D22), C2 역할 재구성의 요청 방식(P-C2), C3·C5 6.4.x 소스 대응(D13·Q16)이 정해지면 C2 칸과 C1·C3·C5 칸을 다시 채움. C3의 OSAL 4.1.1 queue 구현이 `mq_open`을 쓰는지도 이 작업에서 확인 | C2 칸은 그 전에는 불명이 대부분 |
| A8 | 다른 문서에 반영할 정정 목록 전달 | §5.5 | 이 작업의 소유 범위 밖이라 수정하지 않음 |
| A9 | 로드맵 §12 작업 14(`process_race_comparison.md`)의 입력으로 §3·§4를 사용 | 작업 14의 완료 확인은 "`NOT_RUN`과 실제 누락을 구분" | §3.1의 정의 유지 |
| A10 | STVR(SIMEXPLORER 행의 근거)과 RRF의 쪽 번호 인용을 둘째 취득 경로(저자 공개본, 기관 저장소, 연결된 학술 검색)로 재확인 | **2026-10-16에도 얻지 못함**(SimRacer.md, SIMEXPLORER.md, RRF.md). 같은 호스트에서 다시 받은 같은 파일은 둘째 경로가 아님 | 재확인 전까지 "단일 취득 경로, 독립 재확인 안 됨" 표시 유지. PI 확인으로 대신할 수 있다(§5.4 항목 16) |
| A11 | `OS_TaskCreate` → `clone3` 근거 표기를 도구별 파일 사이에서 맞춤 (2026-10-16 추가) | DESCRY.md는 OSAL 시험의 기록, SysPro.md·RacePro.md·SimRacer.md는 에이전트 C 프로그램의 기록으로 적음. 두 기록이 모두 있음(§2.5 항목 9) | 각 파일 담당이 정함. 이 문서는 §0.1 (4)로 씀 |
| A12 | R2 ④ "syscall 없음"의 근거 표기를 맞춤 (2026-10-16 추가) | RRF.md가 `results/R2_gdb_v701.md`의 "no syscall was made"를 인용(§2.5 항목 10) | RRF.md 담당이 정함 |

### 5.2 코드 또는 저자가 필요한 것

저자 연락의 정책은 PI 사안이다. 아래는 목록이며 **아무에게도 연락하지 않았다.** 연락 여부, 주체(누구의 이름으로), 시점, 요청 범위는 PI가 정한다.

| 도구 | 필요한 것 | 현재 상태 | 해결 경로 후보(도구별 파일) |
|---|---|---|---|
| RacePro | 코드는 공개. 별도 머신(KVM 가능 또는 bare metal), i386 Linux 2.6.35 + Scribe 내장 커널(번들 `kernel_config`), 그 위의 userland(판본은 `unspecified_by_reference`), cFS를 그 userland에서 빌드·실행하는 조건 | userland 배포판과 cFS 빌드 조건은 레퍼런스가 정하지 않음(`unspecified_by_reference`) | RacePro.md P-3. RacePro.md는 저자 요청 항목을 두지 않았다(코드가 공개) |
| SimRacer | 구현, ISSTA'13 본문, Simics 구성, 저자 연구 자료 | 저장소·저자 페이지에 없음(2026-10-16 재확인). Yu의 2016년 UNL 홈 Wayback 스냅숏은 있으나 이 세션에서 열리지 않아 PDF 링크 유무를 모름 | 저자 요청, 기관 도서관, 다른 사본, `web.archive.org`가 열리는 환경(§5.4 항목 17) |
| SIMEXPLORER | 구현, 객체 프로그램 24개와 oracle 파일(first author에게 요청하라고 논문이 적음, p.15), 학위논문 PDF | 도구 코드 제공 여부는 논문에 서술 없음 | 저자 요청 |
| RRF | 구현(정적 분석 도구 이름, 이벤트→소스 대응, oracle 저장소, Scribe 판본), 학위논문 PDF, 내부 oracle 논문 [36] 본문 | 논문에 위치 서술 없음. 저자 CV(2026-10-16)에도 아티팩트 링크 없음 | 저자 요청 또는 PDF 확보 |
| DESCRY | 구현(LLVM 분석기, KLEE 확장, Simics 설정), Zaman 학위논문 PDF, 프로젝트 페이지의 내용 | 프로젝트 페이지는 원 서버 403, Wayback 가용성 API에 스냅숏 없음, CDX 검색은 egress 정책이 막음(2026-10-16). 학위논문 초록 전체에는 구현 상세가 없음 | 저자 요청. 논문에 저자 이메일이 있음 |
| SimEvo | 구현(및 기반 SimRacer), CodeSurfer 가용성, cFS용 테스트 입력·오라클 정의 | 정의의 레퍼런스가 없음 | 저자 요청. 입력·오라클 정의는 PI 결정 |
| ReDPro | 논문 전문, 구현, PIN 판본 | 전문이 없어 모든 질문이 불명. ORCID 목록에도 ACM DOI 링크뿐 | PDF 확보 또는 저자 요청. Internet Archive 스냅숏(ReDPro.md D-RD-1 (e), §5.4 항목 17). 전문 없이 가는 안도 있음(D-RD-1 (d), 파일의 기본값) |
| SysPro | ReproSys, JSS판 본문과 data-availability 문장, sleep 길이·재현 판정 방법·사용 판본 | 익명 접근에 인증을 요구함. 2026-10-16 `add_repo`(읽기) 거절, 저자 홈페이지·CV에 링크 없음. JSS판은 NSF PAR이 2027-06-01에 공개 예정 | 저자 요청(논문의 교신저자 서술은 SysPro.md §4.3-2. 첫 저자의 현 소속은 UMBC), 또는 접근 권한이 있는 계정 |
| 공통 | 사례 이슈 댓글(`nasa/CF` #184, `nasa/cFE` #198·#73·#72·#2663) | 세션이 두 저장소를 막음. 2026-10-16 GitHub MCP `issue_read`는 저장소가 세션에 구성되지 않았다며 사용자가 요청하지 않은 저장소를 추가하지 말라고 안내함. `add_repo`는 자격을 붙이는 조치라 사용자의 요청이 필요함 | PI(사용자) 요청 또는 사용자가 댓글을 직접 제공 |

환경 쪽 도구(Simics의 EULA 동의와 설치, PIN, KLEE, srcML, CodeSurfer 등)는 설치하지 않았다. 설치와 판본 선택은 `deviation` 또는 `unspecified_by_reference` 기록 대상이며 PI 결정이다(§5.4).

### 5.3 재구현과 원 도구 결과의 분리 [제안]

로드맵 §10.4의 원칙: 원 도구를 실행할 수 없고 논문·코드로 기법을 재구현하면 원 도구 결과와 분리한다. 사건을 추가로 제공했는지, 기법을 확장했는지 기록한다. Kairos 기능 제거 시험을 기존 도구의 결과로 부르지 않는다.

| # | 규칙 | 근거 |
|---|---|---|
| S1 | 모든 도구 결과 행에 `availability.md` §5.1의 라벨을 붙인다: `ORIGINAL-RUN`(저자 구현을 문서화된 요구를 모두 충족해 실행), `DOCUMENTED-SETUP-RUN`(문서화된 설정 그대로, cFS 고유 추가 없음), `DEVIATION-RUN`(요구를 대체. `deviation`으로만 허용), `EXTENSION`(원 도구에 cFS 지원을 추가), `REIMPLEMENTATION`(원 도구 없이 논문의 방법을 다시 만듦), `ANALYSIS`(관측 모델 분석, 미실행). 오늘 존재하는 라벨은 `ANALYSIS`뿐이다 | `availability.md` §5.1. 도구별 파일의 §4.2 |
| S2 | `REIMPLEMENTATION` 결과는 원 도구 결과로 보고하지 않는다. 별도 표·별도 열에 둔다. 재구현 결과가 생겨도 원 도구 칸의 `NOT_RUN`을 바꾸지 않는다 | `availability.md` §5.1, 로드맵 §10.4 |
| S3 | 재구현 행마다 "논문이 서술한 부분"과 "서술하지 않은 부분"을 나눠 적는다. 서술하지 않은 부분의 선택은 각각 `deviation` 또는 `unspecified_by_reference`이며 값을 임의로 고르지 않는다. 선택은 PI 결정이다 | CONDITIONS_POLICY. 아래 표 |
| S4 | `EXTENSION`(예: OSAL 레지스트리를 자원으로 추가, RacePro의 `mem_owned_*` 공유 메모리 사건을 후보 생성에 쓰기(RacePro.md §2.5 K5), 소스 위치를 직접 지정, 반환값을 보는 checker)은 어떤 사건을 추가했는지 기록하고 원 도구의 자동 결과로 부르지 않는다 | 로드맵 §10.4 끝, RacePro.md §3.6, SIMEXPLORER.md §3.6 |
| S5 | Kairos의 단일 기능 제거 시험(P-ABL-ALLOWED [D]: 허용 처리 정보 제거와 반환·결과 의미 제거 각각)은 기존 도구의 결과 표에 넣지 않는다 | 로드맵 §10.4 마지막 문장 |
| S6 | `OBSERVATION_GAP`·`ANALYSIS_GAP`을 §4의 "확인된 부족"으로 올리는 것은 `ORIGINAL-RUN`·`DOCUMENTED-SETUP-RUN`에서 얻은 경우로 한정한다. `ANALYSIS_GAP`은 "해당 입력·기준에서" 보고한다(로드맵 §10.4). `DEVIATION-RUN`에서 얻은 부족은 "deviation 조건 한정 부족", `REIMPLEMENTATION`에서 얻은 부족은 "재구현 한정 부족", `EXTENSION`에서 얻은 부족은 "확장 한정 부족"으로 따로 표기하고 원 도구의 부족으로 승격하지 않는다 | 이 문서의 제안([제안]). 로드맵에 이 구분을 직접 적은 문장은 없다. 근거는 S2·S4와 `availability.md` §5.1·§5.2: `DEVIATION-RUN`은 문서화된 요구를 대체한 실행이라(예: RacePro의 QEMU TCG, `-m32` userland) 부족이 도구가 아니라 대체에서 왔을 수 있다 |
| S7 | 재구현의 충실성(Simics 의존 부분 등)을 확인할 구현이 없으면 그 사실을 행에 적는다 | SIMEXPLORER.md §4.2 |
| S8 | 각 결과 행에 도구 판본, 논문 판본(쪽 기준 판본), 입력 구성, `deviation` 목록을 적는다 | CONDITIONS_POLICY |

도구별 재구현 가능 범위(도구별 파일의 서술):

| 도구 | 논문이 서술한 부분 | 서술하지 않은 부분 | 선결 조건 |
|---|---|---|---|
| RacePro | 코드가 공개이므로 후보 규칙을 읽을 수 있음. 코드와 논문의 차이가 있음(RacePro.md §2.4) | 재구현 여부는 RacePro.md가 평가하지 않음 | 논문과 코드의 대조, PI 승인(A9) |
| SimRacer | STVR의 Phase 1·2 알고리즘(Fig.10, 12, 13), HB 규칙, 관측·제어 구현이 SIMEXPLORER 서술 기준으로 있음. ISSTA'13과 다를 수 있음. 그래서 SimRacer.md P-5는 이런 재구현이 SimRacer 원 도구가 아니라 SIMEXPLORER 서술 기준이라고 적음 | Simics 버전·머신·스크립트·guest. Simics 대신 다른 수단을 쓰면 `deviation` | PI 결정(SimRacer.md P-5). 기본값은 만들지 않음 |
| SIMEXPLORER | 알고리즘(Fig.10, 12, 13) | Observer·Controller의 Simics 의존 부분, 23개 syscall 목록, 메모리 SR 지정 방법 | PI 승인(SIMEXPLORER.md P-S4). 기본값은 만들지 않음 |
| RRF | Algorithm 1, 쌍 열거, 네 패턴, 로그 거르기 | 정적 분석 도구, 이벤트→소스 대응 구현, oracle 구현, Scribe 판본, 대기 시간 결정 세부 | 학위논문 본문을 읽은 뒤 결정하는 편이 맞음 |
| DESCRY | 로그 → logging point 대응(p.5), goal list(p.5), schedule 뒤집기(p.7) | KLEE 확장(p.6의 서술 수준), Simics 설정, 사건·자원 모델링([21,38] 위임) | PI 승인(A9). 실패 로그 입력이 필요함 |
| SimEvo | 알고리즘 수준(p.4–6) | 사건·자원 모델(SimRacer에 위임), Simics·CodeSurfer 값 | SimRacer 원문 열람, PI 승인 |
| ReDPro | 없음(전문 미도달) | 전부 | 전문 |
| SysPro | man 페이지 NAME 기반 syscall 선택(p.15), 12조합 구조화 IR 파일 순위(p.16–17), apriori 쌍 순위(p.17–18), 쌍 위치 탐색(p.18–19), 정규식·TSL test case(p.19–20), PIN sleep 삽입(p.20–21) | sleep 길이, 재현 판정 방법, 다른 프로세스를 함께 실행하는 방법, man 페이지 묶음, PIN 판본. cFS용 앞단(Linux syscall 이름 대신 cFE/OSAL API)을 붙이면 `EXTENSION` | PI 결정 |

### 5.4 PI 판단이 필요한 항목 (결정 후보. 이 문서는 정하지 않았다)

| # | 항목 | 해당 도구(도구별 파일의 번호) | 파일의 기본값 |
|---|---|---|---|
| 1 | 같은 프로세스 안 task 쌍을 `INAPPLICABLE`로 볼 것인가 | 도구별 표(아래) | 도구마다 다르다. 일곱 도구가 모두 "아니오"를 적은 것은 아니다(아래 표) |
| 2 | 논문의 실험 서술(소프트웨어·guest 값)을 `from-reference`로 볼 것인가 | RacePro P-1, SimRacer P-1, SIMEXPLORER P-S1 | `unspecified_by_reference` 유지, 논문 서술 병기 |
| 3 | 저자 연락의 정책(여부, 주체, 시점, 요청 범위) | SimRacer P-3, SIMEXPLORER P-S3, RRF §4.3-3, DESCRY §4.3-3, SimEvo §4.3-2, ReDPro D-RD-1 (c), SysPro §4.3-2 | 연락하지 않음 |
| 4 | 재구현·확장을 만들 것인가 | SimRacer P-5, SIMEXPLORER P-S4, RRF §4.3-2, DESCRY §4.3-5, SimEvo §4.3-3, SysPro §4.3-6 | 만들지 않음 |
| 5 | RacePro를 돌릴 별도 머신 | RacePro P-3 | `NOT_RUN` 유지 |
| 6 | Simics 설치(EULA 동의 포함) | SimRacer P-4, SimEvo §4.3-4 | 설치하지 않음. 설치해도 도구 코드가 없어 SimRacer 실행은 되지 않음. 2026-10-16의 `/` 여유(1.3–2.7 GB, §1.2)는 벤더 문서의 설치 크기(약 10 GB)보다 작음 |
| 7 | SimRacer와 SIMEXPLORER를 한 계보로 묶을지 | SimRacer P-6, SIMEXPLORER P-S5 | SimRacer.md: 묶고 판본을 표시. SIMEXPLORER.md: 다른 문서를 바꾸지 않았고 PI와 오케스트레이터가 결정. **이 문서는 별개 행으로 두고 계보를 표시했으며 묶는 결정을 하지 않았다** |
| 8 | 논문 PDF를 확보할 방법(학위논문, ReDPro, SysPro JSS판 등) | RRF §4.3-4, DESCRY §4.3-4, SysPro §4.3-4, ReDPro D-RD-1 (b) | P-PAPERS [D]는 사용자 PDF 제공을 착수 조건으로 만들지 않음 |
| 9 | 사례 이슈 댓글을 읽을 방법과 SysPro 입력에 댓글을 넣을지 | DESCRY §4.3-2, SysPro §4.3-3 | 읽지 않음. GitHub MCP는 사용자가 요청하지 않은 저장소를 추가하지 말라고 안내하므로 저장소 추가는 사용자의 요청이 있어야 함(SysPro.md §4.3-3, 2026-10-16) |
| 10 | 도구 판본 확정의 해석(SysPro 참고문헌의 NLTK 3.5·gensim 3.8.3을 사용 판본으로 읽을지, ReDPro의 PIN 판본) | SysPro §4.3-5, ReDPro D-RD-4 | `unspecified_by_reference` |
| 11 | cFS에서 "테스트 입력"과 "오라클"의 정의 | SimEvo §4.3-5 | 정의 없음 |
| 12 | 이전 시도가 남긴 빌드 로그(자격 증명 이름의 변수를 포함)의 처분 | RacePro P-4 | 그대로 둠 |
| 13 | 로드맵 §10.5의 "관측된 실행만 다룸" 행의 처리 | 여덟 도구 전부(§4.2 행 5) | 이 문서는 "비교 결과로 주장하지 않음"까지만 정함 |
| 14 | C2의 입력 미충족(앱 P·R 소스 비공개)을 `INAPPLICABLE`로 볼 것인가. 입력 요구가 충족되지 않은 것이 "명시된 입력·대상 범위에 해당하지 않음"에 드는지, P-C2 [D]의 역할 재구성으로 입력을 마련할 수 있으면 입력 미충족이 아닌지도 함께 정해야 한다 | DESCRY §4.3-1 (c)(PuD 소스 요구: DESCRY p.1, p.3), SysPro §4.3-1 (c)(대상 응용 소스 비공개: SysPro.md Y8). RRF(PuD 소스 필요, RRF p.5)와 SimEvo(대상 앱 소스 비공개)는 같은 사유를 적었으나 안으로 올리지 않았다 | 정하지 않음. DESCRY·SysPro 파일은 (a) `NOT_RUN`을 적용. 채택하면 DESCRY·SysPro·RRF·SimEvo의 C2 최대 4칸이 `NOT_RUN`에서 `INAPPLICABLE`로 바뀌고 이유와 원문 위치를 보고해야 한다(§3.1). §0.2 항목 1의 "40칸"은 이 항목이 열린 상태의 수다 |
| 15 | ReDPro를 관련 연구에서 어떻게 서술할지(D-RD-2), 전문을 얻은 뒤 여섯 질문을 다시 채울지(D-RD-3) | ReDPro D-RD-2, D-RD-3 | D-RD-2: 전문 확인 전에는 초록 수준의 서술만 허용하고 역할 표에서 제외. D-RD-3: 전문을 얻으면 §6의 확인 목록으로 채운다 |
| 16 | CPS-VO 사본(STVR, RRF)의 취득 방식(WAF 거부 뒤 클라이언트 문자열 변경 또는 WebFetch)을 허용 범위로 볼지, 둘째 취득 경로를 찾을지. CPS-VO 파일 페이지의 "Creative Commons 2.5" 표기가 재배포 조건을 정하는지도 함께(§1.1, SIMEXPLORER.md §1.3 항목 10과 SimEvo.md §1.3이 서로 다른 페이지를 봄) | §1.1 취득 경로, SIMEXPLORER.md 경로 13–14, SimRacer.md 경로 18, SimEvo.md 경로 29·§4.3-6, DESCRY.md 경로 18·§4.3-6, RRF.md 경로 1–2·§6 항목 11 | 확인 전까지 STVR·RRF 기반 쪽 번호 인용에 "단일 취득 경로, 독립 재확인 안 됨" 표시를 유지. 2026-10-16에도 둘째 경로는 없음. PDF는 저장소에 넣지 않음 |
| 17 | 공개 웹 보관본(Internet Archive)을 원문 출처로 인정할지, `web.archive.org`가 열리는 다른 환경에서 받을지, 받은 뒤 출판본과 같은 판본인지 어떻게 확인할지 (2026-10-16 추가) | ReDPro D-RD-1 (e)(ACM PDF 주소의 2022-05-07 스냅숏), SimRacer.md §6 항목 16(Yu의 2016년 UNL 홈 스냅숏), DESCRY.md §6 항목 6(프로젝트 페이지는 스냅숏 없음) | 정하지 않음. 이 세션의 egress 정책이 `web.archive.org`를 막고 있어 우회하지 않았다. ReDPro.md의 기본값은 (d) 전문 없이 간다 |

**항목 1의 도구별 현황** (도구별 파일이 적은 대로. 일곱 도구가 같은 값을 적은 것이 아니다):

| 도구 | 출처 | 파일의 기본값 |
|---|---|---|
| RacePro | RacePro.md P-2 | 아니오(`NOT_RUN` 유지) |
| SimRacer | SimRacer.md P-2 | 아니오(`NOT_RUN` 유지) |
| SIMEXPLORER | SIMEXPLORER.md P-S2 | 아니오(`NOT_RUN` 유지) |
| DESCRY | DESCRY.md §4.3-1 | 선택지 (a) `NOT_RUN`을 적용. (c) C2만 입력 미충족으로 따로 처리하는 안이 있음(항목 14) |
| SysPro | SysPro.md §4.3-1 | 선택지 (a) `NOT_RUN`을 적용. (c) C2만 입력 미충족으로 따로 처리하는 안이 있음(항목 14) |
| RRF | RRF.md §4.3-1 | 기본값 선언 없음. "보지 않으면 `NOT_RUN`"이라고만 적었고 오늘 상태에는 `NOT_RUN`을 적용 |
| SimEvo | SimEvo.md §4.3-1 | 정하지 못함("범위 일치 여부를 문서가 정하지 못한다"). 오늘 상태에는 `NOT_RUN`을 적었음 |
| ReDPro | ReDPro.md §4.2 | 붙일 수 없음. 전문 확인 뒤에 판단 |

### 5.5 다른 문서에 반영이 필요한 것 (도구별 파일이 지적한 것과 2026-10-16 대조에서 이 문서가 찾은 것. 이 문서에서 수정하지 않았다)

| 대상 | 내용 | 지적한 파일 |
|---|---|---|
| `baselines/availability.md` §1·§3·§5.4 | RacePro·SimRacer·SIMEXPLORER·RRF·DESCRY·SimEvo·SysPro 칸의 등급과 서술. "syscall 모델이라 shared-resource 사건이 아니다", "None of the five", RRF 칸 "Possibly, via yield points" 등 | SimRacer.md §1.3·§6-11, RRF.md §1.3·§6-9, DESCRY.md §1.3, SimEvo.md §1.3, SysPro.md §1.3 |
| `baselines/yu-group.md` §3~§7, `baselines/descry.md`, `baselines/syspro.md`, `baselines/racepro.md` | 발췌 기반 서술의 정정(예: SysPro의 "entry·exit 사건을 관측한다"는 논문에서 확인되지 않음, SimEvo의 "시스템 호출 모델이 서술되지 않았다"는 발췌의 한계, RacePro 서지의 DOI 오기) | SysPro.md §1.3, SimEvo.md §1.3, RacePro.md §1.4 |
| `Architecture/spec/SPEC_FINAL.md` | RacePro 근거 등급이 `[V-abs]`(초록 확인)로 표기됨. 이 파일은 병렬로 고쳐지고 있어 줄 번호가 유지되지 않으므로 절과 본문으로 가리킨다: §1.11의 출처 등급 표("Triggering and confirmation; outcome triage" 행의 "RacePro SOSP 2011 … [all V-abs]"), Appendix B의 "RacePro 2011; DESCRY 2017; RaceFuzzer 2008; Portend 2012 — V-abs", §8.3 Roles 표의 RacePro 행("paper not opened in the survey"). 전문 열람 수준으로 올릴 수 있음. 이 정정은 별도 단계 | RacePro.md §1.4-8 |
| `results/R2_gdb_v701.md` | §2 Results의 "no syscall was made"와 §5 Supported의 "invisible … at the syscall level"은 gdb 기록(breakpoint 기록)이 관측한 것이 아니다. 소스 읽기(`cases/mapping_v701.md` §1.2 "No syscall is involved")에서 온 진술이므로 소스 수준으로 한정해야 한다. 이 문서는 R2의 "syscall 없음"을 `[I 소스]`로만 쓴다 | 로드맵 §3 "R2 결과 표현의 과장" 행, 이 문서 §0.1 |
| `plan/ROADMAP_20261015.md` §10.1 | SimRacer와 SIMEXPLORER를 별개 행으로 둔 것(저자가 확장판 관계라고 밝힘). §10.5의 "관측된 실행만 다룸" 행 | SimRacer.md §1.3, SIMEXPLORER.md §0-2, 각 파일 §5.1 |
| `plan/methods/SysPro.md` §1.3 항목 17·증거 태그 절, `plan/methods/RacePro.md` C5 보충표, `plan/methods/SimRacer.md` C5 보충표 (2026-10-16) | `OS_TaskCreate` → `clone3`를 에이전트 C 프로그램의 기록으로만 적고 SysPro.md는 "OSAL 시험이 아니다"라고 적음. `osal-visibility.md`의 stack 귀속 표에는 OSAL `count-sem-test`에서 `OS_TaskCreate` 아래 `clone3` ×4가 있음. 두 기록 모두 인용할 수 있음 | 이 문서 §2.5 항목 9(저장소 기록 재대조) |
| `plan/methods/RRF.md` R2 사건표 ④, §3.5 R2 Q1 (2026-10-16) | `results/R2_gdb_v701.md` §2의 "no syscall was made"를 근거로 인용. 위 행의 이유로 gdb 기록은 syscall 근거가 아님 | 이 문서 §0.1, §2.5 항목 10 |
| `plan/methods/RacePro.md` §3.5 R2 Q5, H-RP-4 | R2의 순서를 "고정된다(사례 기록)"로 씀. 같은 파일 Q6은 "코드상 매 부팅 결정적이라는 예측(미실행)"으로 쓰고, 다른 파일들은 고정인지 스케줄 의존인지 미결로 씀 | 이 문서 §7 항목 10 |

## 6. RacePro 사건 대응 — R2, C1, C3, C5, C2 (로드맵 §10.3의 첫 산출물)

로드맵 §10.3의 요청: R2의 AppId 설정·값 전달·이벤트 요청·오류 반환을 RacePro의 기록 사건·후보 사건에 각각 대응시키고, 원 코드의 어느 규칙이 적용되는지와 대응하지 않는 사건이 무엇인지 적는다. 이어 C1·C3·C5에 같은 분석을 하고, C2는 역할 재구성에서 알려진 조건만 쓴다.

이 절은 `RacePro.md` §3.5의 사건 표와 후보 클래스별 대조표(2026-10-16)를 옮기고 대응하지 않는 사건을 따로 모았다. 근거 등급은 논문 전문 + 코드 직접 읽음(2026-10-16에 Scribe 커널 sparse checkout @`b29e9afb`와 py-scribe @`56d6187e`를 더 읽음, RacePro.md §2.5 K1–K7)이며, 사례 쪽 사실은 저장소 기록이다. **RacePro를 실행하지 않았다.** "대응 경로 없음"은 "읽은 설계 기술 안에서 그 사건을 micro-op·HB·후보 입력으로 만드는 경로를 찾지 못했다"는 뜻이며 탐지 실패가 아니다. 참조 revision은 사례 기록의 B/M/D(`fc8c48ad`/`12cb84fc`/`f22e538c`)이며 v7.0.1 코드와 다를 수 있다.

**"커널에서 보이는 흔적" 열의 근거 구분.** 이 열의 모든 "syscall 없음/0 syscalls"에는 `[M-probe 기록]` 또는 `[I 소스]`를 붙였다(§0.1). `[M-probe 기록]`은 OSAL `d2d877a`의 자체 시험(경합 없는 실행, cFS 밖, 도구 실행 아님)이며 C1의 OSAL `42af0f73`과 C3·C5의 OSAL 4.1.1 시대에서는 실행하지 않았다. R2·C3·C5의 "syscall 없음"은 모두 `[I 소스]`로 소스를 읽은 추론이다. 따라서 아래 "대응 경로 없음"은 RacePro의 설계 기술(micro-op이 되는 것은 syscall이 공유 커널 객체에 하는 접근뿐)과 이 열의 소스 수준 진술에서 도출한 분석이며, 사례 revision에서 syscall을 관측한 결과에 기대지 않았다.

### 6.1 대응 대상: RacePro의 모델 층

| 층 | 내용 | 근거 |
|---|---|---|
| 기록(recorder) | syscall, signal(sync point에서 전달), Table 2의 8종 커널 객체(inode, file, file-table, mmap, cred, global, pid, ppid)에 대한 접근 순서, 공유 메모리 page ownership 전이(재생용). 응용 수정은 필요 없음. 코드의 객체 열거는 INODE, FILE, FILES_STRUCT, PID, FUTEX, IPC, MMAP, PPID, SUNADDR로 논문 표와 다름(`cred`·`global` 없음, FUTEX·IPC·SUNADDR 추가). System V IPC(`msg`, `sem`, `shm`)는 `scribe_lock_ipc`를 부르고 POSIX mqueue(`ipc/mqueue.c`)는 Scribe 호출이 없음(2026-10-16 코드 읽기, K1). 커널은 공유 메모리 `mem_owned_read/write` 사건을 정의하고 py-scribe는 이를 파싱하나 `racepro` explorer는 쓰지 않음(K5) | 논문 §4 pp.356–357, Table 2; 커널 `scribe_resource.h:221-229`, `ipc/msg.c:488`, `ipc/sem.c:1099`, `ipc/shm.c:655`; `include/linux/scribe_events.h:160–177`; py-scribe `src/scribe/event_iterators.pxi:200–211` |
| micro-op | syscall이 공유 커널 객체에 하는 load/store(Table 3의 syscall 템플릿). 경로 이름은 부모 inode 안의 경로별 위치, 스트림은 가상 read/write offset, 신호는 신호별 추상 위치 | §5.2 pp.358–359 |
| HB 간선 | 같은 프로세스 안의 순서 + 4종: fork → 자식의 fork 반환, 자식 exit → 부모 wait, kill → 신호 전달, stream write → 겹치는 read. 공유 메모리·파일 내용에 의한 의존은 담지 않음(p.358). 코드는 일치하며 `clone`을 fork로 취급 | §5.1 pp.357–358; `execgraph.py:87-137, 139-180, 182-186`, `unistd.py:373` |
| 후보 규칙 | 논문: load-store, wait-wakeups, wakeup-waits. 3개 초과 syscall에 걸친 race는 기본으로 검출 안 함(p.361). 코드의 클래스: RESOURCE, EXIT-WAIT, SIGNAL, TOCTOU. FUTEX는 RESOURCE 경합 대상에서 제외. 논문의 일반 wait-wakeups·wakeup-waits에 해당하는 별도 클래스는 코드 목록에 없음(함수 단위로는 확인하지 않음). TOCTOU는 논문이 future work(p.365)라 하고 코드에 `toctou.py`가 있으나 `TODO`는 후보를 만들지 못한다고 적음. `find_show_races`는 RESOURCE, EXIT-WAIT, SIGNAL 세 단계만 돌고 TOCTOU는 별도 함수 `find_show_toctou`(기본 탐색 밖, K7) | §5.3 pp.359–361; `racecore.py` `find_show_races`(692–727행), `racecore.py:194, 282, 773`; `toctou.py:245–314`; `TODO` 5행 |
| 검증 | 앵커 syscall 순서를 바꾼 execution branch → consistent cut → replay → go-live → checker. checker는 built-in(로그 오류 메시지, segfault, timeout, 비정상 종료 코드), fsck류, 사용자 프로그램·스크립트, 선형화 실행과의 출력 비교. "can detect only what is checked"(p.363) | §6.1–6.3 pp.359–363; `racetest.py:151-152`(프로그램별 `<test>.test`) |
| 출력 | 후보마다 Diverged / Benign / Harmful. checker가 녹화 때 없던 실패를 볼 때만 버그로 보고하고 합친 recording을 저장 | p.354; §6.3 p.363; Table 5 p.364 |

대응 라벨: **대응(부분)** — 사건이 RacePro의 기록·HB 입력에 닿는다. **대응 경로 없음(분석)** — 읽은 설계 기술 안에서 경로를 찾지 못했다. **불명** — 입력이 되는지 자료로 정해지지 않는다. **검증층** — 기록 입력이 아니라 checker 판정에서만 닿을 수 있다.

### 6.2 R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

사례 쪽 사실(저장소 기록): SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로(`CreatePipe`/`Subscribe`/이벤트 전송)를 실행하고, `EVS_GetAppDataByID`가 NULL을 돌려 `ILLEGAL_APP_ID`가 된다. 상태는 폐기된다. 서술은 "이벤트 요청 거부"이다. gdb 3회 모두 설정 전 사용 24회가 거부 24회와 대응한다(`CreatePipe` 2 + `SubscribeFull` 4 + `ReportSingleEvent` 18, task별 ES 20·EVS 4. DESCRY.md와 ReDPro.md가 2026-10-16에 원시 trace에서 다시 세어 같은 값을 적었다). 이 24는 한 원인에서 나온 동적 발생이며 독립 결함 24개가 아니다(`results/R2_gdb_v701.md`). 순서가 고정된 것인지 스케줄 의존인지는 사례 기록상 미결이다.

**대응 요청의 네 항목(로드맵 §10.3)과 표의 행.** AppId 설정 = ①, 값 전달 = ②, 이벤트 요청 = ③, 오류 반환 = ④. ⓪은 2026-10-16에 RacePro.md가 더한 행으로, 순서의 원천인 core task 생성과 시작 직렬화(S1)를 RacePro의 HB 층에 대어 본 것이다.

| # | R2 사건 | 소스 위치 | 커널에서 보이는 흔적 | Scribe 기록 | micro-op | HB 간선 | 후보 규칙·클래스 | 대응 라벨 |
|---|---|---|---|---|---|---|---|---|
| ⓪ | **core task 생성과 S1**: ES가 `CFE_ES_CreateObjects`에서 core task를 `OS_TaskCreate`로 하나씩 만들고 각 task가 RUNNING이 될 때까지 `CFE_ES_MainTaskSyncDelay`로 기다림(사례 기록) | 사례 기록(`cases/R2.md` §6 S1·S2) | `OS_TaskCreate` → `clone3`(`[M-probe 기록]` (4), OSAL `d2d877a`의 `count-sem-test`, cFS 아님). RUNNING 대기는 메모리 폴링과 지연 `[I 소스]` | `clone`은 syscall로 기록됨 | 코드가 `clone`을 fork로 취급(`unistd.py:373`) | ES task의 `clone` → 새 task의 첫 syscall. RUNNING 폴링은 공유 메모리 의존이라 HB 간선이 아님(p.358) | 후보의 대상이 아니라 HB 간선의 원천 | **대응(부분)**(분석) |
| ① | **AppId 설정**: `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, 반환값 무시 | B `cfe_sb_task.c` L126, M L138, D L128 | syscall 없음 `[I 소스, 미실행]`. ES 조회는 메모리 + `pthread_getspecific`(`[I 소스]`, 실행 미확인) | 메모리 page ownership 순서만(재생용. py-scribe가 `mem_owned_*`를 파싱하나 explorer는 쓰지 않음, K5) | 없음 | 없음 | 네 클래스 모두 적용 없음(자원 접근 없음) | **대응 경로 없음**(분석) |
| ② | **값 전달**: EVS(B) 또는 ES·EVS(M, D) task가 같은 전역을 읽어 이벤트 API 인자로 넘김 | 같은 전역 | syscall 없음 `[I 소스, 미실행]` | 위와 같음 | 없음 | 없음 | 네 클래스 모두 적용 없음 | **대응 경로 없음**(분석) |
| ③ | **이벤트 요청**: `CFE_SB_CreatePipe`/`Subscribe`/전송 → `CFE_EVS_SendEventWithAppID` | SB API 경로 | `osal-core-test`에서 `OS_QueueCreate`가 `mq_open("<pid>.<name>")`을 69회 냄 `[M-probe 기록: OSAL d2d877a, cFS 아님]`. `CreatePipe` → `OS_QueueCreate` → `mq_open` 연결은 소스 읽기 `[I 소스]`(번들 `cfe_sb_api.c` L163, `os-impl-queues.c` L111-116). `Subscribe`와 이벤트 전송 자체는 syscall이 아님 `[I 소스]` | **2026-10-16 코드 읽기(K1–K3, 미실행):** `ipc/mqueue.c`에 Scribe 호출 0건. `mq_open`의 fd 할당(`alloc_fd`)·설치(`fd_install`)가 `scribe_lock_files_write`를 불러 FILES_STRUCT 쓰기가 됨. 큐 이름 조회는 mqueue 루트 inode의 `i_mutex`만 잡고 Scribe 잠금 없음. `mq_timedsend`/`mq_timedreceive`의 `fget`은 같은 syscall에서 `lock_next_file`이 세워졌을 때만 잠그며 그 호출은 `ipc/mqueue.c`에 없음(받은 디렉터리만 검색) | FILES_STRUCT store(fd table). 이벤트 요청 자체에는 없음 | 같은 process의 thread 사이 `mq_open`끼리는 syscall 의미에서 나오는 간선이 없음 | RESOURCE: 다른 task의 fd 할당과 HB상 동시이면 쌍이 될 수 있음(K4. 경로 거르기 `skip_parent_dir_race`가 FILES_STRUCT에 적용되나 경로를 아는 syscall 목록에 `mq_open`이 없음). 그 쌍은 **fd table 접근**이며 AppId 설정·사용이 아님. EXIT-WAIT·SIGNAL·TOCTOU는 적용 없음(`mq_open`은 TOCTOU 패턴 syscall이 아님) | **분석**(2026-10-15판의 "불명"에서 바뀜. 실행 확인 없음) |
| ④ | **오류 반환**: `EVS_GetAppDataByID` NULL → `ILLEGAL_APP_ID`, 상태 폐기 | B(`fc8c48ad`) `cfe_evs.c` L182-186. M(`12cb84fc`)·D(`f22e538c`)는 L185-189(`R2.md` §9). v7.0.1(`c5fb2b4d`)은 L185-188(`mapping_v701.md` §1.1, gdb breakpoint는 L188)이며 이 파일은 M과 diff가 없어(`mapping_v701.md` §0) 두 표기는 같은 코드의 범위 끝을 다르게 잡은 것이다 | 반환값. syscall 없음 `[I 소스, 미실행]` | 없음 | 없음 | 없음 | 네 클래스 모두 적용 없음 | **대응 경로 없음**(분석). 검증층에서만 닿을 수 있음(아래) |

**R2 × 코드의 후보 클래스(RacePro.md §3.5, 2026-10-16. 코드 읽기, 미실행).** 코드의 `find_show_races`는 RESOURCE, EXIT-WAIT, SIGNAL 세 단계만 돈다. TOCTOU는 별도 함수 `find_show_toctou`이고 기본 탐색 밖이며 `TODO` 5행이 이 시험이 후보를 만들지 못한다고 적는다(`racecore.py:692–727, 773`; `toctou.py:245–314`).

| 클래스 | 코드상 적용 조건 | ⓪ | ① | ② | ③ | ④ |
|---|---|---|---|---|---|---|
| RESOURCE | FUTEX 이외 자원에서 쓰기를 포함한 HB상 동시 접근 쌍(`racecore.py:185–303`) | 대상 아님(HB 간선) | 자원 접근 없음 | 자원 접근 없음 | FILES_STRUCT 쌍 가능(fd table) | 자원 접근 없음 |
| EXIT-WAIT | 같은 reaper가 거두는 두 exit와 wait | 해당 없음(생성) | 아님 | 아님 | 아님 | 아님 |
| SIGNAL | 기록된 신호 전달 | 아님 | 아님 | 아님 | 아님 | 아님 |
| TOCTOU | 기본 탐색 밖. stat·access·open·chmod·chown·link 계열 | 아님 | 아님 | 아님 | 아님 | 아님 |

[I] ①②④에 적용되는 클래스는 없다. ③에서 RESOURCE가 적용될 여지가 있지만 대상은 fd table이다. 이것은 "RacePro가 R2를 탐지하지 못한다"는 결과가 아니다.

R2의 "syscall 없음"(①②④)은 gdb 3회 기록(`env/logs/r2_g1..3_*`)이 뒷받침하지 않는다. 그 기록은 breakpoint 기록이고, cFS에서 strace를 돌린 기록은 없다(`osal-visibility.md` §0 D1 미결). R2의 syscall 부재는 소스 읽기(`cases/mapping_v701.md` §1.2)일 뿐이다.

**R2에서 RacePro가 순서의 이유를 설명하는가(H-RP-4).** 사례 기록은 core 시작 직렬화(S1)가 ES/EVS를 SB보다 먼저 실행하게 한다고 본다. 이것은 코드 읽기의 예측이며, 순서가 고정인지 스케줄 의존인지는 사례 기록상 미결이다(gdb 3회가 같은 결과를 냈으나 결정성의 증명이 아니다. `results/R2_gdb_v701.md`). ⓪이 주는 HB 간선은 `clone` → 새 task이고, RUNNING 폴링은 HB에 들지 않는다(p.358). 그래서 도구 안에서 순서의 이유가 설명되지 않는다. AppId 설정·사용 쌍은 커널 객체 syscall이 아니어서(`[I 소스]`) 그 순서를 뒤집은 branch는 만들어지지 않는다. ③의 FILES_STRUCT 쌍을 뒤집은 branch는 fd table 접근 순서를 바꾸는 것이며, 그 branch가 replay에서 diverge하는지, go-live 뒤 AppId 설정 순서가 달라지는지는 읽은 자료로 정할 수 없다.

**검증층과 보고 조건(H-RP-5, 2026-10-16).** built-in checker(로그 오류 메시지, segfault, timeout, 비정상 종료 코드)는 내부 반환값을 보지 않으며, R2는 실패 메시지도 출력하지 않는다. 대응하려면 사용자 checker가 내부 상태를 볼 수 있어야 한다. 또 논문은 checker가 "a failure that did not occur during recording"을 볼 때 버그로 보고한다(p.363). R2 현상은 gdb 3회 모두에서 나타났고, RacePro 녹화에도 나타난다면 branch에서 보이는 같은 현상은 "녹화 때 없던 실패"가 아니다. RacePro 녹화에서 같은 순서가 나오는지는 실행하지 않아 미확인이다. 이것은 RacePro의 목적(해로운 다른 순서 찾기)과 범위가 다르다는 분석이며 결함이 아니다. 원 녹화에 checker를 돌리는 일은 race 검증 절차 밖에서도 할 수 있다.

**대응하지 않는 R2 사건(요약).** ① AppId 설정, ② 값 전달, ④ 오류 반환은 기록·micro-op·HB·후보 어디에도 대응 경로가 없다. ③ 이벤트 요청은 fd table 접근으로만 닿고 의존 상태(AppId)와는 이어지지 않는다. ⓪은 HB 간선의 원천으로만 닿는다.

### 6.3 C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

사례 쪽 사실(저장소 기록): provider(공개 BP 구성에서는 `OS_CountSemCreate`)가 이름 있는 count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`(`CF_CFDP_InitEngine`)이 성공한다. 수정 전 CF는 한 번 조회하고 실패하면 `CF_EID_ERR_INIT_SEM` 이벤트와 `APP_ERROR` 종료이다. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × 100 ms 재시도이다(수정 전후 후보 CF `281a941` → `833fdbb`는 D22 [D]이며 결정이지 사실이 아니다). 실패 출력과 빈도는 `unspecified_by_reference`이다.

| # | C1 사건 | 커널에서 보이는 흔적 | RacePro 입력 | 대응 라벨 |
|---|---|---|---|---|
| C1-a | 생성: `OS_CountSemCreate`(`sem_init` + 테이블 등록) | 0 syscalls `[M-probe 기록: OSAL d2d877a의 count-sem-test, 경합 없음, cFS 아님. C1의 OSAL 42af0f73에서는 실행하지 않음]` | 없음 | **대응 경로 없음**(분석) |
| C1-b | 조회: `OS_CountSemGetIdByName` | 0 syscalls `[M-probe 기록: 위와 같은 조건]` | 없음 | **대응 경로 없음**(분석) |
| C1-c | 수정 후 재시도 대기: `OS_TaskDelay(100)` × 25 | sleep 계열 syscall로 추정 `[I 소스, 미실행]` | Table 2 객체를 건드리지 않아 micro-op 없음 | **대응 경로 없음**(분석) |
| C1-d | 조회 반환값 `OS_ERR_NAME_NOT_FOUND`와 CF 초기화의 최종 결과(`CF_EID_ERR_INIT_SEM` 이벤트, `APP_ERROR` 종료) | 응용의 함수 반환값은 기록 대상으로 열거되지 않음(§2.2 Q1) | 기록 입력에 없음. 실패 기준(CF 초기화 abort)은 사용자가 checker로 써야 함(C1 Q4). 대응하려면 checker가 abort를 실패로 보고하고 보고된 앵커 쌍이 생성↔조회여야 하며 후자는 경로 없음(C1 Q6) | **검증층**(분석) |

원 코드 규칙: 이름 `BP0_tsem`(공개 BP 구성의 값, D22 기반. 원 보고자 환경의 이름은 미확인) → OSAL ID → 사용자 공간 테이블 항목은 RacePro의 식별 장치(경로 이름, 파일 핸들, pid)에 대응하지 않는다. 커널에는 대기·wake 때에만 futex 주소로 나타나며 이름과 이어지지 않는다 `[M-probe 기록]`(경합이 있으면 type lock 주소의 `FUTEX_LOCK_PI`와 condvar 대기가 보일 수 있고 이름은 싣지 않는다. `availability.md` §4.5). 코드는 FUTEX를 RESOURCE 후보에서 제외한다. 세대(재생성된 ID)는 논문에 서술이 없다. 회복과 최종 실패를 구분하는 출력 항목은 논문에 없다.

### 6.4 C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

사례 쪽 사실(저장소 기록): SB task의 `CFE_SB.AppId`, EVS task의 `EVS_AppID`와 등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 호출하고 `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 원 플랫폼은 Microblaze이다. 오류 지점은 사례 기록에서 미확인이다. 6.4.x 소스 대응은 D13·Q16 [D]의 별도 작업이다.

| # | C3 사건 | 커널에서 보이는 흔적 | RacePro 입력 | 대응 라벨 |
|---|---|---|---|---|
| C3-a | SB `AppId` 저장, EVS `EVS_AppID`·등록 플래그 설정 | syscall 없음 `[I 소스, 미실행]`(읽은 소스는 번들. C3의 6.4.1 코드에서는 읽지 않음) | 없음 | **대응 경로 없음**(분석) |
| C3-b | TIME의 `CreatePipe` | `mq_open` `[M-probe 기록: OSAL d2d877a의 OS_QueueCreate 단위]`. TIME의 `CreatePipe`와의 연결은 `[I 소스]`이고 C3의 원 판본(OSAL 4.1.1 posix)의 queue 구현이 `mq_open`을 쓰는지는 확인하지 않음 | `mq_open`이라면 R2 ③과 같다: FILES_STRUCT 쓰기(fd table)만 자원 접근이 되고 큐 이름·큐 객체는 Scribe 자원이 아님(K1–K3). 그 쌍은 의존 상태(AppId, 등록)와 이어지지 않음 | **분석(조건부)**(2026-10-15판의 "불명"에서 바뀜) |
| C3-c | TIME의 이벤트 요청(AppId=0)과 `EVS_IsFiltered`의 범위 검사 없는 접근 | 사용자 공간의 함수 호출과 전역 메모리 읽기 `[I 소스]` | 경합 분석 입력(커널 객체 접근)에 나타나지 않음 | **대응 경로 없음**(분석) |
| C3-d | 의존 상태의 식별: AppId 정수(ES가 부여)와 EVS 배열 항목 | 커널 객체가 아님(Table 2) | 식별 장치에 대응하지 않음 | **대응 경로 없음**(분석) |
| C3-e | crash(범위 밖 접근의 결과) | — | built-in checker에 segfault가 있어 crash는 오류 결과로 볼 수 있으나 잘못된 상태 사용과 crash를 구분하는 출력은 없음. crash가 일어나는지 자체가 사례 기록상 미확인 | **검증층**(분석) |

### 6.5 C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

사례 쪽 사실(저장소 기록): `CFE_ES_AppCreate`가 `OS_TaskCreate` 전에 ES lock을 풀고 뒤에 다시 잡아 `TaskTable` 항목을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다. 일찍 실행된 것만으로는 위반이 아니고 위반은 "등록 전 조회"이다. 수정(6.4.2)은 lock 범위 확장이다.

| # | C5 사건 | 커널에서 보이는 흔적 | RacePro 입력 | 대응 라벨 |
|---|---|---|---|---|
| C5-a | `OS_TaskCreate` → `pthread_create` → `clone3` | OSAL `d2d877a`의 `count-sem-test`에서 `OS_TaskCreate` 아래 `clone3` ×4(이름 없음) `[M-probe 기록]` (4)(`osal-visibility.md` stack 귀속 표). 에이전트가 쓴 작은 C 프로그램에서도 `pthread_create` → `clone3`가 보였다(기록, `availability.md` §4.5 끝. RacePro.md 보충표가 인용하는 근거). cFS에서 측정한 것도, C5의 OSAL 4.1.1 시대에서 본 것도 아님 | `clone`은 HB 노드·간선(코드가 `clone`을 fork로 취급). "clone → 자식의 첫 syscall" HB는 표현되나 "등록 공개 완료"라는 사건은 없음 | **대응(부분)**(분석) |
| C5-b | ES lock 해제·재획득 사이의 `TaskTable` 항목 쓰기 | 쓰기 자체는 메모리 저장이라 syscall이 없는 것으로 읽힘 `[I 소스, 미실행]`(읽은 소스는 번들, 6.4.x 코드에서는 읽지 않음). "0 syscalls"로 쓰지 않는다. 쓰는 task와 조회하는 task가 ES lock에서 겹치면(6.4.1의 `CFE_ES_GetAppID`는 ES lock을 잡는다: `cases/C5.md` F5) lock 주소의 futex(PI mutex라면 `FUTEX_LOCK_PI`)가 보일 수 있다(`availability.md` §4.3, §4.5). 이름·레코드 내용은 싣지 않는다 | 없음. 등록 쓰기를 `clone` 앞뒤로 옮기는 branch는 만들 수 없음 | **대응 경로 없음**(분석) |
| C5-c | 새 task의 `CFE_ES_GetAppID` 조회와 `CFE_ES_ERR_APPID` 반환 | 조회 자체는 메모리 조회 + `pthread_getspecific` `[I 소스]`. 조회가 ES lock을 기다리면 lock 주소의 futex 대기가 보일 수 있음. OSAL task table의 레지스트리 대기가 lock·condvar 주소의 futex 대기로만 보인 것은 `osal-core-test` 1회 관측 `[M-probe 기록]`이며 ES lock의 관측이 아님. 6.4.x 시대의 ES lock 구현은 확인하지 않음 | 없음. 참고: 코드는 FUTEX를 RESOURCE 경합 대상에서 제외(`racecore.py:194, 282`) | **대응 경로 없음**(분석) |
| C5-d | PSP의 `prctl(PR_SET_NAME)` | `[I 소스]`(`availability.md` §4.3). ES 레코드 쓰기와 순서가 정해지지 않음 | 이름 설정은 Table 2 객체가 아님 | **대응 경로 없음**(분석) |
| C5-e | 반환값 `CFE_ES_ERR_APPID` | — | built-in checker 4종은 내부 API 반환값을 보지 않음. 반환값을 보는 사용자 checker가 필요. 원 보고에 없는 앱 crash는 기대 결과로 넣지 않음(로드맵 §9.3) | **검증층**(분석) |

### 6.6 C2 — cFE#198 (알려진 조건만)

사례 쪽 사실(저장소 기록): 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식과 초기화 내용이 공개되지 않았다. 공개된 것은 단일 단계 시작 동기화(`WaitForStartupSync`)와 6.6.0a의 두 단계 방식(`APPS_INIT`/`LATE_INIT`, `CFE_ES_WaitForSystemState`)이다. 이 절은 공개된 부분만 쓰며, P·R의 사건은 정할 수 없다. 결과 명칭은 "C2 메커니즘의 역할 기반 검증"이고 원 비공개 시스템의 재현이 아니다(P-C2 [D]).

| # | C2 사건(공개 조건) | 커널에서 보이는 흔적 | RacePro 입력 | 대응 라벨 |
|---|---|---|---|---|
| C2-a | `CFE_ES_WaitForSystemState`가 `while (SystemState < MinSystemState)`를 돌며 `OS_TaskDelay` | 상태는 `CFE_ES_Global`의 메모리 필드, 대기는 sleep 계열 syscall `[I 소스, 미실행]` | sleep은 Table 2의 객체를 건드리지 않아 micro-op이 만들어지지 않음. 상태 필드는 메모리 [I] | **대응 경로 없음**(분석) |
| C2-b | 단일 단계 / 두 단계 barrier의 상태 전이와 timeout | 같은 메모리 필드 | 위와 같음 | **대응 경로 없음**(분석) |
| C2-c | P의 late init 완료와 R의 의존 호출 | 비공개 | 앵커가 될 쌍을 알 수 없음 | **불명** |

코드 근거는 `cfe_es_api.c:514, 585, 603`에 대한 `racepro.md` §6의 서술이며 재검증하지 않았다. "sleep이 micro-op을 만들지 않는다"는 해석은 Table 3의 syscall 템플릿과, 기록기가 사용자 공간에서 보이는 커널 객체만 추적한다는 서술(§4 p.357)에서 도출했다 [I]. 논문 도구 모델에는 시작 단계나 준비 상태 개념이 없다. 다만 의존이 거치는 OS 자원이 비공개라 도구가 그 관계를 표현할 수 있는지는 판단할 근거가 없다. 역할 재구성의 요청 방식(P-C2)이 정해진 뒤에야 같은 분석을 할 수 있다.

### 6.7 대응하지 않는 사건 종합

| 사례 | 대응 경로 없음(분석) | 불명 | 대응(부분)·조건부 | 검증층 |
|---|---|---|---|---|
| R2 | ① AppId 설정, ② 값 전달, ④ 오류 반환 (3) | — (2026-10-15판의 ③ 불명은 코드 읽기로 해소) | ⓪ `clone`(HB 간선의 원천), ③ `mq_open`의 FILES_STRUCT 쓰기(fd table 쌍이며 AppId와 무관) (2) | ④ 거부 결과를 보려면 사용자 checker. 보고 조건 "녹화 때 없던 실패"(H-RP-5) |
| C1 | C1-a 생성, C1-b 조회, C1-c 재시도 대기 (3) | — | — | C1-d 반환값과 초기화 최종 결과 |
| C3 | C3-a 저장·등록 플래그, C3-c 이벤트 요청·범위 검사 없는 접근, C3-d 의존 상태 식별 (3) | — | C3-b TIME의 `CreatePipe`(OSAL 4.1.1 queue가 `mq_open`이라면 R2 ③과 같음) (1, 조건부) | C3-e crash |
| C5 | C5-b 등록 쓰기, C5-c 조회·반환, C5-d `prctl(PR_SET_NAME)` (3) | — | C5-a `clone`(HB 노드·간선) (1) | C5-e 반환값 |
| C2 (공개 조건) | C2-a 상태 polling 대기, C2-b barrier 전이 (2) | C2-c P·R의 사건 (1) | — | — |

읽는 법: 이 표는 **RacePro의 설계 기술과 코드 읽기에서 도출한 분석**이다. (1) 어느 칸도 RacePro가 사례를 탐지하지 못한다는 뜻이 아니다. 논문은 go-live 뒤 자연 발생한 실패를 checker가 보고할 수 있다고 인정한다(p.363). (2) "RacePro는 공유 메모리를 기록하지 못한다"는 틀리다. 기록은 하되 HB와 후보에 쓰지 않는다(p.357–358). (3) 이 표의 "대응 경로 없음"을 메우기 위해 OSAL 레지스트리를 자원으로 추가하거나 `mem_owned_*` 사건을 후보 생성에 쓰는 확장은 `EXTENSION`이며 원 도구의 결과가 아니다. (4) ③과 C3-b의 결론은 "SB pipe 통신이 도구에 보이지 않는다"가 아니다. fd table 접근은 잡히고 큐 이름·큐 객체는 Scribe 자원 잠금 경로가 보이지 않는다는 코드 읽기(받은 디렉터리만 검색)이며 실제 로그는 실행하지 않아 미확인이다. 다섯 사례의 의존성은 SB 메시지 전달이 아니라 AppId·레지스트리 상태라서 RacePro.md는 이것을 가설로 올리지 않았다. (5) 이 분석이 가리키는 Kairos 기능 가설은 H-RP-1(R2·C3·C5·C1), H-RP-4(R2)이고, H-RP-5(R2)는 목적 차이에 대한 참고다. 어느 것도 §4.2에서 확인된 부족이 아니다. (6) "syscall 없음"의 근거는 `[M-probe 기록]`(OSAL `d2d877a`, 경합 없음)과 `[I 소스]`로 구분했다. 어느 쪽도 사례 revision(C1의 OSAL `42af0f73`, C3·C5의 OSAL 4.1.1 시대, R2의 B/M/D)에서 syscall을 관측한 결과가 아니고, gdb 기록은 syscall 근거가 아니다.

### 6.8 참고: 다른 도구의 R2 사건 상태

R2 사건 다섯 행(⓪–④)에 대해 각 도구별 파일이 쓴 상태를 모았다. RacePro를 제외하면 이 문서의 첫 산출물이 아니며 도구별 파일의 표를 옮긴 것이다. "—"는 그 파일에 해당 행이 없다는 뜻이다.

| 도구 | ⓪ core task 생성·S1 | ① AppId 설정 | ② 값 전달 | ③ 이벤트 요청(`mq_open`) | ④ 오류 반환 | 파일의 형식 |
|---|---|---|---|---|---|---|
| RacePro | 대응(부분) — `clone`이 HB 간선의 원천, RUNNING 폴링은 간선 아님 | 대응 경로 없음(분석) | 대응 경로 없음(분석) | 분석 — FILES_STRUCT(fd table) RESOURCE 쌍 가능, AppId와 무관(코드 읽기) | 대응 경로 없음(분석) | 사건 대응표 + 후보 클래스별 대조표 |
| SimRacer | SIMEXPLORER 설계: 분석(fork 규칙이 `clone`에 적용되는지 서술 없음). SimRacer 자체: 불명 | SIMEXPLORER 설계: 분석(대상 주소가 SR이면 memory write, 생성 뒤 쓰기는 다른 task와 HB 없음). SimRacer 자체: 불명 | SIMEXPLORER 설계: 분석. SimRacer 자체: 불명 | 불명 — `mq_open`이 23개 syscall에 드는지 서술 없음, SimRacer의 syscall 목록도 못 읽음 | 분석 — SR이 아님, 출력 기반 oracle 대상이 아닌 내부 반환값 | 사건 대응표(라벨 열 둘) |
| SIMEXPLORER | — | 분석 — SR 지정이 전제 | 분석 | 불명 | 분석 — 반환값은 사건 목록에 없음 | 사례 칸만 |
| RRF | 분석 — `clone`은 RacePro의 추적 syscall 예(p.2)이나 경로 문자열 인자가 없어 자원 이름 거르기에서 관련 이벤트가 되는 경로가 서술에 없음. 시작 직렬화 모델링 서술 없음 | 분석 — 같은 자원의 syscall 쌍이 아님 | 분석 — 위와 같음 | 불명 — 엔지니어가 정한 자원 이름과 일치할 때만 관련 이벤트. Fig. 2 형식상 큐 이름을 자원 이름으로 주면 `mq_open` 이름 인자로 거를 수 있다는 분석이 있으나 그 쌍은 SB pipe 이름에 대한 것이고 AppId가 아님 [I] | 분석 — 기록 대상이 아님 | 사건 대응표 |
| DESCRY | 분석 — `clone`의 `/proc` inode 생성 모델(p.3)과 fork-return(p.7)이 thread 생성에 적용되는지 서술 없음 | 대응 경로를 찾지 못함(분석) | 대응 경로를 찾지 못함(분석) | 분석 — `mq_open`이 자원 접근 사건으로 모델링되는지 불명, 되더라도 짝이 될 ① 쓰기가 사건 집합에 없음 | 대응 경로를 찾지 못함(분석) — failure point가 아니고 e_f를 고를 기준이 없음 | 사건 대응표(2026-10-16 신설) |
| SimEvo | — | 서술된 사건 종류에 없음 | 위와 같음 | — | 위와 같음 | 사례 칸만 |
| ReDPro | — | 불명 | 불명 | 불명 | 불명 | 사례 칸만(조건부 Q5 제외) |
| SysPro | 분석 — 보고서가 task 생성·시작 순서를 이름 붙이지 않음, `clone3`가 호출 지점이 되는지 서술 없음 | 분석 — 직접 대응하는 syscall 호출 지점 없음 | 분석 — 위와 같음 | 불명 — `mq_open`은 Linux syscall 이름이라 keyword 검색 대상일 수 있으나 OSAL 구현 파일에 있고 SB API 파일에 없음, 지연 방향 불명 | 분석 — 재현 판정 방법 서술 없음 | 사건 대응표(2026-10-16 신설) |

③ 칸의 `mq_open`은 `[M-probe 기록]`(OSAL `d2d877a`, `OS_QueueCreate` 단위)이고 `CreatePipe`와의 연결은 `[I 소스]`다. ⓪의 `clone3`는 `[M-probe 기록]` (4)(§0.1)이며 cFS에서 측정하지 않았다. 다른 칸의 "syscall 없음" 유형 서술도 같은 구분을 따른다(§0.1). SimRacer.md와 DESCRY.md는 ⓪이 들어가도 ①과 ②가 HB 없는 쌍으로 남는다는 전제(사용 task가 생성자 thread인 ES 시작 thread가 아님)를 사례 기록(`case_basis_register.md` R2.E3)에 기대며 코드로 다시 확인하지 않았다고 적는다.

## 7. 도구별 파일의 얇은 곳과 이 문서의 한계

도구별 파일 여덟 개가 모두 있다. 없는 파일은 없다. 여덟 파일 모두 2026-10-16에 보완됐다. 아래는 얇거나 서로 어긋나는 곳이다.

| # | 대상 | 내용 |
|---|---|---|
| 1 | ReDPro.md | 논문 본문을 읽지 못해 초록·서지·2차 문장뿐이다. 30칸 중 26칸이 불명이고 답은 0칸이다. 방법·입력·관측 사건·평가 대상이 모두 불명이다. 2026-10-16 재시도에서도 전문은 없고 Internet Archive 스냅숏 하나가 egress 정책 때문에 열리지 않는다. 이 문서도 역할 표에 ReDPro를 확정 행으로 넣지 않았다 |
| 2 | SimRacer.md | ISSTA'13 본문을 읽지 못했다. 2026-10-16에 Q1·Q2를 불명으로 바꿔 사례별 25칸 중 13칸이 불명이다. 남은 Q3·Q5·Q6 분석은 STVR(SIMEXPLORER) 서술에 기대며 SimRacer 자체와 같은지는 SimRacer.md §2.3의 범위에서만 말할 수 있다. Sim-O/C(ITJ 2013)는 같은 저자·같은 해의 서술이지만 SimRacer라는 이름이 없어 같은 도구라는 판단은 [I]다 |
| 3 | RRF.md | 학위논문과 IEEE 판본을 읽지 못했다. 내부 oracle 논문 [36]은 초록만 읽었다. 구현 상세(정적 분석 도구, Scribe 판본, oracle 인터페이스)가 없어 H-RRF-1·3이 부분·조건부 근거에 머문다. 가설 번호 H-RRF-2는 결번이다(RRF.md §1.3 항목 12: 다른 문서가 1·3·4·5로 인용하므로 번호를 다시 매기지 않음) |
| 4 | SimEvo.md | 사건·자원 모델 세부를 RacePro·SimRacer에 위임하며 SimRacer 원문을 읽지 못했다. 가설 ID가 SIMEXPLORER.md와 겹친다. R2 사건표가 없다 |
| 5 | DESCRY.md, SysPro.md, RRF.md, SimEvo.md | 사례 이슈 댓글을 읽지 못했다. 실패 로그·재현 서술 유무가 입력 조건 판단에 걸려 있다. DESCRY.md는 C2의 댓글 수 기록이 충돌한다고 적는다(검색 메타데이터 11, 로그아웃 HTML 기준 없음). R2 댓글 2건은 snippet 조각만 기록돼 있다 |
| 6 | C2 전체 | 여덟 파일 모두 앱 P·R이 비공개라 C2 칸이 대부분 불명이다. C2에 대한 도구 부족 가설을 세운 파일이 없다 |
| 7 | RacePro.md R2 사건표의 task 생성 행 | 2026-10-15판이 지적한 공백은 2026-10-16에 ⓪ 행으로 메워졌다. RRF.md·SimRacer.md·DESCRY.md·SysPro.md도 같은 행을 넣었다(§6.8) |
| 8 | SimRacer.md와 SIMEXPLORER.md | 같은 STVR 근거인데 Q1·Q2 6칸의 라벨이 다르다(§2.5 항목 1). 대상(SimRacer 자체 대 STVR의 도구)이 달라서이며 두 파일의 Q3·Q5·Q6 칸은 내용이 상당히 겹친다 |
| 9 | ReDPro.md | 도구 수준 Q6(분석)과 사례별 Q6(불명)의 차이는 2026-10-16에 ReDPro.md §3.4가 이유를 적었다(§2.5 항목 4) |
| 10 | RacePro.md와 나머지 파일 | R2 순서의 성격이 다르게 적혀 있다. RacePro.md(Q5 칸, H-RP-4)는 순서가 core 시작 직렬화로 "고정된다(사례 기록)"고 쓰고, 같은 파일 Q6과 H-RP-5는 "코드상 매 부팅 결정적이라는 예측(미실행)"으로 쓴다. SimRacer.md·RRF.md·DESCRY.md·SysPro.md·ReDPro.md는 고정인지 스케줄 의존인지가 사례 기록상 미결이라고 쓴다. 이 문서는 미결로 쓴다. gdb 3회의 같은 결과는 결정성의 증명이 아니다(`results/R2_gdb_v701.md` §3) |
| 11 | 모든 파일 | "분석" 칸은 실행하지 않은 설계 기술 분석이다. OSAL 자체 시험의 probe는 어느 도구의 결과도 아니고 cFS 실행도 아니다. 이 probe는 OSAL `d2d877a`의 경합 없는 실행에서 §0.1의 네 가지만 뒷받침한다(C1의 OSAL `42af0f73`, C3·C5의 OSAL 4.1.1 시대에서는 실행하지 않았다). C3·C5·R2의 의존 상태 접근은 소스 수준 진술이며 "도구 X가 사례 Y를 놓친다"는 뒷받침하지 않는다 |
| 12 | DESCRY.md 대 SysPro.md·RacePro.md·SimRacer.md (2026-10-16) | `OS_TaskCreate` → `clone3`의 근거 표기가 다르다(§2.5 항목 9). 이 문서는 저장소 기록을 다시 읽고 OSAL 시험의 기록을 `[M-probe 기록]` (4)로 썼다 |
| 13 | RRF.md (2026-10-16) | R2 ④·Q1 칸이 `results/R2_gdb_v701.md`의 "no syscall was made"를 인용한다. 이 문서는 그 문장을 근거로 쓰지 않는다(§2.5 항목 10) |
| 14 | SIMEXPLORER.md·SimEvo.md (2026-10-16) | R2 사건표가 없다. SIMEXPLORER의 설계 기준 R2 사건 분석은 SimRacer.md의 "SIMEXPLORER 설계" 열이 대신한다 |

이 문서의 한계: (1) 도구별 파일의 칸을 압축해 옮겼으며 쪽 번호 등 근거는 해당 파일의 같은 칸에 있다. 이 문서는 논문·코드를 다시 읽지 않았다. 2026-10-15 검토 반영 때 RacePro p.356과 DESCRY p.1·p.3의 인용 두 곳만 저자 호스팅 PDF로 대조했고(§9), 2026-10-16에는 도구별 파일끼리 어긋난 `clone3` 근거를 가리기 위해 저장소 기록(`baselines/osal-visibility.md` stack 귀속 표, `baselines/availability.md` §4.2·§4.5)만 다시 읽었다(§2.5 항목 9). 원출처로 확인이 필요하면 도구별 파일의 근거 칸을 거친다. (2) 도구별 파일은 2026-10-15 UTC 08:12–09:50에 작성되고 2026-10-16 UTC 00:00–01:10에 보완됐으며 다른 담당이 이후에 고칠 수 있다. §8.1의 해시가 읽은 판본이다. (3) 같은 계보인 SimRacer·SIMEXPLORER·SimEvo·RRF·DESCRY·SysPro는 서로의 서술을 인용하므로 독립 근거 여섯이 아니다. (4) 라벨의 개수(§2.3)는 점수가 아니다. Q4는 도구당 1건으로 세었고, SimRacer·SIMEXPLORER의 겹침 때문에 200칸은 독립 근거의 수가 아니다. (5) STVR·RRF 기반 쪽 번호 인용은 단일 취득 경로이며 독립 재확인이 없다(§1.1).

## 8. 출처

### 8.1 읽은 도구별 파일 (읽은 판본)

2026-10-16판이 읽은 판본(파일 수정 시각은 UTC):

| 파일 | 크기(바이트) | 줄 | 수정 시각(UTC) | sha256 앞 16자 |
|---|---:|---:|---|---|
| `plan/methods/RacePro.md` | 64945 | 457 | 2026-10-16 00:09 | `02f921ce132c88c8` |
| `plan/methods/SimRacer.md` | 92379 | 505 | 2026-10-16 00:21 | `0a8ab54390c1ccab` |
| `plan/methods/SIMEXPLORER.md` | 73225 | 436 | 2026-10-16 00:15 | `d32a569fce01f83b` |
| `plan/methods/RRF.md` | 80130 | 440 | 2026-10-16 00:22 | `fb39c1f05d938589` |
| `plan/methods/DESCRY.md` | 95603 | 482 | 2026-10-16 00:32 | `f1f523d3d95e1336` |
| `plan/methods/SimEvo.md` | 68761 | 469 | 2026-10-16 00:28 | `6de44b43e56a40e5` |
| `plan/methods/ReDPro.md` | 68263 | 475 | 2026-10-16 00:37 | `deae34c4432499a0` |
| `plan/methods/SysPro.md` | 99112 | 490 | 2026-10-16 00:40 | `bad2d37b6f566006` |

2026-10-15판이 읽은 판본(기록):

| 파일 | 크기(바이트) | 줄 | 수정 시각(UTC) | sha256 앞 16자 |
|---|---:|---:|---|---|
| `plan/methods/RacePro.md` | 52396 | 421 | 2026-10-15 08:27 | `25bb2518ef1775f5` |
| `plan/methods/SimRacer.md` | 70959 | 454 | 2026-10-15 08:51 | `62460032254cbc87` |
| `plan/methods/SIMEXPLORER.md` | 68322 | 427 | 2026-10-15 09:08 | `5c66e29edbb53855` |
| `plan/methods/RRF.md` | 66823 | 415 | 2026-10-15 09:05 | `ec2b3feaface3248` |
| `plan/methods/DESCRY.md` | 71085 | 431 | 2026-10-15 09:29 | `bee9b5887dbffe7b` |
| `plan/methods/SimEvo.md` | 56726 | 435 | 2026-10-15 09:26 | `e717eeb417c8f364` |
| `plan/methods/ReDPro.md` | 57335 | 444 | 2026-10-15 09:41 | `e3bc7d4774b4c7a4` |
| `plan/methods/SysPro.md` | 77705 | 433 | 2026-10-15 09:43 | `3e71f1076edf06e7` |

### 8.2 함께 읽은 저장소 문서

- [ROADMAP_20261015.md](ROADMAP_20261015.md) §1.2·§1.4·§3·§9·§10·§12(sha256 앞 16자 `59c495b120423dca`).
- [PI_DECISIONS_RESOLVED_20261015.md](../Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md): 이 문서가 쓰는 결정은 T1, P-PAPERS, P-CLASSGT, D12, P-C2, P-C1HOLD, D13·Q16, D22, P-R2CTRL, P-ABL-ALLOWED이다. 결정으로만 인용했고 사실로 쓰지 않았다(앞 16자 `9222d88ce482b92b`).
- [CONDITIONS_POLICY.md](../CONDITIONS_POLICY.md).
- `baselines/availability.md` §4–§5(§4: OSAL probe의 설정·결과·한계 [M-probe 기록]의 출처. §5: 결과 라벨의 정의, 허용·금지 문구), A12.
- `baselines/osal-visibility.md` §0 D1·D4, §8, §9, `cases/C1.md`, `cases/C3.md`, `cases/C5.md`, `cases/R2.md` §9, `cases/mapping_v701.md` §0–§1: 검토 반영 때 syscall 흔적의 revision 한계와 사례 쪽 revision을 대조하는 데 썼다.
- `results/R2_gdb_v701.md`: R2 설정 전 사용 24회와 "독립 결함 24개가 아님"의 확인에만 썼다. gdb 기록이 syscall을 관측하지 않았다는 점은 §0.1에 있다.
- 2026-10-16에 다시 읽은 저장소 기록: `baselines/osal-visibility.md`의 "Stack attribution" 표(`OS_TaskCreate` 행: `clone3` ×4, 이름 없음)와 `count-sem-test` 창 서술, `baselines/availability.md` §4.2(OSAL 시험 결과 1–6)와 §4.5 끝(에이전트 C 프로그램과 OSAL probe의 관계). §2.5 항목 9의 근거로만 썼다. 다시 측정하지 않았다.

### 8.3 원출처

이 문서는 논문과 코드를 직접 인용하지 않는다. 예외는 검토 반영 때 저자 호스팅 PDF로 대조한 RacePro p.356과 DESCRY p.1·p.3의 인용 두 곳이며 §9.1에 있다. 원출처의 서지, 해시, 접근 경로, 쪽 번호의 기준 판본은 각 도구별 파일의 §1과 §7에 있다.

## 9. 검토 지적의 처리 (2026-10-15)

이 절은 2026-10-15판에 대한 검토 처리의 기록이다. 수치와 반영 위치는 그날 판본 기준이며 2026-10-16 갱신으로 바뀐 것은 §10에 있다(예: 지적 4의 "50칸 중 48칸"은 2026-10-16판에서 44칸, 지적 2·9의 칸 문장 일부는 보강됨).

검토가 지적한 9건(major 1, minor 8)을 해당 출처와 대조했다. 반영 위치는 아래 표에 있다. 일부 세부 제안은 기각했고 §9.2에 이유를 적었다.

### 9.1 처리 결과

| # | 지적 | 대조한 출처와 확인 | 반영 위치 |
|---|---|---|---|
| 1 (major) | "syscall 없음/0 syscalls"의 근거 구분 없음, probe의 revision 한계 누락 | `availability.md` §4.2–§4.5(OSAL `d2d877a`, 경합 없음, "One OSAL revision only", C3·C5·R2는 source-level), A12(C1의 OSAL `42af0f73`), `osal-visibility.md` §8(C3·C5는 번들 소스만 읽음)·§9, 로드맵 §3 "R2 결과 표현의 과장" 행. 지적이 맞다. C5의 조회는 `cases/C5.md` F5상 ES lock을 잡으므로 생성 task와 겹치면 futex가 보일 수 있다 | §0.1 표기 규칙, §0.2 항목 7, §2.4 칸, §6 서두와 §6.2–§6.6(C5-b/c의 "0 syscalls" 삭제, ③의 "69회"를 `osal-core-test`의 `OS_QueueCreate` 단위로 한정하고 `CreatePipe` 연결은 `[I 소스]`로), §6.7 (6), §7 항목 11 |
| 2 | §5.4 항목 1과 §3.3이 도구별 기본값을 잘못 적고 C2 입력 미충족 안과 ReDPro D-RD-2·3 누락 | RacePro.md P-2, SimRacer.md P-2, SIMEXPLORER.md P-S2, RRF.md §4.3-1, DESCRY.md §4.3-1(c), SimEvo.md §4.3-1, SysPro.md §4.3-1(c), ReDPro.md D-RD-2·3. DESCRY의 PuD 소스 요구는 DESCRY PDF p.1·p.3에서 확인(§9.1 지적 9). 지적이 맞다(세부 하나는 §9.2) | §5.4 항목 1(도구별 표), 14, 15, §3.3, §3.1, §0.2 항목 1 |
| 3 | §4.3이 불명인 것을 "이미 가능"으로 나열, §2.5-7에 SCMiner 누락, ReDPro Q1에 초록의 syscall 순서 서술 누락 | SimEvo.md §1.4("어느 쪽도 쓰지 않는다"), SimRacer.md §2.3(SCMiner p.11 `2차`), ReDPro.md §0 항목 5·§2.1. 지적이 맞다 | §4.3 표와 "불명" 표, §2.5 항목 7·8, §0.2 항목 4, §2.2 ReDPro Q1, §4.2 H-RD-1 |
| 4 | 240칸 합계가 공유 근거와 도구 수준 답을 중복 집계 | §2.1·§7 한계(3)의 자체 규칙과 §2.3 표를 대조. SimRacer와 SIMEXPLORER는 Q4를 뺀 50칸 중 48칸이 같고 다른 2칸은 C1·C5의 Q2. 지적이 맞다 | §0.2 항목 3, §2.1, §2.3(표를 Q4 / 나머지로 분리, † 표시), §7 한계 (4) |
| 5 | `ANALYSIS_GAP` 한정구 누락, S6이 `DEVIATION-RUN` 부족을 원 도구 부족으로 승격 | 로드맵 §10.4 "해당 입력·기준에서 분석의 부족으로 보고"와 이 문서 §3.1을 대조. `availability.md` §5.1·§5.2(`DEVIATION-RUN`은 요구 대체). 지적이 맞고 S6은 로드맵에 직접 근거가 없는 [제안]이다 | §3.1, §4.1, §5.3 S6 |
| 6 | `BP0_tsem`을 사례의 이름처럼 씀 | `cases/C1.md`("Whether [C1-BP] used them is unknown"), PI_DECISIONS_RESOLVED §3.2 D22("공개 구성의 값과 원 보고자 환경의 값은 구분한다"). 지적이 맞다 | §2.4.3 SIMEXPLORER C1 Q2, §6.3 |
| 7 | §5.5의 SPEC_FINAL.md 줄 번호가 유효하지 않음 | SPEC_FINAL.md는 3701줄이고 662·3349줄에 RacePro 등급이 없음을 확인. 같은 내용이 §1.11 출처 등급 표, Appendix B, §8.3 Roles 표에 있음. 지적이 맞다 | §5.5 |
| 8 | SIMEXPLORER·RRF 전문의 취득 방식 미공개 | SIMEXPLORER.md 경로 13–14, SimRacer.md 경로 18, SimEvo.md 경로 29, DESCRY.md 경로 18, RRF.md 경로 1–2. 지적이 맞다. 이 문서는 두 파일을 다시 열지 않았다 | §1.1 취득 경로, §2.1, §5.1 A5·A10, §5.4 항목 16, §7 한계 (5) |
| 9 | 작은 서술·인용 오류 넷 | (a) RacePro.md §4.1·§1.2 표(userland는 `unspecified_by_reference`). (b) RacePro PDF(sha256 `645a21ba…`) PDF 4쪽(인쇄 쪽 356)의 "records all nondeterministic interactions between applications and the OS"를 대조. (c) DESCRY PDF(sha256 `f9d7b57b…`) p.1("DESCRY only assumes the existence of the source code of processes under debugging (PuDs)")과 p.3(§2.2 Problem Statement)를 대조했고 p.2에서는 같은 요구 문장을 찾지 못했다. (d) `cases/mapping_v701.md` §0·§1.1(v7.0.1 `c5fb2b4d`: L185-188. 이 파일은 M과 diff 없음), `cases/R2.md` §9의 표(B L182-186, M·D L185-189). 모두 지적이 맞다 | (a) §5.2 RacePro 행. (b) §2.2 RacePro Q1. (c) §2.4.5 C2 Q1. (d) §2.4.3 R2 Q6, §2.4.5 R2 Q6, §6.2 ④ |

지적 1의 제안은 `[M-probe]`를 C1 생성·조회와 queue `mq_open`에만 쓰라고 했다. 이 문서는 `osal-core-test`의 task registry futex 대기 1회를 세 번째로 더했다. §6.5 C5-c 칸이 그 관측을 인용하기 때문이며, 그 관측은 OSAL task table lock·condvar이고 ES lock이 아니라고 칸에 적었다.

### 9.2 Rejected findings (세부 제안의 기각)

지적 9건 모두의 핵심은 받아들였다. 아래 세부 제안만 기각했다.

| 대상 | 기각한 제안 | 이유 |
|---|---|---|
| 지적 4의 대안 | SimRacer와 SIMEXPLORER의 Q2 라벨을 같은 근거이므로 통일 | 두 라벨은 도구별 파일 작성자가 다른 가정으로 붙였다. SimRacer.md는 "같은 SR 판정 기준 = 주소" 가정 [I]을 붙여 분석으로, SIMEXPLORER.md는 "SR_i equals SR_j 기준 서술 없음"을 불명으로 뒀다. 이 문서는 도구별 파일에 없는 라벨을 새로 만들지 않으며 두 파일은 이 작업의 소유가 아니다. 대신 겹침을 §2.3 합계에 표시하고 총합을 독립 근거 수로 읽지 않게 했다 |
| 지적 2의 일부 | RRF와 SimEvo도 "C2 입력 미충족을 `INAPPLICABLE`로" 하는 안을 가진다는 서술 | RRF.md §4.3-1과 SimEvo.md §4.3-1에는 (c)안이 없다. 두 파일은 C2의 입력 사유(RRF p.5의 PuD 소스 요구, 앱 소스 비공개)를 적었을 뿐 `INAPPLICABLE` 선택지로 올리지 않았다. §5.4 항목 14에는 네 도구를 넣되 DESCRY·SysPro는 "안 (c) 명시", RRF·SimEvo는 "같은 사유만 기재"로 구분했다 |
| 지적 8의 일부 | STVR·RRF 쪽 번호 인용을 이 문서에서 재확인 | 지적은 둘째 경로가 없을 때 "독립 재확인 안 됨" 표시를 요구했다. CPS-VO 호스트는 WAF가 거부하는 곳이라 이 문서에서 다시 열지 않았고, 표시와 PI 확인 항목(§5.4 항목 16)만 두었다 |

## 10. 2026-10-16 갱신 내역

도구별 파일 여덟 개가 2026-10-16 UTC 00:00–01:10에 보완됐다(각 파일 머리말의 "보완" 줄). 이 문서는 그 판본(§8.1)을 다시 합성했다. 새 논문 조사, 도구 실행, 설치, 빌드, 저자 연락은 하지 않았다. 도구별 파일끼리 어긋난 `clone3` 근거를 가리기 위해 저장소 기록 두 개만 다시 읽었다(§8.2).

| # | 바뀐 것 | 출처(도구별 파일) | 이 문서의 위치 |
|---|---|---|---|
| 1 | SimRacer의 C1·C3·C5·R2 Q1·Q2 8칸이 분석 → 불명. 합계 분석 140 → 132, 불명 60 → 68. SimRacer·SIMEXPLORER의 라벨 차이가 C1·C5 Q2 2칸에서 Q1·Q2 6칸으로 바뀜 | SimRacer.md §1.3 항목 8–10, §3.4 | §0.2 항목 3, §2.1, §2.3, §2.4.2, §2.5 항목 1 |
| 2 | RacePro R2 ③(`mq_open`)이 불명 → 분석. Scribe 커널 코드 읽기로 fd table(FILES_STRUCT) 쓰기만 잡히고 큐 이름·큐 객체에는 Scribe 자원 잠금이 없다고 정리. C3-b도 같은 이유로 불명 → 분석(조건부) | RacePro.md §2.5 K1–K4, §3.5 | §0.2 항목 7, §2.4.1, §6.1, §6.2, §6.4, §6.7 |
| 3 | R2 사건표에 ⓪ core task 생성 행. RacePro.md에 후보 클래스별 대조표. DESCRY.md·SysPro.md에 R2 사건표 신설, RRF.md·SimRacer.md에 ⓪ 행 | RacePro.md, RRF.md, SimRacer.md, DESCRY.md, SysPro.md 각 §3.5 | §6.2, §6.8 |
| 4 | 가설 H-RP-5(보고 조건 "녹화 때 없던 실패"와 R2). 결함이 아니라 목적 차이이며 §10.5 표에 같은 행이 없음 | RacePro.md §5.2 | §2.4.1 R2 Q6, §4.2 행 6, §4.4 행 6, §6.2 |
| 5 | SimRacer 계보에 Sim-O/C(ITJ 2013)·SimTester(VEE'12) `2차` 추가. H-SM-1 보정. SimRacer의 기존 기능 표에서 SR 지정 인터페이스를 SIMEXPLORER 서술로 옮김 | SimRacer.md §1.2, §2.3, §5.1–5.2; SimEvo.md §2.3 | §0.2 항목 2·4, §1.1, §2.2, §2.5 항목 7, §4.2, §4.3 |
| 6 | SIMEXPLORER·RRF 논문의 범위 서술(단일 process 안 multithreaded 응용을 thread-level 기법의 영역으로 둠, p.1 등)을 `INAPPLICABLE` 판단 자료로 추가. 범위에서 뺀다는 명시 문장은 없음 | SIMEXPLORER.md §1.3 항목 11, RRF.md §1.3 항목 8 | §0.3, §2.4.3, §2.4.4, §3.3 |
| 7 | DESCRY의 logging point 식별(p.5 customized logging facilities)과 v7.0.1 EVS 출력 경로 `[I 소스]`. R2 거부 분기가 패킷 생성(L204) 앞이라는 소스 읽기 | DESCRY.md §1.3 행 16, §3.5 | §2.2, §2.4.5 |
| 8 | SimRacer.md·DESCRY.md의 [I] 도출: S1 아래에서 R2 뒤집기 schedule은 교착·진행 막힘에 이를 수 있음(실행 결과 아님) | SimRacer.md §3.5 R2 Q5, DESCRY.md §3.5 R2 Q5 | §2.4.2, §2.4.5, §4.2, §4.4 |
| 9 | `[M-probe 기록]`에 (4) `OS_TaskCreate` → `clone3`를 더함. 도구별 파일끼리 근거 표기가 어긋나 저장소 기록을 다시 읽어 정함 | DESCRY.md(증거 태그 절) 대 SysPro.md §1.3 항목 17 | §0.1, §2.5 항목 9, §6.5 C5-a, §5.5 |
| 10 | RRF.md의 "no syscall was made" 인용을 근거로 쓰지 않는다고 표시 | RRF.md §1.3 항목 9, §3.5 | §0.1, §2.4.4, §2.5 항목 10, §5.5 |
| 11 | 접근 시도 결과: 새 전문 없음. 둘째 취득 경로 없음. [36] 초록, Zaman 학위논문 초록 전체, 피인용 문맥, JSS 서지·PAR 엠바고, ReDPro Internet Archive 스냅숏 존재(열리지 않음), SysPro `add_repo` 거절, GitHub MCP의 저장소 추가 안내 | 각 파일 §1.1 | §1.1, §1.2, §5.1 A5·A6·A10, §5.2 |
| 12 | 환경 재측정: 커널 `6.18.44-fc-v139`, `/` 여유 1.3–2.7 GB. 블로커 분류는 그대로 | 각 파일 §4.1 | §1.2, §3.2, §5.4 항목 6 |
| 13 | CPS-VO 사본의 라이선스 표기를 두 파일이 서로 다른 페이지에서 봄 | SIMEXPLORER.md §1.3 항목 10, SimEvo.md §1.3 | §1.1, §5.4 항목 16 |
| 14 | PI 판단 항목 17(공개 웹 보관본) 추가 | ReDPro.md D-RD-1 (e), SimRacer.md §6 항목 16 | §5.4 |
| 15 | 바뀌지 않은 것: 40칸 모두 `NOT_RUN`. 확인된 부족 0건. §10.5의 여섯 행 모두 비교 결과로 주장하지 않음. 행 5("관측된 실행만 다룸")는 여덟 도구에 근거가 없거나 반대 | 여덟 파일 §4.2, §5 | §0.2 항목 1·5·6, §3, §4 |

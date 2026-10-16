# RacePro (SOSP 2011) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 08:12–08:35, 경로 시도와 읽기 시각).
- 보완: 2026-10-16 (UTC 00:00–00:25). `plan/process_race_method_matrix.md`가 남긴 후속 항목(A3, R2 사건표의 task 생성 행, 후보 클래스별 대조)을 처리했다. Scribe 커널과 py-scribe의 코드를 더 읽어 §6 항목 1·4를 코드 수준에서 정리했고(§2.5), R2 사건표에 task 생성 행과 클래스별 대조표를 넣었다(§3.5). 논문의 보고 조건을 R2에 적용한 분석과 가설 H-RP-5를 추가했다. 환경 값은 오늘 다시 측정했다(§4.1). 이번에도 RacePro나 Scribe를 실행하지 않았다.
- 대상: Laadan, Viennot, Tsai, Blinn, Yang, Nieh, *Pervasive Detection of Process Races in Deployed Systems*, SOSP 2011, pp. 353–367. DOI 10.1145/2043556.2043589.
- 범위: 로드맵 §10 (M6)의 RacePro 항목. 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다. 로드맵 §12 작업 6(`process_race_method_matrix.md`)의 입력으로 쓴다.
- 이 문서의 성격: **분석 문서**다. RacePro를 실행하지 않았다. 사례에 대한 "탐지 못 함" 결과를 담지 않는다.
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다.
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정으로만 인용한다. 사실로 쓰지 않는다. 이 문서가 쓰는 결정은 P-PAPERS와 T1이다.

## 증거 태그

| 태그 | 뜻 | 이 문서에서의 쓰임 |
|---|---|---|
| [F] | 인용이 있는 사실 | 원출처(논문 쪽·절, 코드 파일·줄) 또는 저장소 기록(문서·절). 저장소 기록은 "기록"이라고 표시하며 이번에 재검증하지 않았다 |
| [M] | 측정 | 오늘(2026-10-15) 이 컨테이너에서 읽기 전용으로 확인한 값, 또는 "기록"이 붙은 이전 측정 |
| [I] | 추론·분석 | 설계 기술에서 도출한 해석. 도구를 실행하지 않았다 |
| [D] | 결정 | PI 결정 |
| [U] | 미확인 | 자료가 없거나 읽지 못함 |

쪽 번호(p.)는 SOSP'11 인쇄 쪽(353–367)이다. PDF 쪽 번호는 여기서 352를 뺀 값(1–15)이다.

---

## 0. 한눈에 보기

1. **논문 전문을 읽었다.** 저자 호스팅 PDF가 오늘 열린다. 이전 기록(`baselines/racepro.md` D1, "논문 미열람")은 해소되었다. 공개 코드 저장소 5개도 git으로 열리며, 고정 커밋이 2026-10-08 기록과 같다. [F]
2. **로드맵 §10.3의 [원출처 사실]이 전문으로 확인된다.** 공유 메모리 접근 순서는 기록·재생용이다(§4, p.357). 후보 분석은 시스템 호출이 공유 커널 객체에 하는 load/store와 happens-before(HB)가 중심이다(§5, pp.357–361). [F]
3. **RacePro는 순서위반을 겨냥한다.** 연구한 process race 109건 중 79건이 execution order violation이다(p.355). 논문의 선행 사례 launchpad-596064는 "생성 후 사용" 구조다(upstart가 smbd의 디렉터리 생성을 기다리지 않고 nmbd를 시작, Table 4). "기존 기법은 순서위반을 못 다룬다"는 문장은 쓸 수 없다. [F]
4. **다섯 사례의 의존성 쌍은 후보 입력 사건으로 이어지는 경로가 확인되지 않는다 (분석, 미실행).** 사례의 의존성은 사용자 공간의 메모리 상태이고, 논문은 HB 그래프가 "dependencies between processes via shared memory"를 담지 않는다고 스스로 밝힌다(p.358). 이것은 "탐지 실패"가 아니다. 다른 후보의 go-live 뒤에 실패가 자연스럽게 나타나면 checker가 보고할 수 있고, 논문도 이때 원인을 짚지 못한다고 밝힌다(p.363). [I]
5. **논문 §6.2가 직접 관련된 한계를 적는다.** 같은 process 안의 thread 사이 OS 자원 race에는 "less effective"라고 한다(p.362). native cFS는 한 process 안의 OSAL task(pthread)다. 이 문장을 INAPPLICABLE 근거로 볼지는 PI 판단이다(§4.3). [F]
6. **실행은 오늘도 불가능하다.** 문서화된 환경이 i386 Linux 2.6.35 + Scribe 내장 커널이고, 이 컨테이너(Firecracker guest, 2026-10-16 측정 `6.18.44-fc-v139`)는 kexec·모듈·KVM이 모두 없다. 결과 상태는 다섯 사례 모두 `NOT_RUN`이다. [M]
7. **Kairos 가설을 근거가 있는 만큼만 세운다.** 근거가 지지하는 것은 "cFS 내부 사건이 후보 생성 입력에 없음"(H-RP-1)과 "고정된 순서의 이유를 도구 안에서 설명하지 않음"(H-RP-4)이다. 로드맵 §10.5의 "관측된 실행만 다룸" 행은 RacePro에 근거가 없다. 예측·탐색·실행 확인은 RacePro에 이미 있다(§5.1).
8. **보고 조건 (2026-10-16 추가).** 논문은 checker가 "a failure that did not occur during recording"을 볼 때 버그로 보고한다(§6.3 p.363). R2 현상은 저장소 기록상 gdb 3회 모두 나타났다. 녹화한 실행에 현상이 이미 있다면 그것은 이 보고 조건이 겨냥하는 "녹화 때 없던 실패"가 아니라는 분석이 나온다. RacePro 녹화에서도 같은 순서가 나오는지는 실행하지 않아 미확인이다(§3.5 R2 Q6, H-RP-5). [I]
9. **POSIX mqueue (코드 읽기, 2026-10-16).** Scribe 커널의 `ipc/mqueue.c`에는 Scribe 호출이 없다. `mq_open`의 fd 할당과 설치는 공통 경로(`alloc_fd`, `fd_install`)에서 FILES_STRUCT 쓰기로 잡힌다. 큐 이름 조회와 `mq_timedsend`/`mq_timedreceive`의 큐 객체 접근에는 Scribe 자원 잠금 경로가 보이지 않는다(§2.5). 실행으로 확인하지 않았다. [F 코드][I]

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 근거 등급

| 등급 | 뜻 |
|---|---|
| 전문 읽음 | 본문 전체를 텍스트로 추출해 읽었다 |
| 초록만 | 초록·서지만 읽었다 |
| 2차 설명 | 저자 발표자료, 검색 요약, 이전 저장소 기록 |
| 코드 직접 읽음 | 고정 커밋의 파일을 이번에 열어 줄 번호를 확인했다 |
| 기록 재사용 | 저장소의 이전 보고서에 있는 값. 이번에 재검증하지 않았다 |
| 미도달 | 열지 못했다 |

### 1.2 시도한 경로와 결과

시각은 2026-10-15 UTC 08:12–08:18이다. 같은 호스트를 반복 재시도하지 않았다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | `https://www.cs.columbia.edu/~orenl/papers/sosp2011-racepro.pdf` | HTTP 200, PDF 1,740,862 B, 15쪽. sha256 `645a21ba…fd64e` | **전문 읽음.** 이 문서의 쪽 인용은 이 판본 기준 |
| 2 | `https://www.sigops.org/s/conferences/sosp/2011/current/2011-Cascais/printable/25-laadan.pdf` | 200, 위와 같은 sha256 | 같은 파일 |
| 3 | `https://www.cs.columbia.edu/~nieh/pubs/sosp2011_racepro.pdf` | 200, 위와 같은 sha256 | 같은 파일 |
| 4 | `https://www.cs.columbia.edu/~junfeng/papers/racepro-sosp11.pdf` | 200, 855,588 B, sha256 `caf650ed…8794` | 같은 논문의 다른 빌드(ACM 저작권 블록·쪽 번호 없음). 줄 단위 비교는 하지 않았다. 인용에 쓰지 않는다 |
| 5 | `https://systems.cs.columbia.edu/projects/racepro/` (그룹 페이지, WebSearch 결과) | 200 | **초록만.** 초록, 연구 자료 `races.xlsx`/`races.txt`, 논문 두 편의 서지만 있다. 코드 링크는 없다 |
| 6 | `https://www.cs.columbia.edu/~junfeng/papers/racepro/` | 200 | 서지·초록. 자료 링크에 `github.com/columbia/racepro`, `races.xlsx`, HotOS'11 PDF가 있다 |
| 7 | `https://www.usenix.org/legacy/event/hotos11/tech/final_files/Laadan.pdf` (HotOS'11 예비 설계) | 200, 5쪽, sha256 `e25380f9…8815f` | **전문 읽음.** 초기 설계라 SOSP 본문과 다르면 SOSP를 따른다 |
| 8 | `https://www.sigops.org/s/conferences/sosp/2011/current/2011-Cascais/25-laadan-slides.pdf` | 200, 65쪽 | **2차 설명.** 저자 발표자료의 텍스트를 읽었다 |
| 9 | `https://systems.cs.columbia.edu/projects/racepro/races.txt` | 200, 64 KB | 첫 부분만 열람. 이 문서에서 쓰지 않는다 |
| 10 | `https://api.openalex.org/works/doi:10.1145%2F2043556.2043589` | 200 | 서지만(쪽 353–367, DOI 확인). 전문 위치 없음(`oa_status: closed`) |
| 11 | `https://reproducibility.cs.arizona.edu/v2/data/sosp11_LaadanVTBYN11_bib.html` | 200 | 서지만 |
| 12 | `https://dl.acm.org/doi/10.1145/2043556.2043589`, `https://doi.org/10.1145/2043556.2043589`(→ACM), `https://dl.acm.org/doi/pdf/10.1145/2043556.2043589` | 모두 HTTP 403 | **미도달** |
| 13 | `http://rcs.cs.columbia.edu/projects/racepro/` (논문 참고문헌 [1]의 주소) | 404. https는 연결 실패 | **미도달** |
| 14 | `https://www.cs.columbia.edu/~orenl/papers/` (디렉터리) | 403 | **미도달.** `~orenl/` 홈은 200이지만 RacePro 언급이 없었다 |
| 15 | `https://github.com/columbia/racepro` (웹 UI) | 403. 세션의 GitHub 프록시가 거부했다 | 웹 UI **미도달.** git 전송과 raw 주소는 아래대로 열린다 |
| 16 | `git ls-remote` — `columbia/{racepro, racepro-bundle, linux-2.6-racepro, libscribe-racepro, py-scribe-racepro}` | 5개 모두 응답. HEAD가 2026-10-08 기록의 커밋과 같다 | 코드 접근 가능 |
| 17 | `git clone --depth 1 columbia/racepro` (@`681f94e3`) | 성공 | **코드 직접 읽음** (§1.3) |
| 18 | `raw.githubusercontent.com` — 커널 `README.md`, `Documentation/scribe.md`, `include/linux/scribe_resource.h`, 번들의 `config/kernel_config`, `config/deploy.rb`, `racepro/setup.py` | 모두 200 | 코드 직접 읽음 |
| 19 | WebSearch (standard) | 위 호스트들의 목록. 새로운 1차 출처는 없다 | — |
| 20 | Scholar Gateway 전문 검색 | 이번에는 시도하지 않았다. 2026-10-08 기록에서는 ACCESS_DENIED | 미시도 |
| 21 | 경로 1 재확인 (2026-10-16T00:05Z) | HTTP 200, 1,740,862 B | 같은 판본. 인용 문장 10개를 추출 텍스트에서 다시 찾았다 |
| 22 | `git clone --depth 1 --filter=blob:none` + sparse checkout — `columbia/linux-2.6-racepro` (HEAD `b29e9afb`). 받은 경로: `ipc/`, `scribe/`, `include/linux/scribe*.h`, `include/linux/file.h`, `fs/{file.c, open.c, namei.c, file_table.c, read_write.c}`, `kernel/fork.c` | 성공 (2026-10-16) | **코드 직접 읽음** (§2.5). 트리 전체는 받지 않았다 |
| 23 | `git clone --depth 1 columbia/py-scribe-racepro` (HEAD `56d6187e`) | 성공 (2026-10-16) | **코드 직접 읽음** (§2.5) |

### 1.3 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 이 문서에서의 쓰임 |
|---|---|---|
| SOSP'11 본문(경로 1–3) | **전문 읽음** | 도구의 설계·모델·한계·평가 서술 전부. §2, §3 |
| HotOS'11(경로 7) | 전문 읽음(초기 설계) | 참고만. SOSP와 다르면 SOSP를 따른다. 예: 가지가 1000개를 넘으면 무작위 탐색으로 바꾼다는 서술은 HotOS에만 있다 |
| 저자 슬라이드(경로 8) | 2차 설명 | 로그에 `futex` 자원 항목이 실제로 찍힌다는 예시, 자원표 |
| 그룹·저자 페이지(경로 5, 6) | 초록만 | 코드 위치 확인 |
| `racepro` @`681f94e3` | **코드 직접 읽음** | `racecore.py`, `execgraph.py`, `racetest.py`, `scribewrap.py`, `unistd.py`, `setup.py`, `TODO`, `tests/tests.list`. 줄 번호는 이 커밋 기준 |
| 커널 README·`scribe.md`·`scribe_resource.h`, 번들 `kernel_config`·`deploy.rb` | 코드 직접 읽음 | 요구 조건(§4) |
| 커널 @`b29e9afb`의 `ipc/{mqueue,msg,sem,shm}.c`, `fs/{file,open,file_table,read_write}.c`, `include/linux/{file,scribe_events,scribe_api}.h`, `scribe/resource/lock_wrappers.c`, `scribe/pump.c`, `kernel/fork.c`; `racepro`의 `syscalls.py`, `toctou.py`, `session.py`; py-scribe @`56d6187e`의 `src/record`, `src/scribe/event_iterators.pxi` | 코드 직접 읽음 (2026-10-16) | §2.5, §3.5의 R2·C3 표 |
| 커널 diff 목록 130개 파일 | 기록 재사용 | 저장소 밖 작업 디렉터리의 `/home/user/work/procrace/baselines/racepro/scribe_changed_files.txt`(2026-10-08 저장)를 오늘 다시 읽어 `ipc/mqueue.c` 부재와 `arch/x86`의 `*_32` 편중을 확인했다. `git diff`는 다시 돌리지 않았다 |
| `baselines/racepro.md`의 그 밖의 코드 서술 | 기록 재사용 | 예: `lock_wrappers.c`, `memory.c`, Python 모듈의 이벤트 참조. 이번에 재검증하지 않았다 |
| 사례 쪽 사실 | 기록 재사용 | `cases/ledger.md`, `baselines/availability.md` §3–§4, 로드맵 §2.2, §9 |

### 1.4 논문 전문을 읽고 이전 기록과 달라지거나 확정된 것

| # | 항목 | 내용 |
|---|---|---|
| 1 | `racepro.md` D1 | 논문을 열 수 없던 문제가 해소되었다. 2026-10-08에는 호스트 접근이 막혔고 오늘은 열린다(P-PAPERS [D]) |
| 2 | 로드맵 §10.3 | 공유 메모리 기록(§4 p.357)과 syscall 중심 후보 분석(§5)의 구분이 논문 본문으로 확인된다 |
| 3 | 코드 기반 모델 | 논문과 일치하는 것: HB 간선 종류(p.358), micro-op 모델(pp.358–359), replay + go-live(pp.361–362), checker(p.363) |
| 4 | 논문과 코드의 차이 | 아래 §2.4 |
| 5 | go-live 뒤 무관한 실패 | `racepro.md` §6의 caveat(코드 기반 추정)가 논문 §6.3 p.363의 문장으로 뒷받침된다 |
| 6 | 소프트웨어·하드웨어 값 | `racepro.md` R7, R8, R10, R11, R14는 `unspecified_by_reference`였지만 논문 §7에 값이 있다(§4.1). 이것이 정책상 "레퍼런스"인지는 PI 판단(§4.3) |
| 7 | DOI | 로드맵의 DOI `…2043589`가 맞다. DBLP 서지, 저자 페이지, OpenAlex가 일치한다. `racepro.md`가 시도한 `…590`은 잘못된 주소였다 |
| 8 | `SPEC_FINAL.md`의 RacePro 등급 | SPEC은 RacePro를 `[V-abs]`(초록 확인)로 표기한다(줄 662, 줄 3349). 전문 열람 수준으로 올릴 수 있다. 이 작업 범위 밖이라 수정하지 않았다 |
| 9 | 환경 수치 | 2026-10-15 커널 `6.18.44-fc-v114`(2026-10-16 재측정 `fc-v139`), `/` 여유 16 GB(2026-10-15), 2.7 GB(2026-10-16). 2026-10-08 기록은 `fc-v80`, 여유 2.x GB였다. 블로커 자체는 그대로다(§4.1) |

---

## 2. 도구의 정체

### 2.1 요약

| 항목 | 내용 | 근거 |
|---|---|---|
| 역할 | **탐지(detection).** 논문은 "the first system designed to detect" process races라고 한다. 후보를 재실행으로 검증해 해로운 것만 보고하므로 재현 단계를 포함하지만, 사용자가 가진 실패 보고를 재현하는 도구(DESCRY, RRF 유형)가 아니고 회귀 분석 도구도 아니다 | [F] 초록 p.353, §6 p.361 |
| 실행 단위 | process와 thread. recorder가 "multiple processes and threads"를 시스템 전체로 기록한다. process race의 정의는 "multiple processes access shared operating system resources"다(초록). 같은 process의 thread 사이는 "less effective" | [F] pp.353–354, p.356, p.362 |
| 관측하는 것 | system call, signal(sync point에서 전달), Table 2의 8종 커널 객체에 대한 접근 순서, 공유 메모리 page ownership 전이(재생용). 응용 수정 불필요 | [F] §4 pp.356–357 |
| 자원 식별 | 커널이 객체 단위로 추적한다: inode, file, file-table, mmap, cred, global, pid, ppid. 경로 이름은 부모 inode 안의 "경로별 고유 위치"로 모델한다. 스트림은 가상 read/write offset, 신호는 신호별 추상 위치를 둔다. pid 같은 식별자는 replay에서 같은 값이 나오도록 가상화한다 | [F] Table 2 p.357; §5.2 pp.358–359; §6.2 p.362 |
| 순서·의존 모델 | HB 그래프의 간선은 같은 process 안의 순서와 4종이다. ① fork→자식의 fork 반환 ② 자식 exit→부모 wait ③ kill→신호 전달 ④ stream(pipe/socket) write→겹치는 read. 이것을 "inherent happens-before"라 부르며 syscall 의미에서 추론할 수 있는 것만 넣는다. 벡터 시계로 동시성을 판정한다 | [F] §5.1 pp.357–358 |
| 후보 종류 | load-store(두 syscall이 같은 커널 객체에 동시 접근, 하나 이상이 store), wait-wakeups(wait 하나를 둘 이상의 wakeup이 깨울 수 있음), wakeup-waits. 3개 초과 syscall에 걸친 race는 기본으로 검출하지 않는다 | [F] §5.3 pp.359–361 |
| 입력 | 배포 시스템의 실행 recording. 응용 코드·수정 불필요. checker(built-in 또는 사용자 프로그램·스크립트). replay용 파일 시스템 체크포인트(unionfs). 코드 쪽 실제 입력은 프로그램별 `<test>.test` 스크립트이며 사용자가 쓴다 | [F] §6.3 p.363; p.362. 코드 `racetest.py:151-152` |
| 출력 | 후보마다 replay 결과를 Diverged / Benign / Harmful로 가른다. checker가 "녹화 때 없던 실패"를 볼 때만 버그로 보고하고, 원 녹화에 go-live 녹화를 이어 붙인 recording을 저장한다 | [F] p.354, §6.3 p.363, Table 5 p.364 |

### 2.2 동작 흐름

[F] 논문 §3 Figure 5–6(p.356), §5–§6.

1. **record.** 배포 시스템을 실행하면서 Scribe(커널 recorder)가 기록한다. 검사는 하지 않는다.
2. **detect.** explorer(사용자 공간, Python)가 recording에서 syscall별 커널 객체 접근을 읽어 HB 그래프를 만들고, 동시이면서 같은 객체에 접근하는 pair/triple을 후보로 뽑는다. 후보를 HB 그래프상 거리로 순위를 매기고, 가치가 낮은 자원(예: 화면 출력)은 미루거나 건너뛸 수 있다(p.360).
3. **validate.** 후보마다 앵커 syscall의 순서를 바꾼 execution branch(수정한 log)를 만든다. consistent cut으로 자르고, replay한 뒤 go-live로 실제 실행을 이어 가며 checker가 실패를 본다.
4. **report.** 실패가 있을 때만 버그로 보고한다. replay가 diverge하면 증거가 없다고 보고 버리며, 논문은 이것을 거짓 양성 제거로 활용한다(p.362).

### 2.3 논문이 직접 밝힌 한계 중 cFS와 관련 있는 것

| 한계 | 인용 | 쪽 |
|---|---|---|
| HB가 공유 메모리·파일 내용에 의한 의존을 담지 않는다 | "the graph does not capture dependencies between processes via shared memory" | p.358 |
| 그 결과 불가능한 race를 후보로 낼 수 있고, 검증 단계가 걸러 낸다 | "such cases will be filtered later by RacePro's validation step" | p.358 |
| 같은 process의 thread 사이 OS 자원 race | "less effective in finding races on OS resources between threads of the same process" | p.362 |
| 공유 메모리 접근 순서가 뒤집히면 replay가 diverge한다. relaxed 모드로 완화한다 | "will certainly lead to replay divergence" | p.362 |
| go-live는 외부 입력이 없는 실행에서 가장 잘 된다 | "works best when a go-live execution requests no inputs" | p.362 |
| 3개 초과 syscall에 걸친 race는 기본으로 검출하지 않는다 | "will not detect this load-store race" | p.361 |
| 재귀 적용(model checker 모드)은 기본으로 꺼 둔다 | "we leave this mode off by default" | p.362 |
| 볼 수 있는 것은 checker가 보는 것뿐이다 | "can detect only what is checked" | p.363 |
| go-live 뒤 무관한 버그를 만날 수 있다. 원인을 짚지 못한다 | "may encounter an unrelated bug" … "without pointing out the root-cause" | p.363 |
| 로그 오류·segfault·timeout·비정상 종료 코드가 built-in checker다 | — | p.363 |

### 2.4 논문과 공개 코드의 차이 (확인된 것)

| 항목 | 논문 | 코드 (`racepro` @`681f94e3`, 커널 @`b29e9afb`) |
|---|---|---|
| 추적 객체 | Table 2: inode, file, file-table, mmap, cred, global, pid, ppid | 커널 `scribe_resource.h:221-229`: INODE, FILE, FILES_STRUCT, PID, FUTEX, IPC, MMAP, PPID, SUNADDR. `cred`, `global`은 이 열거에 없고 FUTEX, IPC, SUNADDR는 논문 표에 없다 |
| race 종류 | load-store, wait-wakeups, wakeup-waits | `racecore.py` 클래스 RESOURCE, EXIT-WAIT, SIGNAL, TOCTOU (`find_show_races`, 692행 이후). 논문의 일반 wait-wakeups(예: 스트림 read)와 wakeup-waits(connect/accept)에 해당하는 별도 클래스는 이 목록에 없다. 함수 단위로는 확인하지 않았다 |
| TOCTOU | "leave for future work" (p.365) | `toctou.py`가 있다. `TODO` 5행은 `racetest -kT toctou`가 후보를 만들지도 찾지도 못한다고 적는다 |
| futex | 본문에 언급 없음 | 저자 슬라이드의 로그 예시에 `write futex`가 찍힌다. 코드는 FUTEX를 RESOURCE 경합 대상에서 제외한다(`racecore.py:194, 282`) |
| 공유 메모리 | 기록은 하되 HB·후보에는 안 쓴다(p.358) | `racepro/*.py`와 `scripts/*`에 메모리 이벤트(`EventMem` 등) 참조가 없다(grep 0건) |
| HB 간선 | 4종 + 프로그램 순서 | `execgraph.py:87-137, 139-180, 182-186` 일치. `clone`을 fork로 취급(`unistd.py:373`) |

[I] 코드는 논문 직후(2011-08)의 스냅샷이다. 위 차이가 프로토타입 판본 차이인지 구현 선택인지는 자료로 결정되지 않는다.

### 2.5 코드 추가 확인 (2026-10-16, 읽기만, 실행 없음)

줄 번호는 커널 @`b29e9afb`, `racepro` @`681f94e3`, py-scribe @`56d6187e` 기준이다.

| # | 질문 | 확인한 것 | 근거 (파일:줄) | 등급 |
|---|---|---|---|---|
| K1 | POSIX mqueue의 큐 객체를 Scribe가 자원으로 잠그는가 | `ipc/mqueue.c`에는 `scribe` 문자열이 0건이다. 같은 디렉터리의 System V IPC는 `scribe_lock_ipc`를 부른다 | `ipc/msg.c:488`, `ipc/sem.c:1099`, `ipc/shm.c:655`; `ipc/mqueue.c` 검색 0건 | 코드 직접 읽음 |
| K2 | `mq_open`이 Scribe 자원 접근을 만드는가 | `mq_open`은 `get_unused_fd_flags`(= `alloc_fd`)로 fd를 받고 `fd_install`로 넣는다. 두 함수 모두 `scribe_lock_files_write(files)`를 부르므로 FILES_STRUCT 쓰기가 된다. 큐 이름 조회는 mqueue 파일 시스템 루트 inode의 `i_mutex`만 잡고 Scribe 잠금은 없다 | `ipc/mqueue.c:673, 691, 695–696, 730`; `include/linux/file.h:44`; `fs/file.c:464–467, 509`; `fs/open.c:937–948` | 코드 직접 읽음 |
| K3 | `mq_timedsend`/`mq_timedreceive`가 큐 file 객체를 잠그는가 | 둘 다 `fget(mqdes)`를 부른다. `fget`의 Scribe 잠금은 같은 syscall에서 먼저 `scribe_track_next_file*`가 `lock_next_file`을 세웠을 때만 걸린다. 그 호출은 `fs/read_write.c`, `fs/open.c`에서 찾았고 `ipc/mqueue.c`에는 없다 | `ipc/mqueue.c:866, 956`; `fs/file_table.c:288–306`; `scribe/resource/lock_wrappers.c:220–233, 274–292`; `fs/read_write.c:175` 외 | 코드 직접 읽음. 받은 디렉터리(행 22)만 검색했다 |
| K4 | explorer가 FILES_STRUCT를 후보 자원으로 쓰는가 | RESOURCE 탐색은 FUTEX만 종류로 제외하고, 쓰기 접근이 있는 나머지 자원에서 HB상 동시인 쌍을 찾는다. 경로가 다른 접근을 걸러 내는 규칙(`skip_parent_dir_race`)은 INODE·FILES_STRUCT에 적용되지만, 경로를 아는 syscall 목록(`get_resource_path`)에 `mq_open`이 없다 | `racecore.py:194, 196–213, 276–303`; `syscalls.py:434–459` | 코드 직접 읽음 |
| K5 | 공유 메모리 기록을 사용자 공간 도구가 읽을 수 있는가 | 커널은 `mem_owned_read/write`(serial)와 주소가 붙은 `_extra` 판 사건을 정의한다. py-scribe는 이 사건을 파싱하고, `record` 스크립트에 메모리 추적을 끄는 `--no-mm`과 `SCRIBE_MEM_EXTRA` 플래그가 있다. `racepro` Python 코드에는 이 사건 참조가 없다 | `include/linux/scribe_events.h:160–177`; `include/linux/scribe_api.h:64`; py-scribe `src/scribe/event_iterators.pxi:200–211`, `src/record:59–61, 101` | 코드 직접 읽음 |
| K6 | thread가 explorer에서 별도 Process가 되는가 | Scribe 상태(`scribe_ps`)는 task마다 붙는다. 로그는 큐의 `pid`가 바뀔 때 PID 사건을 넣고, explorer는 그 값으로 Process를 나눈다. 그 값이 thread id인지는 실행해야 확인된다 | `kernel/fork.c:987–1027`; `scribe/pump.c:125–131`; `session.py:305–308` | 코드 직접 읽음 + 미확인 |
| K7 | TOCTOU 클래스가 기본 탐색에 들어가는가 | `find_show_races`는 RESOURCE, EXIT-WAIT, SIGNAL 세 단계만 돈다. TOCTOU는 별도 함수 `find_show_toctou`이고, 패턴은 stat·access·open·chmod·chown·link 계열 파일 syscall이다 | `racecore.py:692–727, 773`; `toctou.py:245–314` | 코드 직접 읽음 |

[I] K2–K4에서 나오는 분석이다. 같은 process의 thread가 fd table(files_struct)을 공유한다면(pthread의 일반 동작이며 이번 출처로 확인하지 않았다), 서로 다른 OSAL task의 `mq_open`은 같은 FILES_STRUCT 자원에 대한 쓰기 접근이 된다. 두 접근 사이에 HB가 없으면 RESOURCE 후보 쌍이 될 수 있다. 그 쌍이 뜻하는 것은 "fd table 접근 순서"이고, 큐 이름이나 cFS의 AppId 상태가 아니다. 실제 로그에 이 사건이 찍히는지는 실행하지 않아 미확인이다.

---

## 3. 여섯 질문 × 다섯 사례

### 3.1 읽는 법

셀의 라벨은 아래 셋 중 하나다. 한 칸에 관측 가능성, 표현 가능성, 실제 탐지 성공을 합치지 않는다(로드맵 §10.2).

| 라벨 | 뜻 |
|---|---|
| **답** | 출처의 쪽·절로 답했다 |
| **분석** | analysis from stated design (not run). 논문·코드의 기술에서 도출했고 실행하지 않았다 |
| **불명** | not determinable from sources read |

"분석" 셀의 "경로가 확인되지 않는다"는 "읽은 설계 기술 안에서 그 쌍을 후보 입력 사건으로 만드는 경로를 찾지 못했다"는 뜻이다. 실행 결과가 아니다.

근거 칸의 §와 p.는 별도 표시가 없으면 논문(SOSP'11)의 절·쪽이다. 이 문서 자체의 절은 "이 문서 §"로 쓴다.

질문 번호는 로드맵 §10.2의 순서를 따른다.

1. 필요한 **관측 사건**을 수집하는가?
2. 서로 다른 이름·ID·필드가 같은 **자원·세대**를 가리키는 것을 식별하는가?
3. **생성·등록·초기화 완료와 의존 사용**의 관계를 표현하는가?
4. 필요한 **순서 또는 허용된 오류 처리** 정보를 입력받는가, 추론하는가?
5. 아직 실행되지 않은 순서를 **예측·탐색**하는가? 실제 실행에서 **확인**할 수 있는가?
6. **출력**은 후보·재현 성공·오류 결과 중 무엇이며 원 사례의 현상과 대응하는가?

### 3.2 사례 쪽에서 쓴 사실 (모두 저장소 기록, 재검증하지 않음)

| 사례 | 의존성과 현상 | 출처 |
|---|---|---|
| C1 CF#184 | provider(공개 BP 구성에서는 `OS_CountSemCreate`)가 이름 있는 count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`(`CF_CFDP_InitEngine`)이 성공한다. 실패하면 `CF_EID_ERR_INIT_SEM` 이벤트와 APP_ERROR 종료. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × 100 ms 재시도 | [F 기록] `cases/ledger.md` §1a, §1b |
| C2 cFE#198 | 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식·초기화 내용이 공개되지 않았다. 공개된 것은 단일 단계 시작 동기화(`WaitForStartupSync`)와 6.6.0a의 두 단계 방식(`APPS_INIT`/`LATE_INIT`, `CFE_ES_WaitForSystemState`) | [F 기록] `ledger.md` §1a–§1b; 로드맵 §9.1 |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`, EVS task의 `EVS_AppID`와 등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 호출하고, `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 원 플랫폼은 Microblaze. 오류 지점은 사례 기록에서 미확인 | [F 기록] `ledger.md` §1a, §2 |
| C5 cFE#72 | `CFE_ES_AppCreate`가 `OS_TaskCreate` 전에 ES lock을 풀고 뒤에 다시 잡아 `TaskTable` 항목을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다 | [F 기록] `ledger.md` §1a |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로(`CreatePipe`/`Subscribe`/이벤트 전송)를 실행하고, `EVS_GetAppDataByID`가 NULL을 돌려 `ILLEGAL_APP_ID`가 된다. 상태는 폐기된다. 서술은 "이벤트 요청 거부"다. gdb 3회 모두 설정 전 사용 24회 = 거부 24회(`CreatePipe` 2 + `SubscribeFull` 4 + `ReportSingleEvent` 18) | [F 기록] `ledger.md` §1a; 로드맵 §2.2 |
| 공통 | native cFS는 **한 Linux process**이고 OSAL task는 `pthread_create` thread다(osal `os-impl-tasks.c:568`). OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 **syscall 0건**이었다. 이름 조회는 사용자 공간 테이블 검색이며 semaphore는 `sem_init`한 이름 없는 process-private 객체다 | [F 기록] `racepro.md` §6; `availability.md` §4.2. [M 기록] 이번에 재측정하지 않았고, 이것은 OSAL 시험의 probe이지 RacePro 결과가 아니다 |

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | 답 | 근거 |
|---|---|---|---|
| 1 관측 사건 | 답 | recorder는 syscall, signal, Table 2의 커널 객체 접근 순서, 공유 메모리 page ownership 전이를 기록한다. 경합 분석(explorer)이 입력으로 쓰는 것은 syscall이 커널 객체에 하는 load/store micro-op과 4종 HB 간선이다. 응용의 함수 호출, 반환값, 메모리 값은 기록 대상으로 열거되지 않는다 | §4 pp.356–357; §5 pp.357–361 |
| 2 자원·세대 식별 | 답 | 커널 객체 단위 추적, 경로별 위치(부모 inode 안), 스트림 offset, 신호별 위치, pid 가상화. 객체의 세대(식별자 재사용)에 관한 서술은 본문에 없다. 본문에서 "generation"을 검색했을 때 0건이다 | Table 2 p.357; §5.2 pp.358–359; §6.2 p.362 |
| 3 생성·등록 vs 의존 사용 | 답 | 커널 객체에 대한 store(생성)와 load(조회·사용)의 동시 쌍으로 표현한다. 예: 경로 생성은 부모 inode의 경로 위치에 대한 store, 조회는 같은 위치의 load. 선행 사례 launchpad-596064(272개 후보 중 diverged 267, benign 3, harmful 2). 공유 메모리·파일 내용에 의한 의존은 HB에 넣지 않는다 | §5.2 p.358; §5.3.1 p.359; Table 4 p.363; Table 5 p.364 |
| 4 필요 순서·허용 오류 입력 | 답 | 필요 순서나 허용 오류를 기술하는 명세 입력은 논문에 없다. 순서는 syscall 의미에서 추론한 inherent HB로만 들어간다. 실패 판정은 checker가 한다(built-in 4종, fsck류 프로그램, 사용자 프로그램·스크립트, 선형화 실행과 출력 비교). 재시도·회복 처리에 관한 서술은 없다("retry"는 본문에 0건) | §5.1 p.357; §6.3 p.363 |
| 5 예측·탐색, 확인 | 답 | 설계상 있다. 기록된 실행에서 동시 pair/triple을 찾고 순서를 바꾼 branch를 만들어 replay 후 go-live로 실제 실행해 checker로 실패를 확인한다. 한계는 §2.3 | §5.3, §6.1–§6.3 pp.359–363 |
| 6 출력 | 답 | 후보(Detected)를 모두 검증해 Diverged / Benign / Harmful로 가른다. checker가 녹화 때 없던 실패를 볼 때만 버그로 보고하고 합친 recording을 저장한다. 출력 단위는 "race 후보(앵커 syscall 2~3개) + 검증 결과"다 | p.354; §6.3 p.363; Table 5 p.364 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C2** | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| **C3** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C5** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **R2** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |

### 3.5 사례별 셀 (근거 포함)

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 provider의 생성(이름 등록 완료)과 CF의 이름 조회다. OSAL 자체 시험에서 둘 다 syscall 0건이었다(probe). 경합 분석의 입력이 syscall의 커널 객체 접근이므로 두 사건은 그 입력에 나타나지 않는다. 해당 OSAL 테이블이 있는 메모리 페이지의 ownership 전이는 재생용으로 기록되겠지만(p.357), 경합 분석이 쓴다는 서술이 논문에 없고 코드에도 참조가 없다 | §5 pp.357–359; 코드 grep 0건; `availability.md` §4.2 |
| 2 | 분석 | 이름 `BP0_tsem` → OSAL ID → 사용자 공간 테이블 항목의 연결은 도구가 가진 식별 장치(경로 이름, 파일 핸들, pid)에 대응하지 않는다. 커널에는 대기·wake 때에만 futex 주소로 나타나며 이름과 이어지지 않는다(probe). 코드는 FUTEX 자원을 RESOURCE 후보에서 제외한다. 세대(재생성된 ID)는 논문에 서술이 없다 | §5.2 p.358; `racecore.py:194, 282`; `availability.md` §4.2(4) |
| 3 | 분석 | 도구가 표현하는 "생성 후 사용"은 커널 객체의 store → load 쌍이다. C1의 생성·조회는 커널 객체 접근이 아니라 micro-op 쌍이 만들어지지 않는다. 객체가 "등록 완료" 상태인지 구분하는 장치도 논문에 없다 | §5.2 p.358; Table 4 launchpad-596064 |
| 4 | 답 | 도구 수준 답과 같다. C1이 필요로 하는 허용 오류(재시도는 `OS_ERR_NAME_NOT_FOUND`만)를 넣을 인터페이스는 없다. 실패 기준(CF 초기화 abort)은 사용자가 checker로 써야 한다 | §6.3 p.363 |
| 5 | 분석 | 도구가 만드는 branch는 기록된 동시 syscall 앵커 쌍을 뒤집은 것이다. C1의 생성↔조회는 앵커가 아니므로 그 순서를 바꾼 branch는 만들어지지 않는다. 다른 후보의 branch가 go-live한 뒤 C1의 실패가 자연스럽게 나타나면 checker가 볼 수 있고, 이때 보고는 그 후보의 이름으로 나가 원인 쌍을 짚지 않는다 | §6.1 p.361; §6.3 p.363 |
| 6 | 분석 | 출력 유형은 "race 후보 + 검증 결과"다. 사례 현상(CF abort, 그리고 재시도 뒤 회복과 최종 실패의 구분)과 대응하려면 ① checker가 abort를 실패로 보고 ② 보고된 앵커 쌍이 생성↔조회여야 한다. ②는 Q1–Q3의 분석에서 경로가 없다. 회복과 최종 실패를 구분하는 출력 항목은 논문에 없다 | §6.3 p.363; Table 5 |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 P의 late init 완료와 R의 의존 호출이다. P·R이 비공개이고 통신 방식·초기화 내용이 알려지지 않아 어떤 OS 자원을 거치는지 판단할 근거가 없다. 공개된 시작 동기화 부분은 아래 별도 표를 본다 | `ledger.md` §1a; 로드맵 §9.1 |
| 2 | 불명 | 같은 이유 | — |
| 3 | 불명 | 같은 이유. "barrier 통과"와 "앱 고유 초기화 완료"를 구분하는 장치는 논문의 도구 모델에 없다(시작 단계나 준비 상태 개념이 없다. 본문은 Upstart 초기화 스크립트와 parallel boot를 사례로만 언급한다). 다만 의존이 거치는 OS 자원이 비공개라 이 칸은 "불명"으로 둔다 | 본문 키워드 검색 |
| 4 | 답 | 도구 수준 답과 같다. 실패 기준은 원 보고가 구체화하지 않아 연구 쪽 선택이 된다. 선택하더라도 원 보고자의 값으로 인용하지 않는다 | §6.3 p.363; [D] 결정 문서 §3.5 |
| 5 | 불명 | 앵커가 될 쌍을 알 수 없다 | — |
| 6 | 불명 | 원 현상이 구체 출력으로 공개되지 않았다 | `ledger.md` §1a |

C2의 **공개 부분** (역할 재구성에 쓸 수 있는 알려진 조건):

| 사건 | 커널에서 보이는 흔적 | RacePro 입력 | 라벨 |
|---|---|---|---|
| `CFE_ES_WaitForSystemState`가 `while (SystemState < MinSystemState)`를 돌며 `OS_TaskDelay` | 상태는 `CFE_ES_Global`의 메모리 필드, 대기는 sleep 계열 syscall | sleep은 Table 2의 객체를 건드리지 않아 micro-op이 만들어지지 않는다. 상태 필드는 메모리다 | 분석 |
| 단일 단계 / 두 단계 barrier의 상태 전이와 timeout | 같은 메모리 필드 | 위와 같다 | 분석 |

[F 기록] 코드 근거는 `racepro.md` §6의 `cfe_es_api.c:514, 585, 603` 서술이며 재검증하지 않았다. [I] sleep이 micro-op을 만들지 않는다는 해석은 Table 3의 syscall 템플릿과, recorder가 사용자 공간에서 보이는 커널 객체만 추적한다는 서술(§4 p.357)에서 도출했다.

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 `CFE_SB.AppId` 저장, `EVS_AppID`·등록 플래그 설정, TIME의 이벤트 요청(AppId=0)과 `EVS_IsFiltered`의 범위 검사 없는 접근이다. 모두 사용자 공간의 함수 호출과 전역 메모리 읽기·쓰기라 경합 분석 입력(커널 객체 접근)에 나타나지 않는다. TIME의 `CreatePipe`가 부르는 `mq_open`은 syscall로 보이지만 의존 상태 자체가 아니다 | §5 pp.357–359; `availability.md` §3 |
| 2 | 분석 | 의존 상태는 AppId 정수(ES가 부여)와 EVS 배열 항목이다. 커널 객체가 아니다 | Table 2 p.357 |
| 3 | 분석 | 공개(저장)와 사용(읽기)이 모두 메모리라 store/load 쌍으로 표현되지 않는다 | §5.1 p.358 |
| 4 | 답 | 도구 수준 답과 같다. 원 보고의 실패는 EVS 내부의 잘못된 인덱스 접근이고 오류 지점은 사례 기록에서 미확인이라, 어떤 checker가 필요한지는 지금 정할 수 없다 | §6.3 p.363; `ledger.md` §2 |
| 5 | 분석 | C1과 같다. 의존 쌍이 앵커가 아니므로 해당 순서를 뒤집은 branch가 만들어지지 않는다. go-live 뒤 자연 발생한 실패는 다른 후보의 이름으로 보고될 수 있다 | §6.1, §6.3 |
| 6 | 분석 | 사례 현상은 "잘못된 상태 사용"과 "실제 crash 여부"를 분리해 보고하는 것이다(로드맵 §9.2 완료 기준). built-in checker에 segfault가 있어 crash는 오류 결과로 볼 수 있다. 그러나 잘못된 상태 사용과 crash를 구분하는 출력은 없다. crash가 일어나는지 자체가 사례 기록상 미확인이다 | §6.3 p.363; `ledger.md` §2 |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | `OS_TaskCreate`의 `pthread_create`는 `clone`이므로 기록되고 HB 노드가 된다. `TaskTable` 등록 쓰기와 새 task의 `GetAppID` 조회는 메모리 접근이라 입력에 없다 | §5.1 p.358; `unistd.py:373` |
| 2 | 분석 | task ↔ AppId 연결은 ES 테이블 안에 있다. 커널 쪽 task 식별(pid 자원, 가상화)은 AppId와 연결되지 않는다 | §4 p.357 |
| 3 | 분석 | "clone → 자식의 첫 syscall" HB는 표현되지만 "등록 공개 완료"라는 사건은 없다. 도구 관점에서 등록 쓰기와 조회는 순서가 정해지지 않은 메모리 접근이고 micro-op이 아니다 | §5.1 p.358 |
| 4 | 답 | 도구 수준 답과 같다. 원 현상(등록 전 조회의 실패 반환)은 crash가 아니므로 내부 API 반환값을 보는 사용자 checker가 필요하다. 원 보고에 없는 앱 crash는 기대 결과로 넣지 않는다(로드맵 §9.3) | §6.3 p.363 |
| 5 | 분석 | `clone`은 HB 간선의 시작점이지만 순서를 뒤집을 앵커는 커널 객체에 대한 syscall 쌍에서 나온다. 등록 쓰기를 `clone` 앞뒤로 옮기는 branch는 만들 수 없다 | §5.3, §6.1 |
| 6 | 분석 | 사례 현상은 "등록 전 조회와 그 결과"다(로드맵 §9.3 완료 기준). built-in checker 4종(segfault, timeout, 시스템 로그 오류 메시지, 비정상 종료 코드)은 내부 API 반환값을 보지 않는다 | §6.3 p.363 |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 아래 사건 대응표의 네 사건(AppId 설정, 값 전달, 이벤트 요청, 오류 반환)이 모두 함수 호출과 메모리다. `CreatePipe`의 `mq_open`만 syscall이다 | 아래 표 |
| 2 | 분석 | SB AppId의 값(0 대 유효값)은 메모리의 정수다 | Table 2 p.357 |
| 3 | 분석 | 설정(저장)과 사용(읽기)이 메모리라 store/load 쌍으로 표현되지 않는다 | §5.1 p.358 |
| 4 | 답 | 도구 수준 답과 같다. R2의 요구 순서("SB AppId 저장이 SB API 경로의 이벤트 요청보다 먼저")를 넣을 입력은 없다 | §5.1 p.357; §6.3 p.363 |
| 5 | 분석 | R2의 순서는 core 시작 직렬화가 ES/EVS를 SB보다 먼저 실행하게 해서 고정된다(사례 기록). 이 직렬화는 ES가 각 core task가 RUNNING이 될 때까지 기다리는 폴링이다(`CFE_ES_CreateObjects`, `CFE_ES_MainTaskSyncDelay`, 사례 기록). HB 그래프는 이런 사용자 수준 직렬화를 담지 않으므로(p.358) 도구 안에서는 이 순서의 이유가 설명되지 않는다. AppId 설정·사용 쌍은 커널 객체의 syscall이 아니어서 그 순서를 뒤집은 branch는 만들어지지 않는다. ③의 `mq_open`이 만드는 FILES_STRUCT 쌍은 앵커가 될 수 있지만(§2.5 K2·K4, 분석), 그 branch가 뒤집는 것은 fd table 접근 순서다. 그 branch가 replay에서 diverge하는지, go-live 뒤 AppId 설정 순서가 달라지는지는 읽은 자료로 정할 수 없다 | §5.1 p.358; §6.1 p.361; `ledger.md` §1a; `plan/case_basis_register.md` R2.E5 |
| 6 | 분석 | 사례 현상은 "이벤트 요청 거부 24회"이고 프로세스 실패가 없다(상태 폐기). built-in checker는 내부 반환값을 보지 않는다. 출력으로 대응하려면 사용자 checker가 내부 상태를 볼 수 있어야 한다. R2는 실패 메시지도 출력하지 않는다. 또한 논문의 보고 조건은 checker가 "a failure that did not occur during recording"을 보는 것이다(p.363). R2 현상은 기록상 gdb 3회 모두에서 나타났고 사례 기록은 코드상 매 부팅 결정적이라고 예측한다(미실행 예측). RacePro 녹화에도 현상이 들어 있다면 branch에서 보이는 같은 현상은 "녹화 때 없던 실패"가 아니다. RacePro 녹화에서 같은 순서가 나오는지는 미확인이다 | `availability.md` §1 DESCRY 행; 로드맵 §2.2; §6.3 p.363; `ledger.md` §2 R2 "Established" |

**R2 사건 대응표** (로드맵 §10.3 첫 산출물). 참조 revision은 사례 기록의 B/M/D(`fc8c48ad`/`12cb84fc`/`f22e538c`)이며 v7.0.1 코드와 다를 수 있다.

| R2 사건 | 커널에서 보이는 흔적 | Scribe 기록 | micro-op | HB 간선 | 후보 규칙 | 라벨 |
|---|---|---|---|---|---|---|
| ⓪ core task 생성 (2026-10-16 추가): ES가 `CFE_ES_CreateObjects`에서 core task를 `OS_TaskCreate`로 하나씩 만들고, 각 task가 RUNNING이 될 때까지 `CFE_ES_MainTaskSyncDelay`로 기다린다(사례 기록) | `OS_TaskCreate` → `pthread_create` → `clone` 계열 syscall. C5 보충표와 같은 근거이며 cFS에서 측정하지 않았다. 기다림은 메모리 폴링과 지연이다 | `clone`은 syscall로 기록된다 | 코드가 `clone`을 fork로 취급한다(`unistd.py:373`) | ES task의 `clone` → 새 task의 첫 syscall. RUNNING 대기(폴링)는 공유 메모리를 통한 의존이라 HB 간선이 되지 않는다(p.358) | 후보 규칙의 대상이 아니라 HB 간선의 원천 | 분석 |
| ① SB AppId 설정: `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, 반환값 무시 (B `cfe_sb_task.c` L126, M L138, D L128) | syscall 없음. ES 조회는 메모리 + `pthread_getspecific`(소스 수준, 실행 미확인) | 메모리 page ownership 순서만(재생용) | 없음 | 없음 | 적용 규칙 없음 | 분석 |
| ② 값 전달: EVS(B) 또는 ES·EVS(M, D) task가 같은 전역을 읽음 | syscall 없음 | 위와 같다 | 없음 | 없음 | 없음 | 분석 |
| ③ 이벤트 요청: `CFE_SB_CreatePipe`/`Subscribe`/전송 → `CFE_EVS_SendEventWithAppID` | `CreatePipe`는 `mq_open("<pid>.<name>")`을 부른다(OSAL 시험에서 69회 측정, probe). `Subscribe`와 이벤트 전송 자체는 syscall이 아니다(소스 수준) | 코드 읽기(§2.5 K1–K3): `mq_open`의 fd 할당·설치가 FILES_STRUCT 쓰기를 만든다. 큐 이름 조회, 큐 객체 자체에는 Scribe 자원이 없다. 2026-10-15판의 "불명"을 코드 수준에서 바꿨다. 실행 확인은 없다 | FILES_STRUCT store (fd table). 이벤트 요청 자체에는 없음 | 같은 process의 thread 사이 `mq_open`끼리는 syscall 의미에서 나오는 간선이 없다 | RESOURCE: 다른 task의 fd 할당과 HB상 동시이면 쌍이 될 수 있다(§2.5 K4). 그 쌍은 fd table 접근이며 AppId 설정·사용이 아니다 | 분석 |
| ④ 오류 반환: `EVS_GetAppDataByID` NULL → `ILLEGAL_APP_ID`, 상태 폐기 (B `cfe_evs.c` L182-186) | 반환값, syscall 없음 | 없음 | 없음 | 없음 | 없음 | 분석 |

**R2 × 코드의 후보 클래스** (2026-10-16 추가). 코드의 `find_show_races`가 도는 세 클래스와 별도 함수인 TOCTOU를 사건 ⓪–④에 하나씩 대조했다. 모두 코드 읽기에서 나온 분석이며 실행하지 않았다.

| 클래스 | 코드상 적용 조건 | 근거 | ⓪ | ① | ② | ③ | ④ |
|---|---|---|---|---|---|---|---|
| RESOURCE | FUTEX 이외 자원에서 쓰기를 포함한 HB상 동시 접근 쌍 | `racecore.py:185–303` | 대상 아님(HB 간선) | 자원 접근 없음 | 자원 접근 없음 | FILES_STRUCT 쌍 가능(fd table) | 자원 접근 없음 |
| EXIT-WAIT | 같은 reaper가 거두는 두 exit와 wait | `racecore.py:396`(클래스), `525` 이후(`find_races`) | 해당 없음(생성) | exit·wait 아님 | 아님 | 아님 | 아님 |
| SIGNAL | 기록된 신호 전달 | `racecore.py:388–391` | 신호 아님 | 아님 | 아님 | 아님 | 아님 |
| TOCTOU | 기본 탐색 밖(`find_show_toctou`). stat·access·open·chmod·chown·link 계열 패턴. `TODO` 5행이 이 시험이 후보를 만들지 못한다고 적는다 | `racecore.py:625–672, 773`; `toctou.py:245–314` | 아님 | 아님 | 아님 | `mq_open`은 패턴 syscall이 아님 | 아님 |

[I] 표에서 사건 ①②④(AppId 설정, 값 전달, 오류 반환)에 적용되는 클래스는 없다. ③에서 RESOURCE가 적용될 여지가 있지만 대상은 fd table이다. 이것은 "RacePro가 R2를 탐지하지 못한다"는 결과가 아니다. 위 Q5·Q6 셀과 함께 읽는다.

#### C1·C3·C5 보충 사건 대응표

| 사례·사건 | 커널에서 보이는 흔적 | RacePro 입력 | 라벨 |
|---|---|---|---|
| C1 생성: `OS_CountSemCreate`(`sem_init` + 테이블 등록) | 0 syscalls (OSAL 자체 시험, probe) | 없음 | 분석 |
| C1 조회: `OS_CountSemGetIdByName` | 0 syscalls (probe) | 없음 | 분석 |
| C1 수정 후 재시도 대기: `OS_TaskDelay(100)` ×25 | sleep 계열 syscall로 추정 | Table 2 객체를 건드리지 않아 micro-op 없음 | 분석 |
| C3 SB `AppId` 저장, EVS `EVS_AppID`·등록 | syscall 없음 | 없음 | 분석 |
| C3 TIME의 `CreatePipe` | 현재 OSAL에서는 `mq_open`(probe에서 부분 가시). C3의 원 판본(OSAL 4.1.1 posix)의 queue 구현이 `mq_open`을 쓰는지는 이 문서에서 확인하지 않았다 | `mq_open`이라면 R2 ③과 같다: FILES_STRUCT 쓰기(fd table)만 자원 접근이 되고 큐 이름·큐 객체는 Scribe 자원이 아니다(§2.5 K1–K3) | 분석(조건부) |
| C5 `OS_TaskCreate` | `pthread_create` → `clone3`. 에이전트가 쓴 작은 C 프로그램에서 관측된 기록이다(`availability.md` §4 끝). OSAL이나 cFS에서 측정한 것은 아니다 | clone은 HB 노드·간선(코드가 `clone`을 fork로 취급) | 분석 |
| C5 PSP의 `prctl(PR_SET_NAME)` | 소스 수준. ES 레코드 쓰기와 순서가 정해지지 않는다 | 이름 설정은 Table 2 객체가 아니다 | 분석 |
| C5 ES 레코드 쓰기·`GetAppID` 조회 | 0 syscalls(OSAL 시험의 task 레지스트리 대기는 futex 대기만 보임) | 없음 | 분석 |

### 3.6 이 절이 말하지 않는 것

- 어느 셀도 "RacePro가 사례를 탐지하지 못한다"는 뜻이 아니다. 실행하지 않았고, 논문은 go-live 뒤의 자연 발생 실패를 checker가 보고할 수 있음을 인정한다(p.363).
- "RacePro는 공유 메모리를 기록하지 못한다"는 문장은 틀렸다. 기록은 하되 HB와 후보에 쓰지 않는다(p.357, p.358).
- 이 절의 "분석"은 도구의 원래 설계에 대한 것이다. OSAL 레지스트리를 자원으로 추가하는 등의 확장은 EXTENSION이며 원 도구의 결과가 아니다(로드맵 §10.4 끝, `availability.md` §5.1).

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

| 요구 | 레퍼런스 | 값의 상태 | 오늘 상태 |
|---|---|---|---|
| i386 Linux **2.6.35** + Scribe 내장 커널 | 번들 `kernel_config` L3 `2.6.35`, L6 `# CONFIG_64BIT is not set`, L7 `CONFIG_X86_32=y`, L1609 `CONFIG_SCRIBE=y`. 논문 §7 p.363 "Linux kernel 2.6.35" | from-reference | 이 컨테이너에서 충족 불가 |
| 커널을 설치하고 부팅 | `deploy.rb` L24–25 `make install`, `mkinitramfs` | from-reference | **불가.** [M 2026-10-16T00:07Z] `uname -r` = `6.18.44-fc-v139` x86_64 (2026-10-15T08:17Z에는 `fc-v114`; 아래 항목은 두 시각 모두 같다), `/proc/cmdline`에 `nomodule`·`--firecracker-init`, `# CONFIG_KEXEC is not set`, `# CONFIG_KEXEC_FILE is not set`, `# CONFIG_MODULES is not set`, `/dev/kvm` 없음(`CONFIG_KVM=y`지만), vmx/svm 플래그 0개, `/boot` 비어 있음 |
| Python 2, Cython ≥ 0.13, networkx, pygraphviz, `scribe` 확장 | 커널 README 39–44행; `setup.py` L13. 논문 §7 p.363: Python **2.6.6**, Cython **0.14**, Networkx **1.1-2**, UnionFs-Fuse **0.23** | README는 하한만, 논문은 값을 적음. 정책상 "레퍼런스"인지는 PI 판단(§4.3) | [M] `python2`, `python2.7`, `cython`, `qemu-system-i386` 모두 없음 |
| replay용 파일 시스템 체크포인트 | 논문 p.362: unionfs로 체크포인트·복원 | 논문 기술 | 미준비 |
| 하드웨어 | 논문 §7.3 pp.364–365: IBM HS20 BladeCenter(듀얼 3.06 GHz Xeon, HT, 2.5 GB RAM, 40 GB 디스크)와 HP DL360 G3(듀얼 3.06 GHz Xeon, 4 GB, 18 GB 디스크 2개). `kernel_config`는 `CONFIG_SMP=y`, `NR_CPUS=8` | 논문 기술 | 이 컨테이너는 4 vCPU 가상 머신 |
| userland 배포판 | 논문·번들에 이름이 없다. `deploy.rb`의 `apt-get`·`useradd -G admin,sudo`가 Debian/Ubuntu 계열을 암시할 뿐 | **unspecified_by_reference** | — |
| 실패 판정 | 프로그램별 `<test>.test` 스크립트(`racetest.py:151-152`; `scribewrap.py:237-241` `BUG REPRODUCED`/`BUG not triggered`) | 사용자가 작성 | cFS용은 연구 쪽 신규 입력 |
| 문서화된 예제 | `racepro/tests/tests.list` 11개 항목(`fork_one_file`, `ps_grep`, `bashrace` 등)과 각 `.test` | from-reference | 실행 불가 |
| cFS 쪽 | v7.0.1 기준 바이너리 `core-cpu1`은 x86-64 ELF, kernel ≥ 3.2.0 요구. C3·C5의 파생 플랫폼(6.4.x)은 32-bit pc-linux(`-m32`)다 | 사례 기록 | 6.4.x 파생 플랫폼은 i386 커널과 아키텍처가 같을 수 있다. 2.6.35 시대 userland에서 빌드·실행되는지는 `unspecified_by_reference` |

### 4.2 결과 상태 (로드맵 §10.4)

오늘 적용되는 상태는 **다섯 사례 모두 `NOT_RUN`**이다.

| 사례 | 상태 | 이유 |
|---|---|---|
| C1 | `NOT_RUN` | 환경: Scribe i386 2.6.35 커널을 부팅할 수 없다(§4.1). cFS도 그 userland에서 빌드된 적이 없다. 자료 쪽(논문, 코드)은 열려 있다 |
| C2 | `NOT_RUN` | 위와 같고, 원 앱이 비공개다. 가능한 것은 역할 재구성뿐이다 |
| C3 | `NOT_RUN` | 위와 같다 |
| C5 | `NOT_RUN` | 위와 같다 |
| R2 | `NOT_RUN` | 위와 같다 |

- `INAPPLICABLE`을 붙이지 않는다. 논문은 thread를 기록 대상에 넣고(p.354, p.356) 같은 process의 thread 사이 race에 대해 "less effective"라고만 적는다(p.362). 이것이 "명시된 입력·대상 범위"의 제외인지는 PI 판단이다.
- `PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`는 도구를 실행한 뒤에만 쓸 수 있다. 이 문서는 어느 것도 예측하지 않는다. 로드맵이 말하듯 사건 기록의 기준이 부족한 경우를 `ANALYSIS_GAP`으로 서둘러 분류하지 않는다.
- 이 문서의 §3 내용은 라벨 `ANALYSIS`(observation-model analysis, not run)이고, 논문 전문 읽음 + 코드 직접 읽음 등급이다. 원 도구 결과가 아니다.

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

| # | 항목 | 선택지 | 기본값 |
|---|---|---|---|
| P-1 | 논문 §7이 적은 소프트웨어 값(Python 2.6.6, Cython 0.14, Networkx 1.1-2, UnionFs-Fuse 0.23, 커널 2.6.35)과 하드웨어를 `CONDITIONS_POLICY`의 레퍼런스(`from-reference`)로 볼 것인가 | 정책의 레퍼런스 목록은 "도구의 공식 문서: README, 논문의 artifact 절"이다. §7은 실험 서술이다 | 판단 전까지 `unspecified_by_reference` 유지, 논문 서술로 병기 |
| P-2 | 같은 process의 thread 사이 race에 대한 "less effective"(p.362)를 명시된 대상 범위 제외(`INAPPLICABLE`)로 볼 것인가 | 예 / 아니오 | 아니오(`NOT_RUN` 유지). 논문이 thread를 기록하고, PI의 Process-Level 정의가 앱·task 단위라서 |
| P-3 | RacePro를 실제로 돌릴 별도 머신(KVM 가능 또는 bare metal)을 마련할 것인가 | `availability.md` A3 (a)/(b)/(c). userland 배포판 선택은 `unspecified_by_reference`라 PI 승인이 필요 | (a) `NOT_RUN` 유지 |
| P-4 | 이전 시도가 남긴 `build-libscribe32/log`(환경 변수 포함, 자격 증명 이름의 변수 있음)의 처분 | `availability.md` A5. 이번에 다시 열지 않았다 | 그대로 둠 |

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

### 5.1 RacePro가 이미 하는 것 — 차별점으로 주장하지 않는다

로드맵 §10.5는 "이미 가능한 기능은 재발명한 차별점으로 주장하지 않는다"고 한다. 아래는 논문 전문에서 확인한 기존 기능이다.

| 기능 | 근거 |
|---|---|
| 기록된 실행에서 **실행되지 않은 순서를 예측**하고 순서를 바꾼 branch를 **실제 실행(go-live)으로 확인** | §5.3, §6 pp.359–363 |
| 순서위반(execution order violation)을 겨냥한 설계 | p.355 (109건 중 79건) |
| 이름과 자원의 연결(경로 이름 → 부모 inode의 경로별 위치) | §5.2 p.358 |
| 벡터 시계 기반 HB로 동시성 판정 | §5.1 pp.357–358 |
| 후보 우선순위(HB 거리)와 낮은 가치 자원 건너뛰기 | §5.3.1 p.360 |
| 실패가 확인될 때만 보고, 보고된 실행의 결정적 재생 | §6.3 p.363 |
| 사용자 정의 checker로 도메인별 실패 정의 | §6.3 p.363 |

따라서 로드맵 §10.5 표의 "관측된 실행만 다룸 → 예측·탐색 및 실행 확인" 행은 RacePro에 대해 근거가 없다. 이 행을 RacePro와의 차이로 쓰려면 앵커의 종류(커널 객체의 syscall 쌍으로 한정)에 한정해서 써야 하며, 예측·확인 자체를 차별점으로 쓸 수 없다.

### 5.2 가설 (근거가 있는 만큼만)

모든 가설은 **분석**(설계 기술에서 도출, 미실행)이고 연구 가설이다. 어느 것도 "RacePro가 못 한다"는 결과가 아니다. 근거 칸의 §와 p.는 논문의 절·쪽이다.

| 가설 | 대상 사례 | 관찰된 부족 (분석) | 근거 | 연결되는 Kairos 기능 후보 (로드맵 §10.5) | 한계·반증 가능성 |
|---|---|---|---|---|---|
| **H-RP-1** 내부 사건이 후보 생성 입력에 없다 | R2, C3, C5, C1 | 의존성의 공개·조회·값 전달·오류 반환이 함수 호출과 메모리라서 syscall 중심 입력 모델에 나타나지 않는다 | §5.1 p.358(공유 메모리 의존은 HB 밖); §5.2(micro-op은 커널 객체뿐); 코드에 메모리 이벤트 참조 0건; OSAL 자체 시험에서 생성·조회 syscall 0건(probe) | cFS 내부 API·상태 변화·값 전달을 관측하고 소스와 연결 | RacePro에 OSAL 레지스트리를 자원으로 추가하면 메울 수 있다. 그것은 EXTENSION이다. 이 가설은 "Kairos만 가능하다"가 아니라 "원 도구의 입력 모델 밖"이라는 뜻이다. 실행 미확인 |
| **H-RP-2** 이름·ID·등록 정보를 같은 자원·세대로 잇는 장치가 없다 | C1, C5 | OSAL 이름 → ID → 테이블 항목, task ↔ AppId 연결이 도구의 식별 장치(경로, 파일 핸들, pid)에 대응하지 않는다 | §5.2 p.358; 본문에 세대 서술 없음(키워드 0건) | 이름과 객체 ID·세대, task와 AppId의 관계 추적 | **근거가 약하다.** 경로 이름 기반 식별은 RacePro에 이미 있어 "이름으로 식별"은 새롭지 않다. 새로울 수 있는 것은 cFS 내부 레지스트리에 대한 것뿐이다. 세대는 "본문에 없다"는 부재 확인이다 |
| **H-RP-3** 오류 반환·재시도·최종 결과가 도구 안의 일급 입력이 아니다 | C1 (R2, C5에도 같은 성격) | 허용 오류 처리(재시도)와 최종 실패를 기술하는 입력이 없고 판정은 checker에 위임된다. 내부 API 반환값은 입력에도 없다 | §5.1 p.357(HB는 syscall 의미에서 추론한 것뿐); §6.3 p.363 ("can detect only what is checked"); 본문에 retry 서술 없음 | 반환값·재시도·초기화 최종 결과 추적 | 사용자가 최종 초기화 결과를 판정하는 checker를 쓰면 구분할 수 있을 수 있다. 부족이라기보다 "도구 안에 입력 형식이 없다"에 가깝다 |
| **H-RP-4** 고정된 순서의 이유를 도구 안에서 설명하지 않는다 | R2 | 사용자 수준 직렬화가 만든 순서는 HB에 없다. 도구는 불가능한 후보를 replay 결과로 걸러 낼 뿐 순서가 고정된 이유를 설명하지 않는다 | §5.1 p.358(HB가 못 담는 의존, 검증이 걸러 냄); §6.2 p.362(divergence가 필터); R2의 순서는 core 시작 직렬화가 만든다(사례 기록) | 소스 기반 순서 근거·전제·신뢰 범위를 결과에 연결 | divergence 필터링도 일종의 근거이고 실제 동작은 미실행이다. R2의 순서 고정은 사례 기록에 근거한다 |
| **H-RP-5** (2026-10-16 추가) 관측된 실행에 이미 있는 위반은 보고 조건이 겨냥하는 대상이 아니다 | R2 | 보고 조건이 "녹화 때 없던 실패"라서, 매 부팅 나타나는 것으로 기록된 R2 현상은 branch에서 새로 생긴 실패로 보고되는 경우에 해당하지 않는다는 분석 | §6.3 p.363; R2 gdb 3회 모두 거부 24회(기록), 코드상 결정적이라는 예측(`ledger.md` §2, 미실행) | 관측된 실행 하나를 readiness specification에 대어 readiness violation을 판정. 로드맵 §10.5 표에 같은 행은 없고, 가장 가까운 행은 "잘못된 순서가 고정된 이유"(R2)다 | RacePro의 목적(해로운 다른 순서 찾기)과 범위가 다르다는 뜻이지 결함이 아니다. 원 녹화에 checker를 돌리는 일은 RacePro의 race 검증 절차 밖에서도 할 수 있다. RacePro 녹화에서 현상이 나타나는지는 미실행. C1·C3·C5처럼 순서에 따라 나타나는 사례에는 적용하지 않는다 |

**근거가 부족해 가설로 올리지 않는 것**

- **C2 (시작 동기화와 앱 고유 초기화 완료의 혼동, 로드맵 §10.5 3행).** P·R이 의존에 쓰는 자원이 비공개라 RacePro가 그 관계를 표현할 수 있는지 판단할 근거가 없다. 논문에 startup/readiness 상태 개념이 없다는 것만 확인된다. 역할 재구성 실험(결정 P-C2)에서 선택한 요청 방식이 정해진 뒤에야 같은 분석을 할 수 있다.
- **R2와 C3의 통신 경로(`mq_open`).** 2026-10-16 코드 읽기로 큐 이름과 큐 객체에는 Scribe 자원 잠금 경로가 보이지 않고 fd table 접근만 잡힌다는 것을 확인했다(§2.5 K1–K3). 그래도 가설로 올리지 않는다. 다섯 사례의 의존성은 SB 메시지 전달이 아니라 AppId·레지스트리 상태이고, 실제 로그는 실행하지 않아 미확인이기 때문이다. "SB pipe 통신이 도구에 보이지 않는다"는 결과 문장으로 쓰지 않는다.

### 5.3 비교 논문에서 먼저 밝혀야 할 정의 차이

[F] RacePro의 process race는 "multiple processes access shared operating system resources"(초록, p.353)로 정의된다. [D] 이 연구의 Process-Level은 cFS 애플리케이션·task 단위의 운용상 정의이며 OS process로 한정하지 않는다(로드맵 §1.2, T1과 같은 결정 묶음). 두 정의가 달라서, native cFS에서는 대상이 한 process 안의 thread다. 비교 서술은 이 차이를 처음에 밝히고, 이 차이 때문에 생기는 결과를 도구의 결함처럼 쓰지 않는다.

[I] Kairos가 판정하는 대상은 readiness specification이고, RacePro의 판정 대상은 "checker가 보는 실패"다. 두 도구의 출력은 같은 점수로 합치지 않는다(로드맵 §1.4).

---

## 6. 확인하지 못한 것과 다음에 할 일

| # | 미확인 | 이유·다음 단계 |
|---|---|---|
| 1 | Scribe 커널이 POSIX mqueue(`mq_open`, `mq_timedsend`, `mq_timedreceive`)를 추적하는지 | **코드 수준에서 정리 (2026-10-16, §2.5 K1–K3).** `ipc/mqueue.c`에 Scribe 호출이 없다. `mq_open`의 fd 할당·설치는 FILES_STRUCT 쓰기가 된다. 큐 이름 조회와 큐 객체 접근에는 잠금 경로가 보이지 않는다. 남은 것: 실제 로그 확인(실행 필요, P-3) |
| 2 | thread 생성(`clone`)과 thread 종료가 explorer에 어떻게 전달되는지 | 코드는 `clone`을 fork로 취급한다. futex 기반 join에는 HB가 없다. Scribe 상태는 task마다 붙고 explorer는 로그의 PID 사건 값으로 Process를 나눈다(§2.5 K6). 그 값이 thread id인지는 실행하지 못해 확인 불가 |
| 3 | 논문 판본(2011-09)과 공개 커널·도구 스냅샷(2011-08)의 차이 | §2.4의 차이가 판본 차이인지 구현 선택인지 자료로 결정되지 않는다 |
| 4 | 공유 메모리 page ownership 기록을 explorer가 읽을 수 있는지 | **코드 수준에서 정리 (2026-10-16, §2.5 K5).** py-scribe는 `mem_owned_*` 사건을 파싱할 수 있고(주소는 `SCRIBE_MEM_EXTRA` 때만), `racepro` explorer는 쓰지 않는다. 이 사건을 후보 생성에 쓰는 것은 EXTENSION이다 |
| 5 | 문서화된 예제 실행(`tests/tests.list`) | 별도 머신이 있을 때만 가능(`DOCUMENTED-SETUP-RUN`). 이 컨테이너에서는 불가(P-3) |
| 6 | cFS를 i386 2.6.35 시대 userland에서 빌드·실행하는 조건 | 레퍼런스가 정하지 않았다. 별도 조건 기록과 PI 승인이 필요하다(`racepro.md` D3) |
| 7 | 논문의 연구 자료 `races.xlsx`/`races.txt`(109건) | 열지 않았다. 이 문서의 질문과 직접 관련이 없다 |
| 8 | ACM 디지털 라이브러리 판본 | 403. 같은 sha256의 저자 호스팅 PDF가 있어 필요하지 않다 |

---

## 7. 출처 목록

- 논문(전문 읽음): `https://www.cs.columbia.edu/~orenl/papers/sosp2011-racepro.pdf` (sha256 `645a21ba1dc3e4d7f3930294cdbee29429c3f9f6bd43decc0112fa8e133fd64e`; sigops·nieh 호스팅본과 동일).
- HotOS'11 예비 설계(전문 읽음): `https://www.usenix.org/legacy/event/hotos11/tech/final_files/Laadan.pdf` (sha256 `e25380f9d9d8c7e95e837575889725e2f807c737987435bff4ac18630ad7f815`).
- 저자 슬라이드(2차): `https://www.sigops.org/s/conferences/sosp/2011/current/2011-Cascais/25-laadan-slides.pdf`.
- 그룹 페이지·저자 페이지: `https://systems.cs.columbia.edu/projects/racepro/`, `https://www.cs.columbia.edu/~junfeng/papers/racepro/`.
- 코드: `github.com/columbia/racepro` @`681f94e39bf5049e24fd2641099001f5246a7b6d`; `github.com/columbia/linux-2.6-racepro` @`b29e9afb4dae248e83aeb58482fb60013017576f`; `github.com/columbia/racepro-bundle` @`72e9c55dc5c05354dff31820d2ba57b2917b806c`; `github.com/columbia/libscribe-racepro` @`011b64426dbd7d3f9185a8d2b61b0c476ec5164c`; `github.com/columbia/py-scribe-racepro` @`56d6187eafd7ec2d650099c7d7afd6130d4c5bff`. `racepro-bundle`과 `libscribe-racepro`는 HEAD 확인과 개별 파일 읽기만 했다. 커널은 2026-10-16에 일부 경로를 sparse checkout으로 읽었고, `py-scribe-racepro`는 2026-10-16에 받아 읽었다(§1.2 행 22–23).
- 저장소 기록: `baselines/availability.md`, `baselines/racepro.md`, `cases/ledger.md`, `plan/case_basis_register.md`(R2.E5), `plan/process_race_method_matrix.md`(후속 항목 A3), `plan/ROADMAP_20261015.md`(§1.2, §2.2, §9, §10), `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`, `CONDITIONS_POLICY.md`.

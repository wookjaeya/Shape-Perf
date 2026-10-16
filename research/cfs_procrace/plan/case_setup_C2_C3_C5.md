# C2·C3·C5 사례 설정 — 첫 분석·실행 경로 (2026-10-15, 2026-10-16 보완)

- 대상: C2 (nasa/cFE #198), C3 (nasa/cFE #73), C5 (nasa/cFE #72). 보조 쌍: cFE `58abd38d^`/`58abd38d` (P-RGIT).
- 위치와 역할: `plan/case_setup_C2_C3_C5.md`. [ROADMAP_20261015.md](ROADMAP_20261015.md) §12 작업 5의 산출물이며, §9.1(C2)·§9.2(C3)·§9.3(C5)의 `[제안]` 작업을 "첫 분석 또는 실행 경로"로 구체화한다. 조건표·기대 결과표는 병행 문서 `plan/case_basis_register.md`가 다룬다. 이 문서는 그 문서의 ID를 인용하지 않는다.
- 읽은 입력: [PI_DECISIONS_RESOLVED_20261015.md](../Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md) §2·§3.3–§3.6·§4, [ROADMAP_20261015.md](ROADMAP_20261015.md) §3·§6.1·§9·§12·§13, `Architecture/spec/SPEC_FINAL.md` rev 1.1 (§7.6, §8.2, §9.3, §10.1, 부록 A.3·A.8; 2026-10-16에 rev 1.2에서 재확인, 부록 D.1), `Architecture/spec/PI_DECISIONS.md` (D13, D14, D15, P-C1HOLD, P-C2, P-RGIT), [CONDITIONS_POLICY.md](../CONDITIONS_POLICY.md), `cases/ledger.md`, `cases/C2.md`·`C3.md`·`C5.md`, `env/ENV.md`, `env/CONDITIONS.md`, `env/logs/f_root_capability_probe.log`.
- 성격: **읽기, 파일 가져오기, 해시 계산, 압축 해제, 정적 ELF 점검, 구문 분석 전용 프로브**만 수행했다(§0.3). cFS·cFE·OSAL·PSP·CF를 빌드·링크·실행하지 않았고, 패키지를 설치하지 않았고, 파일을 삭제하지 않았고, git add/commit/push를 하지 않았다. 내려받은 외부 바이너리(CentOS RPM의 실행 파일)는 실행하지 못했다(§3.3.1). `kairos/`와 소스 코드를 건드리지 않았다. N·임계값·예산·노력 수치를 새로 정하지 않았다. PI 결정은 결정[D]으로만 적었고 사실로 옮기지 않았다.
- 검토 반영: 검토 지적 11건을 원본과 대조해 반영했다. 항목별 처리는 부록 C이다. ID 표기: `D13`·`D14`·`P-C2`·`Q22`처럼 사례 접두사가 없는 ID는 PI 결정(`Architecture/spec/PI_DECISIONS.md`)이고(모호하면 `PI-D1`처럼 접두사를 붙인다), 사례 기록(`cases/C*.md`)의 결정·가정 ID는 `C2-D4`·`C3-D3(a)`처럼 항상 사례 접두사를 붙이며, `ledger X3`은 `cases/ledger.md` §5의 항목이다.
- 2026-10-16 보완(부록 D): (1) SPEC이 rev 1.2로 바뀌어(헤더 "Revision 1.2 (2026-10-15)", 2026-10-16 일관성 편집 포함) 이 문서가 인용한 SPEC 문구를 rev 1.2에서 다시 찾아 줄 번호를 붙였다. 인용한 문구는 모두 rev 1.2에 남아 있다(부록 D.1). (2) 6.4.1·6.4.2·OSAL 4.1.1 tarball을 다시 내려받아 같은 sha256을 확인했다. (3) 컴파일 DB를 **6.4.2와 앱 번역 단위(5개)까지** 넓혀 같은 구문 분석·민감도 프로브를 돌렸다(§3.2, §3.3, 부록 D.2). (4) C3·C5의 관측 사건과 식별 방법을 §4.3으로 추가했다(ROADMAP §9.2·§9.3의 `[제안]` 항목 대응). (5) 이슈 페이지를 다시 읽으려 했으나 막혔다(§2.1). 이번에도 cFS를 빌드·실행하지 않았고 설치·삭제·git 조작을 하지 않았다.
- 용어: T1 = (a)에 따라 판정 대상은 **준비 상태 명세(readiness specification)**, 전제는 **준비 상태 전제(readiness precondition)**, 위반은 **준비 상태 위반(readiness violation)** 이다. "API contract"는 문서화된 API 의미(헤더의 `\retcode` 등)를 말할 때만 쓴다. 원출처의 "race"는 도구 class로 자동 변환하지 않는다(P-CLASSGT).

## 0. 읽는 법

### 0.1 근거 태그

| 태그 | 뜻 |
|---|---|
| **[F]** | 사실. 출처(permalink·SHA·tarball sha256·파일:줄)를 붙인다. 사례 기록이 보존한 인용을 이 문서에서 다시 읽지 못했으면 "(기록)"을 붙인다 |
| **[M]** | 이 문서 작성 중(2026-10-15) 도구로 측정·계산한 값. 방법을 적는다 |
| **[I]** | 추론·분석. 원출처·도구·논문이 말한 결론이 아니다 |
| **[D]** | PI 결정(2026-10-15 결정문) 또는 SPEC 설계 결정. 사실·실험 결과가 아니다 |
| **[U]** | 확인하지 못한 것, 또는 레퍼런스가 정하지 않은 것(`unspecified_by_reference`) |

조건의 상태 용어는 CONDITIONS_POLICY의 `from-reference` / `deviation` / `unspecified_by_reference`를 그대로 쓴다.

### 0.2 비용 열 — 항목마다 무엇이 필요한가

| 값 | 뜻 |
|---|---|
| **읽기** | 소스·문서를 읽거나, 이미 있는 도구(curl·git·sha256sum·python·clang)로 파일을 가져와 해시·줄을 확인한다. 설치·빌드·실행이 없다 |
| **파싱** | clang/gcc의 `-fsyntax-only`·`-E`·`-M`으로 번역 단위를 구문 분석한다. 객체·실행 파일을 만들지 않는다. D13 (a)의 "분석 전용 파싱" |
| **빌드** | 대상 소스를 컴파일·링크한다. 이 컨테이너 도구로 가능한지는 [U]이며 이 문서에서는 하지 않았다 |
| **실행** | 빌드한 바이너리를 실행·관측한다. 권한·큐 깊이·스케줄링 조건이 붙는다 |
| **설치** | 패키지 설치나 시스템 설정 변경이 필요하다 |
| **플랫폼** | 이 컨테이너와 다른 플랫폼 조건(다른 호스트 OS 이미지, Microblaze 등). 이 컨테이너에서 만들 수 있는지는 항목마다 따로 적는다(시험하지 않은 것은 [U]) |

### 0.3 이 문서에서 실제로 한 일과 하지 않은 일

| 한 일 | 방법 | 하지 않은 일 |
|---|---|---|
| tarball 3개·RPM 3개 내려받기, sha256·gzip 헤더 시각 계산 | SourceForge·CentOS vault에 curl. 압축은 별도 새 디렉터리에만 풀었다 | cFS 빌드(make·cmake)·링크·실행 |
| 소스 줄 번호 확인 | 6.4.1·6.4.2·OSAL 4.1.1 tarball, raw.githubusercontent.com의 고정 SHA 파일에 grep·sed | `make config`·`make` 실행 |
| 컴파일 DB 재구성 | make 조각(`*.mak`, `Makefile`)을 **읽고** 파이썬으로 53개 번역 단위의 명령행을 생성. `make config`의 파일 복사 5건은 파일 복사로만 흉내 냈다 | 패키지 설치, 시스템 설정 변경 |
| 구문 분석 프로브 | clang 18·gcc 13의 `-fsyntax-only`·`-E`·`-M`, clang의 `-ast-dump`·`-fdump-record-layouts-simple` | 앱(sample_app 등)·단위 시험·링크 단계의 분석 |
| 문헌상 host의 헤더로 같은 프로브 | CentOS 6.5 i686 RPM 3개를 가져와 **압축 해제만**(설치 아님) | 그 host의 gcc·실행 환경 재현 |
| (2026-10-16) tarball 재수신·해시, 6.4.2·앱 번역 단위까지 컴파일 DB 재구성과 구문 분석 프로브, 구조체 크기 재측정, Q16 앵커 재확인 | 같은 방법. make는 실행하지 않았고 `make config`의 앱 헤더 복사 3종(`public_inc`·`mission_inc`·`platform_inc`)도 파일 복사로만 흉내 냈다 | 앱 링크, `elf2cfetbl`, 단위 시험 |
| 문헌상 host 사용자 영역의 조립 가능성 점검(§3.3.1) | 같은 vault에서 gcc·binutils·glibc 등 RPM 16개를 더 가져와 새 디렉터리에 페이로드만 풀고, `readelf`로 의존 라이브러리를 정적으로 점검. `command -v`·`docker info`·레지스트리 HEAD 요청 | 풀어낸 바이너리의 **실행**(세션 권한 정책이 거부했고 우회하지 않았다), docker 데몬 기동, 이미지 pull, qemu 설치 |

재현 명령은 부록 A에 있다. 스크래치 경로는 인용하지 않는다.

## 1. 요약

| 항목 | 이번에 확인한 것 | 아직 없는 것·결정 필요 | 비용 |
|---|---|---|---|
| **C2 단일/두 단계 동기화 코드 읽기** | 6.4.2·v6.5.0a의 `WaitForStartupSync`·`ApplicationSyncDelay`·`ES_Main` 말미가 공백 무시 비교로 동일[M]. v6.6.0a·v7.0.1은 `APPS_INIT`·`LATE_INIT`·`WaitForSystemState`·2단계 `MainTaskSyncDelay`[F] (§2.2) | 6.4.1의 세마포어 기반 동기화(Trac #40 이전)는 이 절에서 다루지 않음. C2는 6.4.2를 단일 단계 기준으로 삼는다 | 읽기 |
| **C2 역할·2×2** | P(late init 제공)·R(의존 호출)의 정의를 보고 사실과 재구성 선택으로 분리. 4셀 중 3셀 실행 가능, 1셀(단일 단계×새 provider)은 심볼 부재로 제외[M]. 따라서 PI 결정문·SPEC이 서술한 교차 2×2는 그대로는 실현되지 않는다 (§2.3–§2.4) | 3셀 + B 제외로 충분한지 PI 확인(§7 항목 1), 요청 방식(C2-D4), 앱 수·우선순위·timeout 인자, 단일 단계 쪽 개정(6.4.2 대 v6.5.0a), 6.5.0a·6.6.0a configure 결정 — 모두 P-C2 | 읽기(역할·설계) / 빌드·실행(셀 실행) |
| **C3·C5 분석 기반 아티팩트** | 6.4.1·6.4.2·OSAL 4.1.1 tarball을 원 배포처(SourceForge)에서 지금 가져올 수 있음. sha256이 ledger·C3·C5 기록과 일치[M] (§3.1). 2026-10-16 재수신에서도 HTTP 200, 같은 sha256[M] | 6.4.1·6.4.2의 git 이력은 없음(기록) | 읽기 |
| **컴파일 DB·파싱 (D13 (a)/(b)(i))** | 2026-10-15: 6.4.1 코어·OSAL·PSP 53개 번역 단위. 2026-10-16: 6.4.1과 6.4.2 각각 58개(53 + 앱 5: `sample_lib`·`sample_app`·`ci_lab`·`to_lab`·`sch_lab`). 컨테이너의 32비트 헤더로 clang 18 기본 설정은 두 판 모두 56/58 통과, 오류 2건(같은 두 줄, §3.3), `-Wno-error=incompatible-function-pointer-types`를 주면 58/58, 0 오류. gcc 13은 플래그 없이 58/58 [M]. 이 옵션은 레퍼런스에 없는 우회책이며 `deviation`으로 기록했다(§3.3) | 구문 분석이 통과했다는 사실은 빌드·실행 가능성의 근거가 아님. 단위 시험·링크 단계는 미포함 | 파싱 |
| **헤더·환경 민감도 (deviation)** | 구조체 크기 7종·OS/기본 타입 크기 7종은 컨테이너 헤더(glibc 2.39)와 CentOS 6.5 헤더(glibc 2.12)에서 동일(6.4.1·6.4.2 모두). 전처리한 프로젝트 토큰열은 판마다 58개 중 56개 동일, 2개는 `sched_priority` 멤버 이름 별칭만 다름. clang 대 gcc는 58/58 동일[M] (§3.3) | 민감성 판정과 (ii) 전환 여부는 PI. 이름에 의존하는 바인딩은 보류 대상 후보. 문헌상 host의 gcc·전체 sysroot를 실행해 본 시험은 없음[U] (§3.3.1) | 파싱 / (ii) 확대: 헤더 추출 범위에서는 설치 없음[I], 전체 sysroot·gcc는 미시험[U] |
| **원 사례 플랫폼 대비** | 이 문서의 파생 기반(6.4.1 `pc-linux`, 32비트 x86)은 6.4.1 readme가 지원 런타임으로 적은 플랫폼이다. C3의 보고 플랫폼 Microblaze에 대해서는 `deviation`, C5(#72)의 플랫폼은 `unspecified_by_reference` (§3.3.2) | Microblaze의 OS·PSP·OSAL·툴체인(레퍼런스에 없음). RQ2·H-V 보고에 `deviation`을 이월(PI D13 (b)) | 읽기 |
| **Q16 표 골격** | C3 13행, C5 12행. 6.4.1·6.4.2 파일:줄을 tarball에서 읽어 채움[F] (§4). 2026-10-16 재수신 tarball에서 부록 A.5 앵커가 같은 줄을 돌려줌[M] | Φ_6.4.x의 Allowed 항목(6.4.1 레퍼런스 인용)은 `unspecified_by_reference` 유지. 기계 검증용 golden 시험 미작성 | 읽기 |
| **C3·C5 관측 사건** | 사건 목록, 식별 방법(새 task를 `GetAppID`가 아니라 OS thread와 생성 관계로 식별), 판정 단위와 파생 결과(충돌)의 분리, 6.4.2 diff에 Trac #40·#41·#42가 섞였다는 표시 (§4.3) | 관측 도구·보류 지점·순서 제어 방식은 PI(C3-D3, `ledger.md` X3). 실행은 6.4.x 실행 기반 이후 | 읽기(설계) / 실행 |
| **P-RGIT 보조 쌍** | `58abd38d^` = `6b9c267e`, 각 쪽에서 §5.2가 인용하는 ES 소스 3파일(`cfe_es_api.c`·`cfe_es_apps.c`·`cfe_es_resource.c`)을 가져와 읽음(변경 파일 10개의 목록은 두 커밋의 변경 파일 목록에서 확인), 대기 함수 변경점(앱 레코드 → 태스크 레코드) 확인. 공개 번들 `a41549cb`가 `58abd38d^`와 OSAL·PSP 쌍을 이룸[M] (§5) | 입력 대응 비용 확인 뒤 수행(PI). 쌍의 모든 조건은 `unspecified_by_reference` | 읽기 / 수행 시 빌드·실행 |
| **6.4.x 실행 (분석 전용과 별도)** | 32비트 런타임은 있음. root의 `CAP_SYS_RESOURCE` 부재와 `msg_max` 10이 큐 깊이 10 초과 큐 생성에 걸릴 가능성(§3.5) | 이 조건에서 6.4.1이 기동하는지는 미시험[U] | 실행·플랫폼(권한) |

## 2. C2 — 역할 재구성 (P-C2, Q22)

결과 명칭은 **"C2 메커니즘의 역할 기반 검증"** 이다[D]. 비공개 EVA CWS의 원 실행 재현으로 쓰지 않는다.

### 2.1 공개 보고가 고정하는 것과 열어 둔 것

이슈 #198 본문은 8개 문장이다. 사례 기록은 이를 WebFetch의 125자 이하 조각으로 보존한다(기록). 줄 바꿈·서식은 보장되지 않는다. 댓글은 없다(기록). 이 문서에서는 이슈 페이지를 다시 읽지 못했다(GitHub REST·API는 이 세션에서 403, §3.1). 2026-10-16에 다시 시도했다[M]: `https://github.com/nasa/cFE/issues/198`·`/73`·`/72`와 `api.github.com/repos/nasa/cFE/issues/198`이 모두 HTTP 403("GitHub access to this repository is not enabled for this session")이고, GitHub MCP 도구도 "repository nasa/cfe is not configured for this session"으로 거부했다. 저장소 접근 추가는 사용자 승인이 필요한 조작이라 하지 않았다. raw.githubusercontent.com과 `git ls-remote`(github.com)는 된다.

| 항목 | 보고가 고정하는 것 [F](기록) | 보고가 열어 둔 것 [U] |
|---|---|---|
| 앱 | "one app"(late init을 하는 앱, 이하 **P**)과 "other applications"(**R**). 프로젝트명 EVA CWS | 앱 이름·수·시작 순서·우선순위 |
| P의 구조 | "early" init(로컬 자료구조 초기화) 뒤에 "late" init(다른 앱·라이브러리와의 통신이 필요할 수 있음). 문장 4: late init까지 마쳐야 "completely up and running" | late init의 내용·소요 시간·"fully initialized"의 관측 가능한 의미 |
| 동기화 사용 | P는 late init **전에** `CFE_ES_WaitForStartupSync()`를 호출(6b). R도 "the same sync call"을 사용(7) | 인자(timeout) 값. R의 "other functions" 호출이 sync 반환 뒤였는지. R이 OPERATIONAL로 풀렸는지 timeout으로 풀렸는지 |
| 관측된 현상 | R이 P의 late-phase init 완료 전에 "other functions"를 호출(7). 요청이 완전히 초기화되지 않은 앱에 도달하는 것을 "race condition"이라 부름(5b–5c) | 함수 이름·호출 방식·증상(충돌·오류 코드·로그)·빈도·관측 방법 |
| 제안 | late-phase init을 위한 새 시작 상태(8) | 이름·API |
| 버전 | 6.4.2를 "시작 동기화를 개선한 릴리스"로 언급(1a) | EVA CWS가 실행한 cFE 버전·OSAL·PSP·플랫폼 |
| 연결 | 제목·마일스톤이 6.6.0 VDD의 Trac #167 행과 같고, GitHub# = Trac# + 31 오프셋이 20쌍에서 일치(기록): 18쌍은 제목이 같고 2쌍은 거의 같다(GitHub 제목에 "EDS:"/"SMP:" 접두; `C2.md` §1.3·§12 Q2). 수정은 6.6.0 VDD의 `#167` 행(커밋 `465a958`·`b227d57`, 비공개)[F, 이 문서에서 VDD 행을 `docs/cFE_6_6_0_version_description.md` L189–L191에서 재확인, 태그 `v6.6.0a` = `8811c920`] | #198 = Trac #167은 **추론**[I]. Trac 원문은 읽을 수 없음(`babelfish.arc.nasa.gov`·`web.archive.org` 모두 HTTP 000, 2026-10-15 [M]) |

[I] "R의 호출이 P를 향했다"와 "P가 R에게 서비스를 제공한다"는 보고에 없다(사례 기록 §4·§7이 같은 표시). 로드맵의 provider/service 표현은 레퍼런스가 아니다.

**준비 상태 전제(C2, 역할 수준)** [I]: R의 요청 진입 시점에 P의 late init이 완료된 상태이다. 이것은 연구자가 세운 호출자 요구(QT-3의 둘째 층)이며 원출처나 프레임워크가 말한 사실이 아니다. 인용만 [F]이다: 이슈 문장 4·5b–5c·7(기록), 그리고 수정 쪽 API 문서(v6.6.0a `cfe_es_extern_typedefs.h` L202 `APPS_INIT`: "startup apps have all completed their early init, but not necessarily operational yet"; L279 `RUNNING`: "All initialization is complete. Second sync point.")[F, 재확인]. 호출 대상이 P라는 부분은 [I]이다. 이 전제는 P의 코드(비공개)에서 나온 것이 아니다.

### 2.2 공개 동기화 코드 — 단일 단계 대 2단계

읽은 개정: 6.4.2 tarball(sha256 `c64ed2aa…`)과 v6.5.0a `b2765d9f905aa11f0c3b419b62233597366a7db9`(단일 단계), v6.6.0a `8811c92094512975e14fd545df355705141560b2`와 v7.0.1 `c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd`(2단계). 파일은 raw.githubusercontent.com의 고정 SHA에서 가져왔고 해시는 부록 A.4에 있다. 줄 번호는 모두 해당 파일 기준이다.

| 항목 | 6.4.2 tarball / v6.5.0a (단일 단계) | v6.6.0a (2단계) | v7.0.1 (2단계, ENV 번들 기준) |
|---|---|---|---|
| 시스템 상태 | `#define` 0–5: UNDEFINED, EARLY_INIT, CORE_STARTUP, CORE_READY, **OPERATIONAL=4**, SHUTTING_DOWN=5 (v6.5.0a `cfe_es_apps.h` L90–L95) | enum: …, CORE_READY=3, **APPS_INIT=4**, **OPERATIONAL=5**, SHUTDOWN=6 (`cfe_es_extern_typedefs.h` L184–L214) | 동일 번호 + MAX (`default_cfe_es_extern_typedefs.h` L184–L219) |
| 앱 상태 | `#define` RUNNING=1, STOPPED=2, WAITING=3, INITIALIZING=4. LATE_INIT 없음 (`cfe_es_apps.h` L106–L109) | 순서 있는 enum: UNDEFINED=0, EARLY_INIT=1, **LATE_INIT=2**, RUNNING=3, WAITING=4, STOPPED=5 (L266–L291) | 동일 (`default_…typedefs.h` L264–L289) |
| 앱 생성 때 초기 앱 상태 | INITIALIZING + `++AppStartedCount` (6.4.2 `cfe_es_apps.c` L607·L613; v6.5.0a L676·L682) | EARLY_INIT (`cfe_es_apps.c` L628). 카운터는 사라지고 앱별 상태로 대체 | `cfe_es_apps.c`에서 `EARLY_INIT`을 대입하는 줄을 찾지 못함(grep). 초기값은 레코드 0 초기화에 따른 UNDEFINED일 수 있음[I]. 장벽 판정에는 EARLY_INIT과 같이 "미준비"(`AppState < LATE_INIT`)로 계산됨 |
| 앱 쪽 대기 API | `void CFE_ES_WaitForStartupSync(timeout)` (6.4.2 `cfe_es_api.c` L652–L695; v6.5.0a L651–L694). 본문은 6.4.2와 v6.5.0a가 `diff -w` 동일[M] | `int32 CFE_ES_WaitForSystemState(MinSystemState, timeout)` (`cfe_es_api.c` L584–L673) 신설. `WaitForStartupSync`는 `WaitForSystemState(OPERATIONAL, …)`를 부르는 `void` 래퍼(L686–L689) | `CFE_Status_t CFE_ES_WaitForSystemState` (`cfe_es_api.c` L514–L608), 래퍼 L616–L619 |
| 호출한 앱 자신의 상태 갱신 | INITIALIZING이면 RUNNING으로 바꾸고 `++AppReadyCount` (6.4.2 L670–L673). `RunLoop` 첫 호출도 같음 (L580–L586) | 요구 상태를 호출자에게 **암묵 설정**(L602–L637): 외부 앱은 `≥SHUTDOWN`→STOPPED, `≥OPERATIONAL`→RUNNING, `≥APPS_INIT`→LATE_INIT, 그 밖→EARLY_INIT. 코어 앱은 `≥CORE_READY`→RUNNING. `CFE_ES_SetAppState`는 뒤로 가지 않음(L206). `RunLoop`→RUNNING (L520) | 같은 규칙(L536–L574). `RunLoop`→RUNNING은 직접 대입(L471) |
| 앱 쪽 대기 조건 | `CFE_ES_ApplicationSyncDelay(Min, timeout)`: `SystemState < Min` 이거나 (`AppReadyCount != AppStartedCount` 이고 `Min == UNDEFINED`)인 동안 폴링 (`cfe_es_start.c` L1015–L1017). 외부 앱은 `Min = OPERATIONAL`(`cfe_es_api.c` L686), 코어 앱은 `CORE_READY`(L682) | `while (SystemState < MinSystemState)` 폴링 (`cfe_es_api.c` L649) | 같음 (L585) |
| 시작 스레드(ES main)의 대기 | `CORE_READY` 설정 → `StartApplications` → `ApplicationSyncDelay(UNDEFINED, SCRIPT_TIMEOUT)`로 **모든 시작 앱이 RUNNING**일 때까지 → `OPERATIONAL` 설정 (`cfe_es_start.c` L268–L293; v6.5.0a L266–L291). 대기가 timeout이면 syslog "Startup Sync failed - Applications may not have all started"만 남기고 진행(L286) | 1단계 `MainTaskSyncDelay(LATE_INIT, SCRIPT_TIMEOUT)` (`cfe_es_start.c` L207) → `APPS_INIT` 설정(L214) → 2단계 `MainTaskSyncDelay(RUNNING, SCRIPT_TIMEOUT)` (L223) → `OPERATIONAL` (L233). 각 단계 timeout은 syslog 후 진행(L208–L211, L224–L227). `MainTaskSyncDelay`는 `RecordUsed`이고 `AppState < 요구 상태`인 앱 수가 0이 될 때까지 폴링 (L949–L1005, 조건 L969–L970) | 같은 구조 (`cfe_es_start.c` L210·L216·L225·L234, `MainTaskSyncDelay` L860–L918, 조건 L882) |
| 폴링·timeout 상수 (개정별 구성 파일) | 6.4.2 `cfe_platform_cfg.h`: POLL 50 (L1702), CORE_MAX_STARTUP 30000 (L1726), SCRIPT_TIMEOUT **30000** (L1744). v6.5.0a `cmake/sample_defs/cpu1_platform_cfg.h`: L1812·L1836·L1854 = 50·30000·30000 | `cmake/sample_defs/cpu1_platform_cfg.h`: L1851·L1875·L1893 = 50·30000·**1000** | ENV가 컴파일하는 값은 cFE 모듈 기본값이다(`env/CONDITIONS.md` C8: `sample_defs`가 덮어쓰지 않음): `modules/es/fsw/inc/cfe_es_internal_cfg.h` POLL 50 (L860), SCRIPT_TIMEOUT **1000** (L879); `modules/core_private/config/default_cfe_core_private_internal_cfg.h` CORE_MAX_STARTUP 30000 (L68)[M, 고정 SHA에서 재확인]. 2차 참고로 샘플 구성 `cmake/sample_defs/example_platform_cfg.h`: L838·L856·L84가 같은 값이나 ENV가 컴파일하는 파일은 아니다 |
| timeout의 호출자 가시성 | `WaitForStartupSync`는 `void`. ES main 쪽 timeout은 syslog만 | `WaitForSystemState`는 **호출자 자신의 timeout**에 `CFE_ES_OPERATION_TIMED_OUT`을 반환(L663). `WaitForStartupSync`는 반환값을 버림(L688). ES main 단계 timeout은 상태를 그대로 전진시키므로 대기 중인 R은 `CFE_SUCCESS`로 풀림 | 같음 (L599) |
| 문서상 timeout 최소 | `cfe_es.h` L671–L672: "must be at least 1000. Lower values will be rounded up". **코드는 올림하지 않음**(인자가 그대로 `WaitRemaining`이 됨)[F](기록, v6.5.0a 코드에서 재확인) | `cfe_es.h` L632–L633·L668–L669 | `cfe_es.h` L441–L442 |
| 사용 범위 문서 | "should only be called as the last item of an Apps initialization", "only … by an App that is started from the ES Startup file", "not an option to wait indefinitely" (v6.5.0a `cfe_es.h` L665–L673) | 같은 문구 (L662–L670) | 같은 문구 (L435–L436, L407) |

**상태 전이 요약**(코드 읽기 [I], 읽은 줄은 위 표):

- 단일 단계: 외부 앱 A는 `WaitForStartupSync`(또는 첫 `RunLoop`) 한 번으로 "나는 준비됐다(RUNNING)"를 **게시하고** 동시에 "모두 RUNNING일 때까지 기다린다(OPERATIONAL 대기)"를 **구독한다**. ES main은 모든 시작 앱이 RUNNING이거나 `SCRIPT_TIMEOUT`이 지나면 OPERATIONAL로 올린다. 앱의 late init이 sync 호출 뒤에 있으면, 그 앱의 "준비됐다"는 late init보다 먼저 게시된다.
- 2단계: 앱은 `WaitForSystemState(APPS_INIT)`로 "early init 끝(LATE_INIT)"을 게시하고 기다린 뒤 late init을 하고, `RunLoop` 또는 `WaitForSystemState(OPERATIONAL)`로 "late init 끝(RUNNING)"을 게시한다. ES main은 모든 앱이 `LATE_INIT` 이상이면 `APPS_INIT`, 모든 앱이 `RUNNING` 이상이면 `OPERATIONAL`로 올린다. 각 단계에는 `SCRIPT_TIMEOUT` 소프트 상한이 있다.
- **STOPPED는 장벽을 통과한다**: 앱 상태 STOPPED(5)가 RUNNING(3)보다 크므로 `AppState < RUNNING`이 거짓이 된다(v6.6.0a `cfe_es_start.c` L970, v7.0.1 L882). `ExitApp`이 STOPPED를 쓴다(v6.6.0a `cfe_es_api.c` L451, v7.0.1 L399). 이 사실은 Q22와 직접 이어진다(§2.4).
- 하위 호환 주석(영문 원문 [F], v6.6.0a `cfe_es_api.c` L632–L637): "NOTE -- a call to "CFE_ES_WaitForSystemState()" implies that the calling app MUST also be in the requisite state. This is hooked into here to avoid needing to update all existing apps to add an explicit state change call, but it makes sense because if this was not done an app could be waiting for itself (which will always time out)." 뜻(번역, 인용 아님): 이 함수를 호출하면 호출한 앱 자신도 요구 상태에 있다고 간주하며, 이는 기존 앱마다 명시적 상태 변경 호출을 추가하지 않으려는 목적이다. 따라서 6.4.2 방식(late init **전에** `WaitForStartupSync`)을 쓰는 P는 2단계 코드에서도 RUNNING을 late init 전에 게시한다[I]. 수정은 P가 코드를 바꿔야 효력이 있다.

### 2.3 역할 — 어디까지가 보고이고 어디부터가 재구성인가

| 역할 | 보고에서 오는 부분 [F](기록) | 재구성에서 선택하는 부분 (보고자의 값이 아님) |
|---|---|---|
| **P** (provider, late init 제공) | early init → `WaitForStartupSync` → late init 순서(6b). late init은 다른 앱·라이브러리와의 통신을 포함할 수 있음(3b) | late init의 내용. **관측 가능한 완료 지표**(P 내부의 readiness 표식)를 어디에 두는가. 요청을 받는 방식 |
| **R** (consumer, 의존 호출) | 같은 sync 호출을 사용하고(7), late-phase init 완료 전에 "other functions"를 호출 | R이 부르는 대상·방식(C2-D4). R의 수 |
| **장벽(ES)** | 보고 시점 6.4.2에서 OPERATIONAL 한 단계(1a–1b) | 어느 개정을 쓰는가(§2.4) |

재구성의 원칙[D] (PI 결정문 §3.5의 요지를 옮긴 한국어 요약이며 원문 인용이 아니다; 결정문의 두 문장을 합쳐 적었다): provider의 late initialization과 consumer의 의존 사용을 구분하고, 요청 방식·초기화 내용·우선순위·timeout은 재구성 실험의 선택으로 기록하며 원 보고자의 값으로 인용하지 않는다.

### 2.4 2×2 설계 — 장벽 × provider

축 두 개를 정한다. **장벽** = ES 쪽 코드: 단일 OPERATIONAL(6.4.2 tarball 또는 v6.5.0a) 대 2단계(v6.6.0a, 또는 ENV의 v7.0.1). **provider** = P의 코드: 기존(late init **전에** `WaitForStartupSync`) 대 새(`WaitForSystemState(APPS_INIT)` → late init → `RunLoop`). R의 코드는 구성하는 셀에서 같다(`WaitForStartupSync` 뒤에 의존 호출). **PI 결정문(§3.5)은 이를 두 축을 "교차하는" 2×2로, SPEC §8.2·§9.3은 "a 2 × 2, not a pair"로 서술하며 provider 축을 "on the ENV cFE"로 적는다[D]. 이 문서가 코드로 확인한 바로는 네 셀 가운데 셀 B(단일 장벽 × 새 provider)를 구성할 수 없으므로, 교차 2×2가 서술된 그대로 실현되지는 않는다. 구성 가능한 것은 3셀(A·C·D)이고 B는 아래 이유로 제외한다.** 3셀로 충분한지, B를 별도로 정의할지는 PI 확인 사항이다(§7 항목 1). 이 문서는 어느 쪽도 정하지 않는다.

**1차 대조는 provider의 late initialization이 정상 완료하는 구성이다(Q22)[D].** "초기화는 실패하지 않는다"는 가정은 재구성 구성에만 적용하며 실제 cFS 전체의 보장으로 일반화하지 않는다. 모델 분류는 이 가정의 포함 여부와 함께 보고한다. 초기화 실패가 가능한 구성은 별도 보조 실험으로 구분한다. 구성하는 셀은 **각 셀을 그대로 제시**하고, 제외한 셀 B는 이유와 함께 표시하며, 독립된 실제 결함 네 개(또는 셋)로 세지 않는다[D].

| 셀 | 장벽 × provider | 실행 가능 여부와 이유 | 필요한 기반 | 1차 대조(late init 정상 완료)에서 기대하는 관측 [I] |
|---|---|---|---|---|
| **A** | 단일 × 기존 | **실행 가능.** 보고가 서술한 구성에 가장 가깝다. 이 셀이 "수정 전" 쪽이다 | v6.5.0a: CMake 빌드, OSAL gitlink `7139592f`(`osal-4.2.1a-1-g7139592`)[M: `git ls-tree`, `git describe`], 트리 안 PSP 1.3.0.0(기록). 또는 6.4.2 tarball: classic make, OSAL 4.1.1, PSP 1.2.0.0, `-m32` — 이 행은 D13이 필요하다(SPEC §8.2) | P와 R이 모두 OPERATIONAL에서 동시에 풀린다. P의 late-init 완료 지표가 R의 요청 진입보다 앞선다는 보장이 없다(SPEC LP-CASE-C2 (buggy): `schedule-dependent`, 미실행 설계 기대[D]) |
| **B** | 단일 × 새 | **제외.** 새 provider가 부르는 `CFE_ES_WaitForSystemState`·`CFE_ES_SystemState_APPS_INIT`·`CFE_ES_AppState_LATE_INIT`가 6.4.2 `fsw` 트리 전체와 v6.5.0a의 ES 소스 4개·`cfe_es.h`에 없다(grep 0건 [M]). 컴파일되지 않는다. 심볼을 몰래 추가하면 "새 provider"가 아니라 다른 코드가 된다 | — | 셀 없음. "late init을 sync 호출 **뒤가 아니라 마지막 항목**에 둔다"는 문서화된 사용(`cfe_es.h` L665)은 P가 다른 앱의 로드를 기다린다는 보고의 이유(6a)를 없애므로 별도 역할이며 PI 결정 없이 2×2에 넣지 않는다 |
| **C** | 2단계 × 기존 | **실행 가능.** 하위 호환 주석에 따라 기존 provider도 컴파일·실행된다 | v6.6.0a(트리에 osal·psp 없음; 아래 configure 결정 필요) 또는 v7.0.1(ENV 번들, 이미 빌드 기록 있음) | 기존 provider는 sync 호출 시 RUNNING을 late init 전에 게시하므로 A와 같은 경합이 남는다 — 수정이 호출 측 변경 없이는 효력이 없다는 코드 읽기 결과[I]를 확인하는 셀 |
| **D** | 2단계 × 새 | **실행 가능.** 이 셀이 "수정 후" 쪽이다 | C와 같음 | P의 late init이 끝나 RUNNING이 된 뒤에야 ES가 OPERATIONAL을 올리므로 R은 P의 완료 뒤에 풀린다(G1, `C2.md` §8). 단 ES 2단계의 소프트 timeout(`SCRIPT_TIMEOUT`)이 먼저 지나면 R이 일찍 풀린다 |

**셀 D에서 Q22가 갈리는 지점**[I]:

- late init이 정상 완료하는 1차 구성에서, "P의 late init 완료를 R의 요청 진입까지 **사건 기반으로 보류**"하면 위반은 ES 소프트 timeout이 지나야만 생긴다. 그 경우 이 셀의 분류는 timeout 의존(`timeout-dependent`)이며, SPEC rev 1.1은 이 분류를 "provider의 late init이 실패할 수 없을 때만" 허용한다(PI_DECISIONS의 P-C2 Facts가 SPEC을 인용; 부록 A.8 LP-CASE-C2 (fixed))[D]. 보류로 만든 실행은 자연 실행과 구분한다(P-C1HOLD와 같은 원칙).
- late init이 실패하면(P가 `ExitApp`으로 STOPPED), STOPPED가 장벽을 통과하므로 **timeout 없이도** R이 풀린다(§2.2). 이것은 별도 보조 실험이며 1차 대조의 결과에 섞지 않는다.
- 보류 지점이 만드는 ES timeout 만료는 레퍼런스 상수에서 오지, 새로 고른 값에서 오지 않는다: v6.6.0a 샘플 구성의 `SCRIPT_TIMEOUT` 1000 ms, ENV(v7.0.1) 쪽은 ENV가 컴파일하는 모듈 기본값 1000 ms(`env/CONDITIONS.md` C8; 위 표). 보류의 감시 한도(벽시계 중단 조건)는 C2에 대한 PI 결정이 없어 `unspecified_by_reference`이다. C1에 대해서는 P-C1HOLD가 "중단 조건·감시 한도를 본실험 전에 기록한다"고 정하며, C2에 같은 원칙을 적용하는 것은 유추[I]이고 PI가 정할 일이다. D15는 작업량 단위 탐색 예산의 결정이며 이 한도를 정하지 않는다.

**개정 선택지와 비용** (선택은 P-C2이며 이 문서는 정하지 않는다):

| 선택 | 후보 | 레퍼런스가 말하는 것 | 이 문서의 관찰 |
|---|---|---|---|
| 단일 단계 쪽 | 6.4.2 tarball / v6.5.0a | 이슈가 6.4.2를 지목(1a). v6.5.0a는 동기화 로직 동등성이 `diff -w`로 확인됨[M] | 코드 읽기 결과는 어느 쪽이든 같다. 실행 비용이 다르다: 6.4.2는 classic make + D13 플랫폼(공유 가능, `ledger.md` §3.3), v6.5.0a는 CMake(`cmake_minimum_required 2.6.4`, 이 컨테이너의 CMake 3.28.3은 경고만 낼 것으로 추정[I], 미시험) |
| 2단계 쪽 | v6.6.0a / v7.0.1(ENV) | v6.6.0a가 첫 수정 릴리스(6.6.0 VDD). v7.0.1은 ENV 기준이고 같은 설계(기록) | v7.0.1은 새 빌드가 필요 없는 기반을 쓴다(역할 앱 추가만). v6.6.0a는 트리에 `osal/`·`psp/`가 없다: 공개 번들 태그 `v6.6.0a`(`05d60929`)는 cfe `99038d54`(태그 `8811c920`의 조상, 4커밋 이전), osal `75beb864`(`osal-4.2.1a-3`), psp `8f420020`("Seed from PSP 1.3.0.0 code base")를 고정한다[M]. 6.6.0 VDD는 OSAL 4.2.1·PSP ≥ 1.3.0을 말한다(기록) |
| 6.5.0a configure | OSAL `7139592f` + 트리 PSP | 6.5.0 VDD는 OSAL 4.2.0·PSP 1.3.0.0을 표에 적고, git 쪽 OSAL은 4.2.1a-1이라 다르다(기록). `_ENHANCED_BUILD_`가 CMake 빌드에서 정의되어 정적 앱 목록이 스크립트 앱보다 먼저 생성될 수 있다(v6.5.0a `cmake/global_functions.cmake` L75–L79 [F]) | P-C2의 "configure decision" |

### 2.5 관측할 사건

| 이름 | 무엇 | 위치(개정별) | 방법 | 비용 |
|---|---|---|---|---|
| **E-SYS** | ES 시스템 상태 대입 | 6.4.2 `cfe_es_start.c` L257·L268·L293; v6.6.0a L180·L191·L214·L233; v7.0.1 L183·L194·L216·L234 | 대입 지점의 중단점·프로브 | 실행 |
| **E-APP** | 앱 상태 대입(RUNNING 게시, LATE_INIT 게시, STOPPED) | 단일: `cfe_es_api.c` L585·L672(RUNNING), L509(STOPPED) (6.4.2); v6.5.0a는 L584·L671·L508. 2단계: `SetAppState` 호출 지점 L520(RunLoop)·L637(WaitForSystemState)·L451(STOPPED) (v6.6.0a); v7.0.1 L471·L573·L399 | 같음 | 실행 |
| **E-WAIT** | 앱 쪽 sync 호출의 진입·반환과 반환값(`void` 또는 `CFE_ES_OPERATION_TIMED_OUT`) | `WaitForStartupSync`/`WaitForSystemState` 진입·반환 | 같음 | 실행 |
| **E-MAIN** | ES main 단계의 timeout 여부 | 6.4.2 L283–L287; v6.6.0a·v7.0.1 `MainTaskSyncDelay` 반환(성공/TIMED_OUT) | 반환값과 syslog 문구("Startup Sync failed…") | 실행 |
| **E-P1** | P: sync 호출 진입/반환 | 역할 앱(재구성) | 역할 앱의 함수 심볼 | 빌드·실행 |
| **E-P2** | P: late init 시작·**완료**(준비 지표 설정) | 역할 앱. 완료 지표의 위치가 이 사례의 "게시 지점"이다 | 지표를 설정하는 함수 하나를 프로브 지점으로 둔다 | 빌드·실행 |
| **E-R1** | R: sync 반환 | 역할 앱 | 같음 | 빌드·실행 |
| **E-R2** | R: **요청 진입**(사용 지점) | 역할 앱. 요청 방식에 의존 | 같음 | 빌드·실행 |
| **E-R3** | 요청 결과(준비 전이었는지 여부 포함) | 역할 앱 | P가 지표를 읽어 응답 | 빌드·실행 |

**제안 [U/I]**: 준비 상태 위반의 판정 단위(oracle)를 `E-P2`(완료)가 `E-R2`(진입)보다 **먼저 관측되었는가**로 두는 것. 이것은 `C2.md` A4의 가정이며, `C2.md` §11은 이 가정에 대해 "needs approval before use. None is set yet."이라고 적는다(영문 원문; 뜻은 사용 전 승인이 필요하며 아직 정해진 것이 없다는 것). 이를 정한 PI 결정은 없고, `ledger.md` X7은 (a) 사용 지점의 순서 위반을 주 결과로 삼을지 (b) 사례별로 정할지를 열어 둔다. 충돌·오류 코드를 덧붙이면 파생 결과로 표시한다(`C2.md` A4). 관측 방법으로는 R2에서 쓴 gdb 방식(`env/run_r2_gdb.sh`, `tool/gdb/r2_trace.py`)이 이 컨테이너에 있다(gdb 15.1 [M]). gdb는 중단점을 걸므로 관측 영향 점검(PI-D1) 대상이다.

### 2.6 실험 선택 — 보고자의 값이 아닌 것

| 선택 | 레퍼런스가 정하는 것 | 상태 | 확정 방법 |
|---|---|---|---|
| 단일/2단계 쪽 개정, 6.5.0a·6.6.0a configure | §2.4 | `unspecified_by_reference` → P-C2 결정 | PI |
| 앱 집합: P 1개 + R 1개 이상 | 보고는 R을 복수로 서술하나 수는 없음 | `unspecified_by_reference` | PI. 값을 이 문서가 정하지 않는다 |
| 시작 스크립트 순서·우선순위·스택 | 개정의 샘플 스크립트가 **형식**만 준다 | `unspecified_by_reference` | 형식은 해당 개정 샘플에서 가져오고 값은 PI 결정 후 기록 |
| 요청 방식(R→P): SB 명령 / 라이브러리 호출 / 공유 표식 | 없음(C2-D4) | `unspecified_by_reference` | PI. SB 방식은 큐 손실(D19, `msg_max=10`)과 시간 영향이 섞이고, 호출 방식은 사건이 단순한 대신 앱 경계를 단순화한다는 점이 다르다 |
| P의 late init 내용·소요 | 없음 | 소요는 **고정 지연을 쓰지 않는다**(정책). 사건 기반 보류 | 보류 지점·해제 사건 정의 후 파일럿 |
| sync timeout 인자(P·R) | 문서의 "최소 1000"은 문서 전용. 해당 개정 샘플 구성의 상수(§2.2)는 레퍼런스 | `unspecified_by_reference` | 후보는 샘플 상수와 문서 최소값. 선택 사유 기록 |
| late init 실패 가능 여부 | 없음 | 1차 대조 = 정상 완료 [D]. 실패 가능 = 별도 보조 | Q22 |
| 완료 지표(readiness 표식)의 위치 | 없음(`C2.md` A3) | `unspecified_by_reference` | 역할 앱 설계 시 기록 |
| 플랫폼 | EVA CWS 미상. pc-linux는 6.4.2 지원 목록·6.5.0·6.6.0 시험 목록에 있음(기록) | `unspecified_by_reference`. 선택은 `deviation`이 아니라 가정(`C2.md` A6) | OSAL·PSP는 개정별 짝을 따른다 |
| 판정 단위(oracle) / 주 관측 사건 | 없음. 원 보고가 서술하는 것은 "calling other functions before the late-phase init had completed"뿐(`C2.md` §10 #6, 영문 원문) | `unspecified_by_reference`. §2.5의 `E-P2` 대 `E-R2` 선후는 `C2.md` A4의 **미승인 가정**이다 | PI(`ledger.md` X7: 사용 지점 순서 위반을 주 결과로 삼을지, 사례별로 정할지) |
| 반복 횟수 N | 없음 | PI-D14: S1에서 목적별로 N을 정하고 본실험 전에 동결. 값은 아직 없다 | 새 값을 정하지 않음 |
| 보류 감시 한도·벽시계 실행 시간 상한 | 없음 | `unspecified_by_reference`. C2에 대한 PI 결정이 없다. P-C1HOLD(C1)가 "중단 조건·감시 한도를 본실험 전에 기록"을 정하므로 C2에 같은 원칙을 쓰는 것은 유추[I]. PI-D15는 작업량 단위 탐색 예산이며 벽시계 상한을 정하지 않는다 | PI. 새 값을 정하지 않음 |

### 2.7 C2 경로의 단계와 비용

| 단계 | 내용 | 비용 |
|---|---|---|
| C2-1 | §2.2 코드 비교를 개정 SHA에 고정해 기대 결과표(셀 A·C·D)의 초안 작성 | 읽기 |
| C2-2 | 역할 앱(P·R)·요청 방식·완료 지표·판정 단위(oracle)를 P-C2 결정 후 설계. golden 시험(역할 앱 템플릿)은 VP-D4 전 | 읽기 |
| C2-3 | 셀 C·D를 ENV(v7.0.1)에 역할 앱을 추가한 파생 구성으로 실행 | 빌드·실행. 새 패키지 설치는 없다고 추정[I] |
| C2-4 | 셀 A: v6.5.0a(configure 결정 후) 또는 6.4.2(D13 후) 빌드·실행 | 빌드·실행. 6.4.2 행은 D13과 플랫폼 공유(§3) |
| C2-5 | 보조: late init 실패 구성(Q22) | 빌드·실행 |

## 3. C3·C5 — 6.4.1/6.4.2 분석 기반 (D13 (a)/(b)(i))

결정[D] (PI_DECISIONS_RESOLVED_20261015 §3.4): (a) 분석 전용 파싱 인정, (b)(i)부터 시작(컨테이너의 32비트 헤더, 차이가 있는 조건으로 표시), **(c) F1 전에 배치(결정됨)**. 이 절은 (a)·(b)(i)를 다룬다. (c)는 이미 결정되어 PI에게 다시 묻지 않는다. 남은 PI 몫은 `ledger.md` X3(공유 플랫폼의 OSAL artifact 배치·host·권한·순서 제어 방식)와 기반을 F1 전에 두는 비용이다(§7 항목 2). 파싱 결과를 원 플랫폼 실행 재현으로 쓰지 않는다. 과거 코드의 실제 실행 검증은 별도 산출물이다.

### 3.1 아티팩트의 출처와 해시

원 배포처는 SourceForge이다. 가져오기는 `https://sourceforge.net/projects/…/download`가 `master.dl.sourceforge.net`로 리다이렉트되었다[M]. 2026-10-15에 세 파일 모두 HTTP 200이었다. 2026-10-16에 같은 URL에서 다시 받아도 HTTP 200, 같은 바이트 수, 아래와 같은 sha256이었다[M].

| 아티팩트 | 출처 URL | 바이트 | sha256 (계산) | gzip 헤더 시각(UTC) | 기록과의 일치 |
|---|---|---:|---|---|---|
| cFE 6.4.1 | `https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.1/cFE-6.4.1-OSS-release.tar.gz/download` | 39,586,940 | `ec26e33b4ab40b9616ccdbc984c5a00a790a7d05b260f2628c3335a29c23e98f` | 2014-12-12 17:09:26 | ledger·C3(§3)·C5(§15)와 일치 |
| cFE 6.4.2 | `https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.2-OSS-release.tar.gz/download` | 36,411,531 | `c64ed2aac910e79a641f49279dfdff3b7b49374e54ed56b595d47e16a3f53a4f` | 2015-07-13 19:19:10 | 일치 |
| OSAL 4.1.1 | `https://sourceforge.net/projects/osal/files/OSAL-4.1.1/osal-4.1.1-release.tar.gz/download` | 1,973,805 | `8f0f2b23d8226256fdb8543bf9fcb560b38434411dc12cb4f7f35b9c19e61d46` | 2014-04-22 15:43:05 | C5(§15)와 일치 |

- 6.4.1 readme가 요구하는 OSAL은 "4.1.1 or higher"(`cfe-OSS-readme.txt` L76), 빠른 시작이 쓰는 파일은 `osal-4.1.1-release.tar.gz`(L141)이다[F]. 위 OSAL 파일이 그 파일이다. C5 기록은 같은 이름의 git 태그 `osal-4.1.1`(`2f1fdb0c`)이 `osapi-version.h`만 다르다고 했다(기록; posix `osapi.c`의 sha256은 두 곳에서 `32649c6f…`로 같고, SourceForge 파일에서 이 문서가 다시 계산해 일치를 확인했다[M]). 이 문서는 SourceForge 파일을 기준 artifact로 쓴다. 태그로 빌드하면 `deviation`이다(C5 §3). C3 기록의 "태그와 SourceForge의 동일성 미확인"(C3 A2)은 C5가 이미 해소했으므로 C3 쪽 서술을 맞춰야 한다(`ledger.md` §6).
- 6.4.1·6.4.2는 nasa/cFE git 이력에 없다. 이력은 `792f5e35`(6.5.0a 가져오기, 이미 수정본)에서 시작한다(기록). 둘 다 SourceForge 파일이 유일한 1차 출처이다.
- 해시 위생: C5 기록 §4.3의 "6.4.2 파일이 `94362464…`로 해시된다"는 **git blob SHA-1**이다(`git hash-object`로 `94362464f735207cbaf2729826f15fedefe3fefa` 확인[M]). 같은 파일의 sha256은 `2a1e2f4ce32d05a9…`이다. 6.4.1 `cfe_es_apps.c`의 sha256은 `0a54974ba77af3db…`(C5 기록과 일치), git blob은 `8c6d04174d78a84d…`이다[M].
- 읽은 주요 파일의 sha256 앞 16자리(전체 해시는 재계산 가능): 부록 A.4.

가져오기 경로 점검 [M, 2026-10-15]:

| 경로 | 결과 |
|---|---|
| SourceForge 릴리스 파일 | 성공 |
| `raw.githubusercontent.com` 고정 SHA 파일 | 성공 |
| `git fetch --filter=blob:none <SHA>` (github.com/nasa/cFS) | 성공 |
| `git ls-remote https://github.com/nasa/cFE` (2026-10-16) | 성공. `v6.5.0a`는 주석 태그 객체 `40e7d511…`이고 가리키는 커밋이 `b2765d9f…`, `v6.6.0a` = `8811c920…`, `v7.0.1` = `c5fb2b4d…` (부록 B와 같음) |
| `web.archive.org`, `babelfish.arc.nasa.gov` | HTTP 000(연결 거부). 사례 기록과 같음 |
| `gh api repos/nasa/cFS…`·nasa/cFE REST | 403 "not enabled for this session" |
| `github.com/nasa/cFE/issues/N` HTML, GitHub MCP `issue_read` (2026-10-16) | 403 / "not configured for this session" (§2.1) |

비용: 읽기.

### 3.2 컴파일 DB·include 입력

6.4.1·6.4.2 트리에는 컴파일 DB가 없다. 빌드는 `build/cpu1/Makefile` → `build/cfs.mak` → `build/cpu1/cfe/<서브시스템>/Makefile` → `cfe/fsw/cfe-core/src/make/make-rules.mak`의 규칙이다. 컴파일 명령은 `$(COMPILER) $(LOCAL_COPTS) $(INCLUDE_PATH) $(COPTS) $(DEBUG_OPTS) -DOS_DEBUG_LEVEL=$(DEBUG_LEVEL) -c`이다(`make-rules.mak`). 아래는 Makefile을 **읽어서** 재구성한 입력이며 각 값은 CONDITIONS_POLICY 형식이다.

**상태 열의 `from-reference`가 뜻하는 범위**: 6.4.1 배포 파일이 지원 런타임 대상으로 적은 `pc-linux` 플랫폼(32비트 x86 Linux)에서 읽은 값이라는 뜻이다(readme L100 "1. 32 bit x86 Linux ( CentOS 6.x )", `cfe-config.mak` L27·L32). **C3·C5 사례의 원 플랫폼 조건이라는 뜻이 아니다**(ROADMAP §6.1은 `from-reference`가 해당 원출처의 조건이라는 뜻이며 공개 BP 구성의 값이 원 보고자 구성에서도 쓰였다는 뜻이 아니라고 적는다. 같은 주의를 6.4.1 `pc-linux` 값에 적용한다[I]). C3의 보고 플랫폼은 Microblaze이고 C5(#72)의 플랫폼은 불명이다. 원 플랫폼 대비 상태는 §3.3.2에 따로 적는다.

| 입력 | 값 | 레퍼런스 | 상태 |
|---|---|---|---|
| 소스 | 6.4.1 tarball, 위 sha256 | §3.1 | from-reference |
| OSAL | SourceForge `osal-4.1.1-release`. 별도 경로를 `-I`로 직접 가리켰다. readme의 `mv osal-4.1.1-release osal`은 하지 않았다(6.4.1 tarball의 `osal/`에 `readme.txt`만 있어 literal 이동이면 `osal/osal-4.1.1-release`가 됨[I]) | readme L141–L142; `setvars.sh` `OSAL_DIR=$CFS_MISSION/osal` | 배치 방식은 `unspecified_by_reference` (C5 §10 #6). 분석 전용에서는 이동 없이 경로 지정 |
| 생성 설정 | `make config`의 파일 복사 5건을 파일 복사로 재현: `cfe/fsw/mission_inc/cfe_mission_cfg.h`·`apps/inc/sample_lib.h` → mission_inc; `platform_inc/cpu1/{cfe_msgids.h, cfe_platform_cfg.h}`·`osal/build/inc/osconfig.h` → `build/cpu1/inc` | `build/cfs.mak` `cfs_copy_core_incs`·`cfs_copy_app_shared_incs` | from-reference(단계; 6.4.1 readme 기준 플랫폼, 사례의 원 조건 아님) / 복사 위치는 분석용 별도 디렉터리. 출하 상태의 `build/cpu1/inc`·`build/mission_inc`는 비어 있다[M] |
| 서브시스템 include 경로 | es·sb·tbl·fs: `-I$(OSAL_SRC)/inc -I<자신> -I$(CFE_CORE_SRC)/inc -I$(CFE_PSP_SRC)/inc -I$(CFE_PSP_SRC)/pc-linux/inc -I<mission_inc> -I<cpu1/inc>`. evs·time은 `-I$(CFE_CORE_SRC)/es`를 추가. osal: `-I$(OSAL_SRC)/inc -I$(CFE_CORE_SRC)/inc -I$(OSAL_SRC)/posix -I<cpu1/inc>`. psp: es와 같으나 자기 서브시스템 경로 없음 | `build/cpu1/cfe/<s>/Makefile` | from-reference (6.4.1 readme 기준 플랫폼 `pc-linux`; 사례의 원 조건 아님) |
| 정의(`COPTS`) | `-D_EL -DENDIAN=_EL -DSOFTWARE_LITTLE_BIT_ORDER -D__ix86__ -D_ix86_ -D_LINUX_OS_ -Dposix -DX86PC -DBUILD= -D_REENTRANT -D_EMBED_`. `$(BUILD)`는 make 트리 어디에서도 정의되지 않아 빈 값(`-DBUILD=`)이다[M: grep]. `-D$(OS)`는 `OS = posix`이므로 `-Dposix` | `psp/fsw/pc-linux/make/compiler-opts.mak` L23(`TARGET_DEFS`)·L28(`ENDIAN_DEFS`); `build/cpu1/cfe/cfe-config.mak` L27 | from-reference (6.4.1 readme 기준 플랫폼 `pc-linux`; 사례의 원 조건 아님) |
| 아키텍처·디버그 | `-m32 -Wall -Wstrict-prototypes -g -O0 -DOS_DEBUG_LEVEL=3` | `compiler-opts.mak` L33; `build/cpu1/cfe/debug-opts.mak` | from-reference (6.4.1 readme 기준 플랫폼 `pc-linux`; 사례의 원 조건 아님) |
| `LOCAL_COPTS` | 비어 있음(`EXCLUDE_CFE_TBL`을 정의하지 않음) | `build/cpu1/cfe/es/Makefile` | from-reference(기본; 6.4.1 readme 기준 플랫폼, 사례의 원 조건 아님) |
| 번역 단위 | 53개: ES 11, SB 7, EVS 4, TBL 4, TIME 4, FS 3(= 33), OSAL posix 6(`osapi.c` 등), PSP pc-linux 14(`pc-linux/src` 9 + `psp/fsw/shared` 5) | 각 `cfe_<s>.mak`, `osal.mak`, `psp.mak` | from-reference (6.4.1 readme 기준 플랫폼 `pc-linux`; 사례의 원 조건 아님) |
| 앱 번역 단위 (2026-10-16 추가) | 5개: `THE_APPS := sample_lib sample_app`, `THE_APPS += ci_lab to_lab sch_lab`의 각 앱 `OBJS`(앱마다 1개). include: `-I$(OSAL_SRC)/inc -I$(CFE_CORE_SRC)/inc -I$(CFE_PSP_SRC)/inc -I$(CFE_PSP_SRC)/pc-linux/inc -I$(CFS_APP_SRC)/inc -I<앱>/fsw/src -I<mission_inc> -I../cfe/inc -I../inc`. `../cfe/inc`(= `build/cpu1/cfe/inc`)는 출하 트리에 없는 디렉터리라 아무것도 주지 않는다[M]. 컴파일 줄은 `app-rules.mak` L21(코어와 같은 형식), `LOCAL_COPTS`는 다섯 앱 모두 비어 있다(각 `for_build/Makefile` L36) | `build/cpu1/Makefile` L42–L43; `apps/<app>/fsw/for_build/Makefile`; `cfe-core/src/make/app-rules.mak` L21 | from-reference (같은 단서). C5의 victim 후보 `sample_app.c`가 여기 들어간다 |
| 앱 헤더 복사 (2026-10-16 추가) | `make config`의 `cfs_copy_app_public_incs`·`cfs_copy_app_mission_incs`·`cfs_copy_app_platform_incs`(`build/cfs.mak` L58·L75·L93)를 파일 복사로 흉내 냄. 코어 복사 5건과 합쳐 15건 | `build/cfs.mak` L36–L146 (`cfs_prebuild`) | from-reference(단계) |
| 6.4.2 | `build/` 트리 전체, `psp/fsw/pc-linux`, `psp/fsw/shared`가 6.4.1과 바이트 단위로 같다(`diff -rq` 출력 없음[M]). 6.4.2에 `psp/fsw/beaglebone-linux`가 추가되었으나 pc-linux 빌드에 쓰이지 않는다. 6.4.2 `cfe/fsw/platform_inc/cpu1/`에는 `cfe_platform_cfg.h~`가 함께 들어 있으나 `cfe_platform_cfg.h`와 sha256이 같고(`82eeac232452…`[M]) 복사 규칙의 `*.h`에 걸리지 않는다. 따라서 6.4.2의 컴파일 명령은 경로만 바뀐 같은 58개이다 | 6.4.2 readme: OSAL "4.1.1 or higher"(L76), 지원 런타임 "32 bit x86 Linux ( CentOS 6.x )"(L100) | from-reference (6.4.2 readme 기준 플랫폼 `pc-linux`; 사례의 원 조건 아님) |
| 미포함 | 단위 시험, 링크 단계, `elf2cfetbl` | `build/cpu1/Makefile` | 다음 단계 |

비용: 파싱(설치 없음).

### 3.3 이 컨테이너의 32비트 헤더 대 readme의 host — `deviation` 기록

CONDITIONS_POLICY에 따라 먼저 요구 조건과 레퍼런스를 적고, 우회를 `deviation`으로만 둔다.

| 항목 | 레퍼런스(6.4.1 readme) | 이 컨테이너 [M] | 상태 |
|---|---|---|---|
| 빌드 host | "built on CentOS/RHEL 6.x x86 (32 bit)", "compiled on CentOS 6.5-x86, CentOS-7.0-x64, Ubuntu 12.04 LTS x86, Ubuntu 14.04 LTS x64"(L83·L88–L89). 64비트 host에서는 "build and run as a 32 bit application"(L86–L87). 6.4.2 readme도 같은 줄(L83–L91)에 같은 문구이고, 빠른 시작에 "cFS has been built and run on Ubuntu 13.10 x64 and CentOS 6.5 i386"(L129)을 더한다[F, 2026-10-16 재수신 tarball] | Ubuntu 24.04.4 LTS, x86_64, 커널 6.18.44, 4 CPU | **deviation**. 이유: 이 컨테이너는 readme가 나열한 host 중 어느 것도 아니다. 영향: 아래 표. **그렇게 할 수밖에 없다는 근거는 시험하지 않았다[U]**: 목록 host의 재구성(VM·컨테이너 이미지·사용자 영역 조립)을 시도한 시험이 없다. §3.3.1의 프로브는 사용자 영역 파일이 vault에서 모두 가져와지고 정적 의존이 닫힌다는 것까지만 보였고, 실행은 하지 못했다 |
| 컴파일러 | readme는 gcc 버전을 쓰지 않는다. "On Ubuntu 14.04, the cFE unit tests do not work due to differences in the GCC compiler"(L90–L91) — 컴파일러 차이가 알려져 있다는 뜻 | clang 18.1.3, gcc 13.3.0 | deviation. 목록 host의 gcc 버전은 `unspecified_by_reference`. CentOS 6.5 GA 패키지 세트의 gcc는 `gcc-4.4.7-4.el6`이다(vault 색인; RPM 안 `cc1`의 버전 문자열 `4.4.7 20120313 (Red Hat 4.4.7-4)`[M]). 목록 host가 그 gcc를 썼다는 것은 가정[I] |
| 진단 옵션 `-Wno-error=incompatible-function-pointer-types` (프로브에서만 사용) | 레퍼런스에 없다. 6.4.1 make 트리(`*.mak`·`Makefile`·`*.sh`)에 `-std=`·`-Wno-error`·`-Werror`가 없고, pc-linux 경고 옵션은 `-Wall -Wstrict-prototypes`뿐이다(`compiler-opts.mak` L13)[M: grep] | clang 18 기본 설정에서 번역 단위 2개(`osapi.c:647`, `cfe_psp_start.c:270`)가 오류로 막힘. 이 플래그로 진단을 경고로 내리면 53/53 통과(§3.3 ⑤) | **deviation**(우회책). 이유: clang 16 이후 이 진단이 기본 오류이다. 영향: 오류가 경고가 되어 두 번역 단위가 구문 분석을 통과하며 소스는 바꾸지 않는다. 불가피성은 없다: 컨테이너 gcc 13은 이 플래그 없이 53/53 통과(22 경고). 목록 host의 gcc에서 이 두 호출이 오류였는지는 시험하지 않았다[U] |
| 언어 표준 `-std=gnu89` (프로브에서만 사용) | 레퍼런스에 없다(readme·make 트리가 `-std`를 정하지 않음; 위 grep) | 지정해도 결과가 같다(53/53, 0 오류, 20 경고) | `unspecified_by_reference`. 기본 표준과 비교하려는 프로브 선택이며 레퍼런스에서 읽은 값이 아니다. 근거 [U→부분 확인]: 당초 "gcc 4.x의 기본이 gnu89"라는 배경 지식[U]이었고, 이번에 `gcc-4.4.7-4.el6` RPM의 man 페이지(`gcc.1.gz`)에서 "gnu89 … This is the default for C code"를 읽었다[F: RPM 내용 읽기만]. 목록 host가 그 gcc였는지는 [I] |
| C 라이브러리 헤더 | host 배포판의 것. readme는 "CentOS 6.5-x86"(L88)만 적고 패키지 수준은 적지 않는다 → 정확한 패키지 수준은 `unspecified_by_reference`. 이 문서의 프로브는 CentOS 6.5 GA 트리(`vault.centos.org/6.5/os/i386`)의 `glibc-headers-2.12-1.132.el6`를 썼다. 이는 **CentOS 6.5 GA 패키지 세트를 가정한 프로브 선택**이며 레퍼런스가 정한 값이 아니다. 6.4.1 배포(gzip 헤더 2014-12-12)의 host가 이후 el6 갱신 패키지나 6.6 패키지를 가졌을 수 있다[I] | glibc 2.39-0ubuntu8.9, `libc6-dev-i386` 2.39 | deviation |
| 커널 헤더 | 위와 같이 패키지 수준 `unspecified_by_reference`. 프로브는 GA 트리의 `kernel-headers-2.6.32-431.el6`를 썼다(GA 세트 가정) | linux-libc-dev 6.8.0-106 | deviation |
| 32비트 | `-m32` (`compiler-opts.mak` L33) | `-m32` 사용. 이 배치에서 `clang -m32`의 시스템 include 경로는 `/usr/include`뿐이고 `i386-linux-gnu` 디렉터리가 없다. `/usr/include/bits`·`/usr/include/asm`은 `x86_64-linux-gnu/…`로 가는 심볼릭 링크이고 각각 `libc6-dev-i386`·`gcc-multilib` 패키지가 설치했다(`dpkg -S`). 즉 32비트 파싱은 x86_64 multiarch 헤더를 `__i386__` 분기로 읽는다 | 같은 플래그. 헤더 배치는 deviation |

**민감도 점검 결과** (결정문의 다섯 항목; 같은 53개 번역 단위, clang 18, 컨테이너 헤더 대 CentOS 6.5 헤더):

| 점검 | 측정 [M] | 해석 |
|---|---|---|
| **① 생성 설정** | 출하 상태에서 `make config` 없이 파싱하면 첫 오류가 `osapi.h:126: 'osconfig.h' file not found`. OSAL 경로를 주지 않으면 `cfe.h:52: 'common_types.h' file not found`. 복사 5건을 흉내 내면 해결 | 생성 설정은 tarball 내용이라 host 민감도는 없고, **복사 단계를 충실히 재현했는가**가 민감점이다. 복사 목록은 §3.2 |
| **② include 목록** | 컨테이너 헤더 기준으로 계산한 프로젝트(cFE·OSAL·PSP) 헤더·소스 닫힘은 113개이며, CentOS 헤더 쪽 전처리 토큰열이 51/53에서 같으므로(④) 사실상 같다. 시스템 헤더(clang 내장 제외)는 컨테이너 172개, CentOS 6.5 113개, 공통 93개, 컨테이너만 79개(예: `bits/atomic_wide_counter.h`, `bits/floatn.h`), CentOS만 20개(예: `libio.h`, `stdint.h`, `xlocale.h`) | 시스템 헤더 목록은 host마다 다르다. 이 차이가 프로젝트 코드의 해석을 바꾸는지는 ③–⑤가 답한다 |
| **③ 정수·포인터 크기** | `-m32`에서 `__SIZEOF_LONG__`·포인터·`size_t` = 4. `int32` = `long int`(4), `uint32` = `unsigned long int`(4), `cpuaddr` = `unsigned long int`(4)(`common_types.h` L135·L139·L142). `sizeof` 14종(`EVS_AppData_t` 70, `CFE_EVS_GlobalData_t` 2540, `CFE_ES_AppRecord_t` 168, `CFE_ES_TaskRecord_t` 36, `CFE_ES_Global_t` 34812, `cfe_sb_t` 549076, `CFE_SB_PipeD_t` 68, `pthread_t` 4, `pthread_mutex_t` 24, `time_t` 4, `uint32` 4, `cpuaddr` 4, `void*` 4, `int32` 4)이 컨테이너 헤더·CentOS 6.5 헤더(clang)·컨테이너 gcc 13에서 모두 같다(gcc는 `_Static_assert` 14건 통과, 음성 대조 1건 실패 확인). **`-m32`를 빼면 53개 번역 단위가 모두 실패**한다(오류 106건, 첫 오류 `common_types.h:256 CompileTimeAssert(sizeof(uint32)==4)`) | 크기 수준에서는 헤더에 민감하지 않다. 대신 `-m32`는 **필수**이다: `_ix86_`가 `-D`로 정의되어 `uint32 = unsigned long`이 고정되므로 64비트 대상은 성립하지 않는다. 포인터↔`uint32` 변환이 곳곳에 있다(예: `cfe_esmempool.c` L196·L203, `osapi.c` L968 `OS_TaskGetId`의 `memcpy(&task_id_int,&task_id,sizeof(uint32))`) |
| **④ 매크로가 선택한 소스 경로** | 닫힘 113개 파일 중 환경·컴파일러 매크로를 거는 조건문이 있는 파일은 6개(50줄): `common_types.h` L114(`__GNUC__`)·L126(`_ix86_`‖`__i386__`)·L144(`_ix64_`‖`__x86_64__`)·L162(PPC), `EXCLUDE_CFE_TBL`(`cfe_es_apps.c` L130·L1199, `cfe_es_objtab.c` L57·L67·L100·L128), `SOFTWARE_LITTLE_BIT_ORDER`(`cfe_psp_eeprom.c`), `OS_DEBUG_PRINTF`(OSAL posix; `osconfig.h`에 정의 없음). 전처리 후 프로젝트 파일 토큰열(공백 제거)은 **clang 18 대 gcc 13에서 53/53 동일**(각 1,669,604자). **컨테이너 헤더 대 CentOS 6.5 헤더에서는 51/53 동일**, 다른 2개(`osapi.c`, `cfe_psp_start.c`)는 `sched_priority` 대 `__sched_priority`뿐. clang은 `__GNUC__ 4`(minor 2)를, gcc 13은 `13`을 정의한다 | 닫힘 안에서 `__GNUC__`는 `defined` 형태로만 쓰여 컴파일러 차이가 경로를 바꾸지 않는다. 경로 선택은 `-D`(`_ix86_` 등)와 `-m32`(`__i386__`)가 정하고, `EXCLUDE_CFE_TBL`·`OS_DEBUG_PRINTF`는 정의되지 않은 쪽이 기본이다. 헤더 차이는 **멤버 이름 별칭**에서만 나타난다: CentOS 6.5의 glibc 2.12는 구조체 멤버를 `__sched_priority`로 두고(`bits/sched.h` L76) `sched_priority`를 매크로 별칭으로 정의한다(`sched.h` L45, 전처리 결과가 `__sched_priority`). 컨테이너의 glibc 2.39는 멤버가 `sched_priority`이고(`bits/types/struct_sched_param.h` L25) `/usr/include/sched.h` L47–L48에 항등 매크로(`sched_priority`)와 역방향 별칭(`__sched_priority` → `sched_priority`)이 있다 |
| **⑤ 구조체·반환값 해석** | 반환 코드 상수는 매크로라 동일(`CFE_ES_ERR_APPID` 0xc4000001, `CFE_EVS_APP_NOT_REGISTERED` 0xc2000002, `CFE_EVS_APP_ILLEGAL_APP_ID` 0xc2000003; 오류 심각도 마스크 `0xc0000000`, `cfe_error.h` L132–L136). clang 18 기본 설정에서 **오류 2건**: `osapi.c:647` `pthread_create`에 `osal_task_entry`(`void(*)(void)`)를 전달, `cfe_psp_start.c:270` 시그널 핸들러. 둘 다 `-Wincompatible-function-pointer-types`가 clang 16 이후 기본 오류라서 생긴다. 이 플래그를 경고로 내리면 53/53 통과, 0 오류, 20 경고. `-std=gnu89`를 줘도 같은 결과. 컨테이너 gcc 13은 53/53, 22 경고 | `osapi.c:647`은 `OS_TaskCreate`가 새 task를 `pthread_create`로 시작하는 바로 그 호출이다(C5 Q16 행). 오류가 있는 번역 단위를 버리는 분석기라면 이 호출을 잃는다. `-std=gnu89`는 레퍼런스에서 읽은 값이 아니라 프로브 선택이다(§3.3의 해당 행; 당초 근거는 "gcc 4.x의 기본 표준"이라는 배경 지식[U]) |

**2026-10-16 확장 — 6.4.2와 앱 번역 단위** [M] (같은 방법; 컴파일 DB는 판마다 58개, 부록 D.2):

| 프로브 | 6.4.1 (58) | 6.4.2 (58) |
|---|---|---|
| clang 18 기본 | 56/58, 오류 2 (`osapi.c:647:34`, `cfe_psp_start.c:270:19`; 앱 5개는 통과) | 같음 |
| clang 18 + `-Wno-error=incompatible-function-pointer-types` | 58/58, 0 오류 | 같음 |
| 위 + `-std=gnu89` | 58/58, 0 오류 | 같음 |
| gcc 13 (플래그 없음) | 58/58, 0 오류 | 같음 |
| clang 18 + 우회 플래그, CentOS 6.5 헤더(`-nostdinc`) | 58/58, 0 오류 | 같음 |
| `-m32` 제거 | 0/58 (`common_types.h:256`·`:260` 크기 단정 실패) | 같음 |
| 전처리 프로젝트 토큰열: 컨테이너 헤더 대 CentOS 6.5 헤더 | 56/58 동일. 다른 2개(`osapi.c`, `cfe_psp_start.c`)는 `__` 접두(= `__sched_priority`)만 다름 | 같음 |
| 전처리 프로젝트 토큰열: clang 대 gcc | 58/58 동일 | 같음 |
| `sizeof` 14종(③의 목록) | 컨테이너·CentOS 헤더에서 같은 값: `EVS_AppData_t` 70, `CFE_EVS_GlobalData_t` 2540, `CFE_ES_AppRecord_t` 168, `CFE_ES_TaskRecord_t` 36, `CFE_ES_Global_t` 34812, `cfe_sb_t` 549076, `CFE_SB_PipeD_t` 68, 나머지 7종 ③과 같음 | 6.4.1과 같은 값 |

경고 수는 세는 방법(진단 줄의 `warning:` 수)에 따라 달라 결과 판정에 쓰지 않는다. 해석[I]: 6.4.2 쪽과 앱 번역 단위를 넣어도 (b)(i)의 민감 후보는 늘지 않았다. 영향 후보는 여전히 `sched_priority` 멤버 이름 바인딩과 clang 기본 설정의 두 오류이다. C5의 victim 후보 `sample_app.c`는 두 판 모두 오류 없이 파싱되고 헤더 세트에 따른 토큰 차이가 없다.

**판정에 쓸 수 있는 요약**[I]:

- 구조체 크기·기본 정수형·경로 선택 매크로는 두 헤더 세트에서 같다. 멤버 이름 `sched_priority`는 다르게 보인다(②–④의 관측). 영향을 받는 위치는 RT 우선순위를 OS에 넘기는 저장 `param.sched_priority = 50`(`osapi.c` L443), `priority_holder.sched_priority = os_priority`(L630), `thread_params.sched_priority = …`(`cfe_psp_start.c` L356·L378)이다. SPEC은 RT 주장에 "cited scheduling profile"이 필요하다고 한다(PI_DECISIONS D13의 Facts). 이 저장을 멤버 **이름**으로 바인딩하면 헤더 세트에 따라 달라질 수 있다.
- PI 기준[D] (PI_DECISIONS_RESOLVED §3.4 원문): "관련 경로나 자료형 해석이 헤더·환경에 민감하면 문헌상 host의 sysroot를 확보하는 (ii)로 전환한다. 민감성이 해소되기 전에는 영향을 받는 결과를 보류한다." 이 문서는 전환을 결정하지 않는다. **영향 후보 목록**(보류 대상)은 위 `sched_priority` 바인딩과 clang 기본 설정에서 오류가 나는 두 번역 단위이다.
- (ii)의 입력은 지금 가져올 수 있다[M]: `https://vault.centos.org/6.5/os/i386/Packages/`(HTTP 200)에 `glibc-headers-2.12-1.132.el6.i686.rpm`(sha256 `989ed41829ed46d2b2f2df6c7073ff3ade987c94d7d0c1e0911a908288b03fd3`), `glibc-devel-2.12-1.132.el6.i686.rpm`(`6d0b2cbf484d8e4f72202387852c342b932174c10eeade4589505755e1ffef0a`), `kernel-headers-2.6.32-431.el6.i686.rpm`(`216ee716b0449fd758825db880e6e5461d8268d8db5f016f3261797dc1c4ab93`), `gcc-4.4.7-4.el6.i686.rpm`가 있다(gcc와 의존 RPM 16개는 §3.3.1에서 가져와 압축 해제만 했다). `old-releases.ubuntu.com`·`archive.ubuntu.com`의 eglibc 풀(`libc6-dev_2.15…`, Ubuntu 12.04용)도 HEAD 200이다. 위 헤더 RPM 3개는 이 문서가 **압축 해제만** 해서 같은 프로브를 돌렸다(설치 아님). 완전한 (ii)에는 그 host의 gcc와 기본 표준이 더 필요하며, 그 gcc를 실행한 시험은 없다[U]. PI 결정문은 (ii)를 "문헌상 host의 sysroot를 확보하는" 전환으로, PI_DECISIONS D13은 "obtaining the literature host's sysroot (an install decision)"로 서술한다(뒤의 영문 원문이 설치 결정이라고 명시). 이 문서는 (ii)에 설치가 필요한지 아닌지를 정하지 않는다.

비용: 파싱. (ii) 확대: 헤더 추출만 시험했으므로 그 범위에서는 읽기+파싱이고 설치가 없었다[I]. 전체 sysroot·gcc를 쓰는 비용과 설치 여부는 시험하지 않았다[U]. 설치 결정인지는 PI가 D13의 문구대로 정한다. 목록의 host 자체 재현(VM·컨테이너 이미지·사용자 영역 조립): 시험하지 않았다[U] (§3.3.1).

### 3.3.1 문헌상 host 재구성 가능성 프로브 [M, 2026-10-15]

§3.3의 빌드 host 행은 당초 "이 컨테이너로 만들 수 없다"를 그 근거로 §3.5를 인용했으나 §3.5는 실행 조건만 다루므로 그 근거는 성립하지 않았다. 아래가 이번에 실제로 시험한 것이다. 시험하지 않은 것은 시험하지 않았다고 적는다. 이 프로브는 어떤 조건도 새로 정하지 않는다.

| 경로 | 시험 | 결과 |
|---|---|---|
| 컨테이너 런타임 | `command -v docker podman …`, `docker version`, `docker info` | `docker` 클라이언트 29.3.1이 있다. `docker info`: "Cannot connect to the Docker daemon at unix:///var/run/docker.sock". 데몬이 떠 있지 않다. 데몬 기동은 시스템 변경이라 시도하지 않았고 이미지 pull도 하지 않았다 → 컨테이너 이미지 경로는 **미시험[U]** |
| 레지스트리 도달성 | HEAD 요청만(`registry-1.docker.io/v2/`, `quay.io/v2/`, `ghcr.io/v2/`) | HTTP 401·401·405. 프록시를 통한 연결은 된다(401은 인증 요구). 어떤 이미지가 있는지는 조회하지 않았다 |
| 에뮬레이션·가상화 | `command -v qemu-i386 qemu-system-i386 qemu-system-x86_64 proot bwrap debootstrap`, `/dev/kvm` | 모두 없다 → QEMU·VM 경로는 이 컨테이너에서 설치 없이는 시험할 수 없다. 설치는 하지 않았다 |
| RPM 도구 | `command -v rpm rpm2cpio cpio yum dnf` | 모두 없다. RPM 페이로드는 파이썬(`gzip`·`lzma`, 새 디렉터리에만 쓰기)으로 풀었다 |
| chroot·unshare | `command -v chroot unshare`, `capsh --print` | `/usr/sbin/chroot`, `/usr/bin/unshare`가 있고 root의 bounding set에 `cap_sys_chroot`가 있다. 사용 시험은 아래 마지막 행 |
| CentOS 6.5 i686 사용자 영역 파일 | vault `6.5/os/i386/Packages/`에서 RPM 19개를 가져와 sha256을 기록(부록 A.6)하고 새 디렉터리에 페이로드만 풂: `gcc-4.4.7-4.el6`, `cpp`, `binutils-2.20.51.0.2-5.36.el6`, `glibc`·`glibc-common`·`glibc-devel`·`glibc-headers` 2.12-1.132.el6, `kernel-headers-2.6.32-431.el6`, `libgcc`, `libgomp`, `mpfr`, `gmp`, `ppl`, `cloog-ppl`, `bash`, `coreutils`, `make`, `zlib`, `ncurses-libs` | 모두 가져와졌다. 헤더 RPM 3개의 sha256은 §3.3 요약의 (ii) 불릿에 적은 값과 같다. 일반 파일 2,652개를 풂 |
| 정적 점검(실행 없음) | `readelf -h/-l/-d`로 `gcc`·`cpp`·`cc1`·`collect2`·`ld`·`as`·`make`·`sh`·`bash` 조사 | 모두 ELF32 Intel 80386, 인터프리터 `/lib/ld-linux.so.2`. `DT_NEEDED`는 풀어낸 트리 안에서 모두 해결된다. 단 `zlib`(`libz.so.1`)과 `ncurses-libs`(`libtinfo.so.5`)를 추가한 뒤에야 닫혔다(gcc 의존 목록에 처음 없었다). `cc1` 문자열: `4.4.7 20120313 (Red Hat 4.4.7-4)` |
| **실행 시험** | 풀어낸 `ld-2.12.so --version`과 풀어낸 트리로의 `chroot … gcc --version`을 실행하려 했다 | **실행하지 못했다.** 이 세션의 권한 정책(자동 모드 분류기)이 내려받은 외부 바이너리의 실행을 실행 전에 거부했고, 다른 경로로 우회하지 않았다. 풀어낸 사용자 영역이 컨테이너의 커널(6.18.44)·32비트 로더 위에서 동작하는지는 **미시험[U]** |

판단 [I]:

- "목록 host를 이 컨테이너로 만들 수 없다"는 서술은 근거가 없다. 증거는 사용자 영역의 파일을 가져올 수 있고 정적 의존이 닫힌다는 쪽을 가리키지만, 실행·이미지·VM은 시험하지 않았다. 따라서 §3.3 빌드 host 행의 `deviation`은 사실(이 컨테이너는 그 host가 아니다)이고 불가피성은 [U]이다.
- 이를 갱신하려면 PI가 (a) 데몬 기동·이미지 pull 또는 (b) 내려받은 외부 바이너리의 실행(gcc 4.4.7로 같은 53개 번역 단위를 `-fsyntax-only`로 파싱)을 허용해야 한다. 어느 쪽도 설치·시스템 변경 또는 외부 코드 실행을 수반하므로 이 문서가 임의로 하지 않았다.
- 이 프로브는 분석 전용 파싱(D13 (a))의 입력 확보 비용만 다룬다. cFS 실행 환경의 재현이 아니다.

### 3.3.2 사례의 원 플랫폼 대비 — `deviation` / `unspecified_by_reference`

§3.2의 `pc-linux` 값은 6.4.1 readme가 적은 지원 런타임의 값이다. 사례별 원 플랫폼과 비교한 상태는 다음과 같다.

| 사례 | 보고된 원 플랫폼 (레퍼런스) | 이 문서의 파생 기반 | 상태 | 이유·영향·이월 |
|---|---|---|---|---|
| **C3** (#73) | Xilinx Microblaze, GRC의 EVA 팀(#73 본문, #71)[F](기록: `C3.md` §3·§10 행 6). Microblaze의 OS/RTOS·BSP/PSP·OSAL 버전·툴체인은 `unspecified_by_reference`(`C3.md` §3·§10 행 7). 6.4.1 배포의 PSP 목록에 Microblaze PSP가 없다(`C3.md` §3: `grut699-vxworks6`, `mac-osx`, `mcf5235-rtems`, `mcp750-vxworks6.4`, `pc-cygwin`, `pc-linux`)(기록) | 6.4.1 `pc-linux` PSP, `OS = posix`, 32비트 x86 Linux(`-m32`) | **deviation** (Microblaze에서) | 이유: Microblaze 쪽 조건이 레퍼런스에 없어 원 조건을 구성할 수 없다. 영향: 스케줄링·메모리 배치·fault 동작이 다르다(`C3.md` A1)[I]. 이월: PI_DECISIONS D13 하위 항목 (b)와 SPEC §10.1이 요구하는 대로 C3의 RQ2와 H-V 보고에 이 `deviation`을 싣는다 |
| **C5** (#72) | #72 본문은 플랫폼을 적지 않는다. 형제 이슈 #71·#73이 Microblaze를 언급하나 #72 자체는 아니며, 연결은 추론이다(`C5.md` §1의 "#72 itself names no platform", §3 표의 "Platform / OS / CPU count of the original observation" 행, §12 Q3). 6.4.1·6.4.2 VDD의 시험 플랫폼은 릴리스 시험이지 #72 관측이 아니다(기록) | 같음 | 원 플랫폼 **`unspecified_by_reference`** | 비교할 레퍼런스 값이 없으므로 `pc-linux` 기반은 `deviation`이 아니라 **가정**이다(`C2.md` A6와 같은 처리)[I]. 결과 보고에서 이 기반이 C5의 원 조건이라고 읽히지 않게 한다 |
| **C2** (#198) | EVA CWS의 OS·CPU·플랫폼 미상 `unspecified_by_reference`(`C2.md` §3) | v6.5.0a·6.4.2·ENV 등 개정별 기반(§2.4) | 가정(`C2.md` A6) | §2.6 참조 |

### 3.4 분석 전용 산출물의 한계

- 파싱 통과는 **구문 수준**의 사실이다. 링크·실행·`elf2cfetbl`·앱을 포함한 빌드 가능성의 근거가 아니다.
- 2026-10-15의 프로브는 6.4.1만 돌렸다. 2026-10-16에 6.4.2와 앱 번역 단위까지 같은 방식으로 돌렸다(§3.3 확장 표). 두 판의 소스 차이는 `cfe_es_*`, `cfe_evs_*`, `cfe_sb_task.c`, `cfe_tbl_task.c`, `cfe_time_task.c`, `cfe_platform_cfg.h`, `cfe_error.h`, `cfe_version.h`이다(C3 §3 기록). 빌드 조각은 같다(§3.2).
- 입력·대응 실패는 탐지 기법의 실패로 세지 않는다[D]. 위 오류 2건과 `sched_priority` 차이는 입력·대응 문제로 분류한다.

### 3.5 실행 측면 — 분석 전용과 별도 산출물

6.4.1 pc-linux 바이너리를 실제로 돌리려는 경우의 조건이다. 이 문서는 아무것도 실행하지 않았다.

| 조건 | 레퍼런스 | 이 컨테이너 [M] | 비용·판단 |
|---|---|---|---|
| 32비트 런타임 | readme: 32비트 앱으로 실행 | `libc6-i386` 2.39, `/lib/ld-linux.so.2` 있음 | 있음 |
| 큐 깊이 | readme L168–L174: `msg_max`를 올리거나 root로 실행 | `msg_max` = 10. root는 `CAP_SYS_RESOURCE`가 없다(`env/logs/f_root_capability_probe.log`: CapEff에 비트 24 없음). v7.x OSAL에서 root 실행이 `OS_QueueCreate … errno 22`로 실패한 기록이 있다(`env/ENV.md` D-4) | OSAL 4.1.1 posix `OS_QueueCreate`는 `mq_maxmsg = queue_depth`로 `mq_open`한다(`osapi.c` L1598·L1625)[F]. 깊이 10을 넘는 큐가 있으면 root여도 `mq_open`이 EINVAL일 것으로 추정[I]. **readme의 "root로 실행" 대안이 여기서는 성립하지 않을 수 있다.** 해결은 `msg_max` 상향(호스트 공유 sysctl; 시험하지 않음) 또는 권한 부여 — 플랫폼·권한 결정 |
| RT 우선순위 | OSAL 4.1.1은 `geteuid()==0`일 때만 SCHED_FIFO와 재매핑 우선순위를 건다(`osapi.c` L435·L587–L632)[F]. readme L176–L177 | root, `CAP_SYS_NICE` 있음 | 가능. 적용 여부는 실행 중 확인해야 함(U-RT) |
| CPU 수 | 레퍼런스 없음 | 4 CPU, `taskset` 있음 | 우선순위 기반 순서(C3-D3(a))는 단일 CPU 또는 동등한 제한이 필요하다(`C3.md` A10). `taskset`으로 가능하나 값은 `unspecified_by_reference` |
| 순서 제어 | C3의 순서는 우선순위가 만든다(이슈 항목 2·3) | — | (a) root+RT+단일 CPU, (b) 사건 기반 보류(정책이 선호). 선택은 PI(C3-D3, `cases/C3.md`) |

비용: 빌드·실행·권한(플랫폼). 설치는 필요 없을 수 있다[I]. 다만 `msg_max` 상향은 시스템 설정 변경이다.

## 4. Q16 표 골격 — 상태·반환·소스 위치의 버전별 대응

Q16의 범위[D]: Φ_6.4.x의 outcome map·binding과, C3·C5의 Allowed 항목에 쓸 6.4.1 레퍼런스. 이 표는 그 입력의 **골격**이다. 위치는 6.4.1/6.4.2 tarball(§3.1)에서 `grep`/`sed`로 읽었다[F, M]. 표의 "6.4.1/6.4.2" 열은 줄 번호이다. 파일 이름이 같으면 한 번만 쓴다. 두 개정에서 파일이 바이트 단위로 같은 경우(sha256 앞 12자리 비교)는 "동일"로 표시했다. 기계 검증용 앵커(정규식)는 부록 A.5에 있다. 상태 이름은 Φ authoring의 입력일 뿐이며 사실 진술이 아니다.

### 4.1 C3 — TIME task의 SB 호출이 SB·EVS 초기화보다 앞섬 / EVS의 잘못된 인덱스

준비 상태 전제(코드 수준 세분화, `C3.md` §7; 연구자가 세운 호출자 요구이며 [I]이다. 이슈·6.4.2 VDD가 말하는 것은 아래 충분 조건뿐이다): TIME의 `CFE_SB_CreatePipe`가 이벤트를 보내기 전에 (a) SB가 `CFE_SB.AppId`를 게시하고 EVS에 등록했고, (b) EVS가 `CFE_EVS_TaskInit`을 마쳤다. 충분 조건(이슈·6.4.2 VDD): 각 코어 앱이 다음 앱 시작 전에 RunLoop에 도달한다.

| ID | 경로 단계 | 상태 / 자원 | 6.4.1 | 6.4.2 | 반환·결과 | 판 간 차이 |
|---|---|---|---|---|---|---|
| Q16-C3-01 | SB가 자기 AppId 게시 | `CFE_SB.AppId` (파일 범위, 초기값 0) | `sb/cfe_sb_task.c` 정의 L105, 게시 `CFE_ES_GetAppID(&CFE_SB.AppId)` **L198**(반환값 무시) | 정의 L105, 게시 **L206** | `CFE_ES_GetAppID`: `CFE_SUCCESS` / `CFE_ES_ERR_APPID`(not-ready 후보) | 줄만 이동 |
| Q16-C3-02 | SB가 EVS에 등록 | `AppData[SB id].RegisterFlag` | `CFE_EVS_Register(CFE_SB.EventFilters…)` **L257** | **L265** | `cfe_evs.c` L128에서 `RegisterFlag = TRUE` | 줄만 이동 |
| Q16-C3-03 | EVS 자기 AppId의 초기값과 게시 | `CFE_EVS_GlobalData.EVS_AppID` | 초기값 `CFE_EVS_UNDEF_APPID`(0xFFFFFFFF, `cfe_evs_task.h` L105) `cfe_evs_task.c` L190. 게시 `EVS_GetAppID(&CFE_EVS_GlobalData.EVS_AppID)` **L385**(전역에 직접) | 같은 초기값 L190. 지역 `AppID`에 L394, 전역 대입 **L436**(F2: 마지막 단계) | — | **F2**: 게시 시점이 TaskInit 끝으로 이동 |
| Q16-C3-04 | EVS의 자기 등록·파이프 | `AppData[EVS id]` | `CFE_EVS_Register(NULL,0,BINARY)` **L393**, `CFE_SB_CreatePipe` L401 | L402, L410 | — | 줄만 이동 |
| Q16-C3-05 | TIME 초기화의 EVS 등록·파이프 | — | `cfe_time_task.c`: `CFE_EVS_Register(NULL,0,0)` **L252**, `CFE_SB_CreatePipe(&…CmdPipe…)` **L308** | L260, L316 | — | 줄만 이동 |
| Q16-C3-06 | **사용 지점**: SB가 SB의 AppId로 이벤트 송신 | `CFE_SB.AppId`를 읽음 | `sb/cfe_sb_api.c` `CFE_SB_CreatePipe` L171, 무조건 DEBUG 이벤트 `CFE_SB_PIPE_ADDED_EID` **L264** | 동일 파일(sha256 `a969462dbe06…`) | 반환값은 `CFE_EVS_SendEventWithAppID`에서 버려짐 | 없음 |
| Q16-C3-07 | EVS 송신 API의 분기 | `AppData[AppID].RegisterFlag` | `evs/cfe_evs.c` L277 정의, **L285** `AppID >= CFE_ES_MAX_APPLICATIONS` → `CFE_EVS_APP_ILLEGAL_APP_ID`; **L289** `RegisterFlag == FALSE` → `EVS_NotRegistered` (L292) | 동일 파일(`22bf2c12f7a2…`) | `CFE_SUCCESS` / `CFE_EVS_APP_NOT_REGISTERED` / `CFE_EVS_APP_ILLEGAL_APP_ID` (헤더 `cfe_evs.h` L296–L326이 같은 목록을 문서화) | 없음 |
| Q16-C3-08 | 미등록 앱의 첫 이벤트 | `AppData[AppID].EventCount` | `evs/cfe_evs_utils.c` `EVS_NotRegistered` L194–L220, `EVS_SendEvent(CFE_EVS_ERR_UNREGISTERED_EVS_APP…)` **L211** | 같은 줄 | `CFE_EVS_APP_NOT_REGISTERED` 반환(L218) | 없음 |
| Q16-C3-09 | EVS 자신의 송신 | `EVS_AppID` 읽음 | `EVS_SendEvent` L608, `EVS_IsFiltered(CFE_EVS_GlobalData.EVS_AppID…)` **L616**, 범위 검사 없음 | L620–L621 `EVS_AppID < CFE_ES_MAX_APPLICATIONS &&` 가드 추가 | — | **F1**: 사용 지점 가드 |
| Q16-C3-10 | **잘못된 인덱스 사용** | `AppData[AppID]` | `EVS_IsFiltered` L235, `AppDataPtr = &CFE_EVS_GlobalData.AppData[AppID]` **L244**(주석 L243 "Caller has verified that AppID is good") | 같은 줄(F1로 도달이 막힘) | `-m32`에서 `sizeof(EVS_AppData_t)`=70[M]이므로 인덱스 0xFFFFFFFF는 `AppData`의 시작보다 70바이트 앞을 가리킨다[I]. 실제 fault 여부는 `unspecified_by_reference`(`C3.md` A4) | F1이 이 줄 이전에서 차단 |
| Q16-C3-11 | **순서 보장** (F3) | `AppStartedCount`·`AppReadyCount`·`SystemState` | 코어 task의 `CFE_ES_WaitForStartupSync` 호출이 없음: `cfe-core/src` 전체(단위 시험 제외)를 grep하면 정의(`cfe_es_api.c` L676)와 주석(`cfe_es_start.c` L281)만 나온다[M]. ES 쪽 코어 task 사이 대기도 없음 | 각 코어 `TaskMain`에서 `CFE_ES_WaitForStartupSync(CFE_CORE_MAX_STARTUP_MSEC)`: SB L143, EVS L336, TIME L209, ES `cfe_es_task.c` L209, TBL `cfe_tbl_task.c` L240. ES `CreateObjects`는 생성 후 `ApplicationSyncDelay(UNDEFINED, CFE_CORE_MAX_STARTUP_MSEC)` **L936**, 실패 시 `CFE_PSP_Panic(CFE_PSP_PANIC_CORE_APP)` **L949**, `++AppStartedCount` L849 | `CFE_ES_OPERATION_TIMED_OUT` 신설(`cfe_error.h` L487). 상수: POLL 50 (`cfe_platform_cfg.h` L1702), CORE_MAX 30000 (L1726) | 6.4.2에서 신설 |
| Q16-C3-12 | 생성 순서와 우선순위 | 코어 task 생성 순서, 우선순위 | `es/cfe_es_objtab.c` L120 EVS, L122 SB, L124 ES, L126 TIME, L129 TBL. `cfe_platform_cfg.h` EVS 61 (L1115), SB 64 (L1138), ES 68 (L1161), TIME 60 (L1188), TBL 70 (L1217) | 동일 파일(objtab `a9da09fabd83…`)·동일 값 | 숫자가 작을수록 높은 우선순위(L1185–L1186, `C3.md` 기록) | 없음 |
| Q16-C3-13 | ES의 코어 task 생성과 등록 | `TaskTable[id].AppId` | `es/cfe_es_start.c` `CreateObjects` L803, 락 L838, `OS_TaskCreate` **L875**, `AppId = j` L919, 해제 L933 | 락 L816, `OS_TaskCreate` **L855**, `AppId = j` L899, 해제 L913 | 락이 생성·등록을 감싼다(C5의 `AppCreate`와 대비) | 줄 이동 + 대기 추가 |

**Allowed 후보 레퍼런스(6.4.1)**: `CFE_ES_GetAppID`는 `CFE_SUCCESS`·`CFE_ES_ERR_APPID`·`CFE_ES_ERR_BUFFER`를 반환 코드로 문서화하고 "All tasks associated with the Application would return the same Application ID"라고 쓴다(`cfe_es.h` L598–L623). `CFE_EVS_SendEventWithAppID`는 `CFE_SUCCESS`·`CFE_EVS_APP_NOT_REGISTERED`·`CFE_EVS_APP_ILLEGAL_APP_ID`와 "`CFE_ES_GetAppID`·`CFE_SB_SendMsg`의 오류 코드"를 문서화한다(`cfe_evs.h` L296–L326). 두 헤더는 6.4.1과 6.4.2에서 동일하다(sha256 앞 12자리 `9fde4f8b67b6`, `82a5f6a748ec`). 이것들은 **프레임워크가 허용하는 반환 목록**일 뿐이며, 호출자의 요구를 충족하는지는 QT-3에 따라 별개이다. 호출자 쪽 허용 처리(어느 반환을 정상 continuation으로 보는가)를 인용할 6.4.1 레퍼런스는 이 문서에서 찾지 못했다 → C3의 Allowed 항목은 `unspecified_by_reference`로 둔다(H-V의 표현과 일치: "Allowed unspecified until Φ_6.4.x cites 6.4.1 references").

### 4.2 C5 — `OS_TaskCreate` 전 락 해제, 재획득, 등록 전 `GetAppID`

준비 상태 전제(`C5.md` §7의 정식화를 옮김; 연구자가 세운 호출자 요구이며 [I]/[D]이다): 새 task가 처음 `GetAppID`를 읽기 전에 생성자의 등록이 끝나고 락이 풀려 있다. **위반**은 아직 등록되지 않을 task의 `GetAppID`가 실패하는 것이다. [F]는 인용에만 쓴다: 이슈 본문 "opportunity for the child thread to read the shared table data before it is fully populated … CFE_ES_GetAppID() … will fail"(`C5.md` §7 출처 1, 기록)과 아래 표의 코드 줄. "새 task가 먼저 실행되었다는 사실만으로는 위반이 아니다"는 원출처의 문장이 아니라 로드맵·연구자의 정의이다(`C5.md` §7이 로드맵을 출처로 적는다. ROADMAP_20261015 §9.3의 완료 기준도 같은 구분을 요구한다). 로드맵은 CONDITIONS_POLICY가 인정하는 레퍼런스가 아니다.

| ID | 경로 단계 | 상태 / 자원 | 6.4.1 | 6.4.2 | 반환·결과 | 판 간 차이 |
|---|---|---|---|---|---|---|
| Q16-C5-01 | `AppCreate` 진입: 슬롯 예약 | `AppTable[i].RecordUsed` | `es/cfe_es_apps.c` `CFE_ES_AppCreate` L426, 락 L449, `RecordUsed = TRUE /* Reserve Slot */` L456, 해제 L460 | 동일 줄 | — | 없음 |
| Q16-C5-02 | **락 범위** (핵심) | ES 공유 데이터 락 | 락 **L571** → 채움 L575–L608 → **해제 L610** → `OS_TaskCreate` **L614** (실패 경로 L628 재획득/L630 해제) → **재획득 L637** → TaskTable 채움 L643–L655 → 해제 **L673** | 락 L571 유지 → `++AppStartedCount` **L607** → 상태 INITIALIZING L613 → `OS_TaskCreate` **L619** → (실패 해제 L634) → 채움 L646–L658 → 해제 **L669** | — | **Trac #41**: 해제/재획득 제거(락 범위 확장) |
| Q16-C5-03 | 등록 기록 (게시 지점) | `TaskTable[TaskId].RecordUsed`, `.AppId`, `.TaskId` | `RecordUsed = TRUE` **L649**, `AppId = i` **L651**, `TaskId` L652; 사용 중 슬롯 점검 L643–L646; `RegisteredTasks++` L662; `AppStartupCounter++` L670 | `RecordUsed = TRUE` L652, `AppId = i` L654, `TaskId` L655; 점검 L646–L648; `RegisteredTasks++` L665; `AppStartupCounter++`는 제거됨 | — | 줄 이동, 카운터 변경 |
| Q16-C5-04 | OSAL: task 시작과 ID 반환 | `OS_task_table[id].id`, `*task_id` | OSAL 4.1.1 `src/os/posix/osapi.c`: `OS_TaskCreate` L480; 표 락 해제 L553; **`pthread_create` L645**(새 thread가 이 시점부터 실행 가능); 표 락 재획득 L695; **`*task_id = possible_taskid` L690**; RT는 `geteuid()==0`일 때만 L587–L632 | 같은 OSAL(두 개정 모두 4.1.1) | `OS_SUCCESS` 등 | 없음. 단 `pthread_create` 호출 L645–L648이 `osal_task_entry`를 `void*(*)(void*)`에 전달하는 부분이 clang 18 기본 오류 (§3.3 ⑤) |
| Q16-C5-05 | 새 task의 등록 → 식별 | `thread_key`, `OS_TaskRegister`, `OS_TaskGetId` | `OS_TaskRegister` L906–L948: `pthread_self()`를 `OS_task_table`에서 찾아 없으면 `OS_ERR_INVALID_ID`(L932). `OS_TaskGetId` L959–L972: 등록 전이면 0을 반환하고 반환 값은 포인터의 하위 4바이트를 `memcpy`(L968). `CFE_ES_RegisterApp` `es/cfe_es_api.c` L767, `OS_TaskRegister()` L776 | `CFE_ES_RegisterApp` L706, L715 (본문 동일[M]) | `OS_SUCCESS` / `OS_ERR_INVALID_ID` / `OS_ERROR` | 줄 이동. `OS_TaskGetId`가 0을 반환하면 `GetAppID`가 `TaskTable[0]`을 읽는다(`C5.md` A8) |
| Q16-C5-06 | **사용 지점**: 등록 전 `GetAppID` | `TaskTable[TaskId].RecordUsed` | `es/cfe_es_api.c` `CFE_ES_GetAppID` L846, 결과 초기값 `CFE_ES_ERR_APPID` L848, 검사 **L861**, 성공 시에만 `*AppIdPtr` 기록 L863 | L785, 초기값 L787, 검사 **L800** (본문 동일[M]) | `CFE_SUCCESS` / `CFE_ES_ERR_APPID`(0xc4000001, `cfe_error.h` L223). 실패 때 `*AppIdPtr` 불변 | 줄 이동 |
| Q16-C5-07 | 락 없는 변형 | 같음 | `CFE_ES_GetAppIDInternal` L1873–L1893. 사용: `RunLoop` L551, `WaitForStartupSync` L700, `ExitApp` L445, `CreateChildTask` L1081, `ExitChildTask` L1345 (`C5.md` F5) | L1812–L1832 (본문 동일[M]) | 같음 | 줄 이동 |
| Q16-C5-08 | EVS 등록의 소비 | 반환 전파 | `evs/cfe_evs_utils.c` `EVS_GetAppID` L122–L140(`CFE_ES_GetAppID` L127, 범위 검사 L132). `CFE_EVS_Register` L95, `EVS_GetAppID` L105, 성공일 때만 등록 L107–L131 | 동일 줄 | `CFE_ES_ERR_APPID`가 그대로 전파되어 등록이 건너뛰어짐 | 없음 |
| Q16-C5-09 | SB 파이프 생성의 소비 | 오류 무시 | `sb/cfe_sb_api.c` `CFE_SB_CreatePipe` L171: `AppId = 0xFFFFFFFF` L173, **`CFE_ES_GetAppID(&AppId)` L224 반환 무시**, `CFE_ES_GetAppName(AppName, AppId, …)` L227, `PipeTbl.AppId = AppId` L237 | 동일 파일 | 실패해도 `AppId`가 0xFFFFFFFF로 남아 표에 저장 | 없음 |
| Q16-C5-10 | `RunLoop` | 반환값 | `CFE_ES_RunLoop` L540, `GetAppIDInternal` L551, 실패 시 syslog "Cannot get AppID for the caller" **L658**, `FALSE` 반환 | L540, 실패 로그 L632 | `TRUE`/`FALSE` | 줄 이동 |
| Q16-C5-11 | victim 후보(첫 `GetAppID` 의존 호출) | — | `apps/sample_app/fsw/src/sample_app.c`: `CFE_ES_RegisterApp` L82, `CFE_EVS_Register` L87, `CFE_SB_CreatePipe` L95, `while (CFE_ES_RunLoop(&RunStatus) == TRUE)` L52 | 동일 파일(`9bd823b1809c`) | 반환값 확인 없음 | 없음. 이 앱이 보고자의 앱이라는 근거는 없다(`C5.md` A4) |
| Q16-C5-12 | `AppCreate`의 호출 경로 | 생성 문맥 | 시작 스크립트: `StartApplications` ← `CFE_ES_Main` L292, `ParseFileEntry`가 L387에서 호출; 명령: `cfe_es_task.c` L880; 재시작·재적재: `cfe_es_apps.c` L1098·L1134 | 같은 구조 | 시작 스크립트 앱의 생성자는 `CFE_ES_Main` 스레드(PSP가 SCHED_FIFO 최대로 올림, `cfe_psp_start.c` L352–L366 (기록)) | 없음 |

**Allowed 후보 레퍼런스(6.4.1)**: `CFE_ES_GetAppID`의 문서(위)는 `CFE_ES_ERR_APPID`를 반환 코드로 열거하나, 그것이 "등록 전의 task"의 정상 경로라는 문구는 없다. C5 (a)의 "문서화된 전파"를 Φ_6.4.x가 작성하려면 호출자 쪽 인용이 필요하다(SPEC §8.2·부록의 C5 항목). → `unspecified_by_reference`.

### 4.3 C3·C5의 관측 사건과 판정 분리 (2026-10-16 추가)

ROADMAP §9.2·§9.3의 `[제안]` 항목(관측 계획, 식별 방법, 판정과 파생 결과의 분리)에 대응하는 **설계**이다. 실행하지 않았다. 줄 번호는 6.4.1 기준이고 6.4.2 대응은 §4.1·§4.2의 표에 있다. 사건 이름은 이 문서의 작업 이름이며 SPEC의 용어가 아니다. 관측 도구(gdb 중단점, 프로브 등)와 보류 지점의 선택은 PI 몫이다(C3-D3, `ledger.md` X3). gdb를 쓰면 관측 영향 점검(PI-D1) 대상이다.

**C3** (준비 상태 전제는 §4.1):

| 사건 | 위치 (6.4.1) | 기록할 값 | 비용 |
|---|---|---|---|
| E3-SBPUB | `sb/cfe_sb_task.c` L198 `CFE_ES_GetAppID(&CFE_SB.AppId)` 반환 | `CFE_SB.AppId` 값과 반환값(무시되는 값) | 실행 |
| E3-SBREG | 같은 파일 L257 `CFE_EVS_Register` 반환 | SB의 `RegisterFlag` | 실행 |
| E3-EVSPUB | `evs/cfe_evs_task.c` L385 (6.4.2는 L436 전역 대입) | `EVS_AppID` 값 | 실행 |
| E3-USE | TIME 문맥의 `CFE_SB_CreatePipe`(`time/cfe_time_task.c` L308) 안에서 `sb/cfe_sb_api.c` L264 이벤트 송신 진입 | 호출 thread, `CFE_SB.AppId`·`EVS_AppID`의 그 시점 값, `RegisterFlag` | 실행 |
| E3-IDX | `evs/cfe_evs_utils.c` L244 (`EVS_IsFiltered`의 인덱스 계산) | `AppID` 인자. 0xFFFFFFFF이면 범위 밖 인덱스 사용(판정 단위 후보, `C3.md` C3-D4 (a)) | 실행 |
| E3-FAULT | 프로세스의 신호(SIGSEGV 등)·비정상 종료 | 신호와 위치. **파생 결과**로만 기록. fault가 없어도 E3-IDX는 남는다(`C3.md` A4: fault 여부는 `unspecified_by_reference`) | 실행 |
| E3-SCHED | 각 코어 task의 실제 정책·우선순위와 CPU 집합 | `geteuid()==0`일 때만 OSAL이 SCHED_FIFO·재매핑을 건다(`osapi.c` L435·L587). 실제 적용값은 실행 중 기록(U-RT). CPU 수·고정 여부는 `unspecified_by_reference`(§3.5) | 실행 |

- 해석 규칙[I]: ROADMAP §9.2 완료 기준대로 "잘못된 상태 사용"(E3-USE·E3-IDX)과 "실제 crash"(E3-FAULT)를 나누어 보고한다. 6.4.2에서 crash가 사라진 것만으로 순서 의존이 없어졌다고 쓰지 않는다: 6.4.2는 F3(Trac #40, 순서 보장)·F1·F2(Trac #42, 사용 지점 가드와 게시 순서)를 한 diff에 담고(`ledger.md` §3.3 L214–L216, L225), C5의 Trac #41도 같은 diff에 있다. 판 전체 비교의 결과에는 "여러 수정이 섞였다"를 표시한다. 하나만 떼는 것은 구성한 부분 diff이며 `deviation`이다(같은 곳).
- 현재 판(v7.0.1)의 R2 기록은 C3의 실행 근거로 세지 않는다(ROADMAP §9.2, SPEC rev 1.2 L2828 "Not independent C3 evidence").

**C5** (준비 상태 전제는 §4.2):

| 사건 | 위치 (6.4.1 / OSAL 4.1.1) | 기록할 값 | 비용 |
|---|---|---|---|
| E5-UNLOCK | `es/cfe_es_apps.c` L610 생성자의 락 해제 | 생성자 thread, 앱 슬롯 `i` | 실행 |
| E5-CREATE | L614 `OS_TaskCreate` 진입, OSAL `osapi.c` L645 `pthread_create` | 생성자 thread와 이 호출의 순번 | 실행 |
| E5-START | 새 thread의 첫 실행. OSAL은 ES가 넘긴 진입 주소(`(void*)StartAddr`, `cfe_es_apps.c` L616)를 `pthread_create`의 시작 함수로 그대로 넘긴다(`osapi.c` L645–L648) | 새 thread의 OS thread ID와, 그것을 만든 E5-CREATE. **`pthread_create`가 반환하기 전에 새 thread가 실행될 수 있다**는 것을 전제로 한다(ROADMAP §9.3 항목 2) | 실행 |
| E5-REG | 새 thread의 `CFE_ES_RegisterApp` → `OS_TaskRegister`(`cfe_es_api.c` L776) 반환 | 반환값 | 실행 |
| E5-USE | 새 thread의 첫 `CFE_ES_GetAppID`(L846): 락 획득 L851, `OS_TaskGetId` 결과, 검사 L861, 반환값 | `TaskId`, `RecordUsed`, 반환값(`CFE_SUCCESS`/`CFE_ES_ERR_APPID`) | 실행 |
| E5-RELOCK·PUB | 생성자의 재획득 L637, `RecordUsed = TRUE` L649, `AppId = i` L651, 해제 L673 | 게시 시점 | 실행 |
| E5-CONSUME | 실패한 `GetAppID`의 소비: `EVS_GetAppID`(`evs/cfe_evs_utils.c` L127)를 거친 등록 건너뜀, `CFE_SB_CreatePipe`의 반환 무시(`sb/cfe_sb_api.c` L224), `RunLoop` 실패 로그(`cfe_es_api.c` L658) | 각 경로의 결과. **파생 결과**로 기록 | 실행 |

- **식별 방법**[I] (ROADMAP §9.3 항목 3): 관측기는 새 thread를 그 thread 자신의 `GetAppID`로 식별하지 않는다. 그 호출이 바로 판정 대상이고 등록 전에는 실패하기 때문이다. 대신 (1) OS thread ID(Linux에서는 LWP ID)와 (2) 그 thread를 만든 E5-CREATE(생성자 thread, 호출 순번, 앱 슬롯 `i`, 앱 이름 인자)를 임시 식별로 쓰고, (3) E5-RELOCK·PUB에서 `TaskTable[TaskId].AppId = i`가 기록된 뒤 앱 정보와 연결한다. `TaskId`는 OSAL 표 인덱스이며 `OS_TaskGetId`가 `thread_key`의 값에서 하위 16비트로 계산한다(`osapi.c` L959–L972). 이 연결 방식은 설계 제안이며 아직 어떤 도구로도 시험하지 않았다.
- **판정 단위**[I]: 위반은 "E5-USE가 E5-RELOCK·PUB보다 먼저 일어나고 그 결과가 `CFE_ES_ERR_APPID`"인 것이다(§4.2 준비 상태 전제). E5-START가 E5-CREATE의 반환보다 먼저인 것만으로는 위반이 아니다(ROADMAP §9.3 완료 기준). `CFE_ES_GetAppID`가 락을 잡으므로(L851) 생성자가 L637–L673을 쥐고 있는 동안의 호출은 기다렸다가 성공한다[I, 코드 읽기]. 따라서 창은 L610(해제)부터 L637(재획득)까지이다. 6.4.2는 이 구간의 해제·재획득이 없다(§4.2 Q16-C5-02).
- **원 보고에 없는 결과를 기대하지 않는다**: 사례 기록이 보존한 #72 인용은 `GetAppID` 실패를 서술하고(`C5.md` §7 출처 1, 기록), `C5.md` 전체에 crash라는 낱말이 없다(grep 0건[M]; 이슈 원문과 댓글 9개는 이 세션에서 다시 읽지 못함). crash는 기대 결과에 넣지 않는다(ROADMAP §9.3 항목 5).
- 현재 판의 App-record 잔여 후보(SPEC 부록 A.3)와 `58abd38d^`/`58abd38d` 쌍(§5)은 #72와 별도 결과로 관리한다.

비용: 이 절의 설계는 읽기. 사건 기록은 6.4.x 실행 기반(§3.5)이 생긴 뒤의 실행이며, 권한·`msg_max`·CPU 고정 조건이 붙는다.

**시험 가능성**: 위 각 행은 (파일, 정규식 앵커) 쌍으로 기계 검증할 수 있다(부록 A.5가 앵커 정규식을 가진다). 행 하나가 6.4.1과 6.4.2에서 모두 한 줄에 맞으면 버전 대응이 성립한 것이다. 대응 실패는 기법의 실패가 아니라 입력·대응 실패로 센다[D].

비용: 읽기. 기계 검증은 읽기+스크립트(설치 없음).

## 5. 보조 쌍 `58abd38d^`/`58abd38d` (P-RGIT) — #72와 따로 보고

**결정[D]**: 보조 대조로 승인. 원 C5(#72)의 대체 검증으로 쓰지 않는다. 원 C5의 과거 버전 락 공백 검증을 먼저 하고, 이 쌍은 추가 입력 대응 비용이 확인된 뒤 수행한다. 설계 문서에 이미 알려진 쌍이므로 **맹검 결과로 보고하지 않는다**. 기대(SPEC §9.3 VP-D5): `58abd38d^`에서 PROTECTED 증명서가 나타나고 `58abd38d`에서 보류된다[D, 미실행]. 쌍의 조건은 모두 `unspecified_by_reference`이다(PI_DECISIONS).

### 5.1 쌍의 정체 [M]

| 항목 | 값 |
|---|---|
| `58abd38d` | `58abd38d04390cbe123d89b57819af013464d57f`. 저자 Joseph Hickey, 저자 일시 2021-02-02 22:55:09 -0500, 커밋 일시 2021-02-16 14:59:14 -0500. 제목 "Fix #978, store task parameters in task record" |
| `58abd38d^` | `6b9c267e6ac539ae17fb79a94467928f44559844` (nasa/cFE PR #1150 "integration-candidate" 병합, 2021-02-16 11:47:25 -0500). 이 병합 커밋의 부모는 둘이다(`fcaa1d0c`, `557d92b6`). 반면 Fix #978 커밋 `58abd38d`의 부모는 하나이며 그것이 `6b9c267e`이다[M: `git cat-file -p`] |
| 변경 파일 | `fsw/cfe-core/src/es/` `cfe_es_api.c`·`cfe_es_apps.c`·`cfe_es_apps.h`·`cfe_es_objtab.c`·`cfe_es_resource.h`·`cfe_es_start.c`·`cfe_es_start.h`·`cfe_es_task.c`, `inc/cfe_es.h`, `unit-test/es_UT.c` |
| 커밋 메시지의 관련 문장 | "Fixes a race condition where child tasks may not be fully instantiated at the time the task function is invoked." |

### 5.2 두 커밋에서 읽은 변경점 [F]

`fsw/cfe-core/src/es/cfe_es_apps.c` 기준 줄 번호(고정 SHA에서 가져옴; 해시는 부록 A.4).

| 단계 | `58abd38d^` (`6b9c267e`) | `58abd38d` |
|---|---|---|
| 새 task의 진입 함수 | `CFE_ES_AppEntryPoint` L532 → `CFE_ES_GetAppEntryPoint` L483–L519 | `CFE_ES_TaskEntryPoint` L539 → `CFE_ES_GetTaskFunction` L477–L526 |
| **대기 조건** | 락 안에서 `CFE_ES_GetAppRecordByContext() != NULL`(L500): **태스크 레코드와 확정된 앱 레코드 모두** 필요(`cfe_es_resource.c` L323–L356, `AppRecordIsMatch` L345). 성공하면 `AppState = EARLY_INIT` 설정(L503) | 락 안에서 `CFE_ES_GetTaskRecordByContext()`(L498)의 `AppId`가 정의되고 `EntryFunc != 0`(L503): **태스크 레코드만** 필요. 앱 상태 설정 줄 없음 |
| 대기 방식 | `OS_TaskDelay(CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC)` 폴링, 한도 `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`(L489–L516) | 같음(L485–L517) |
| task 생성 | `CFE_ES_StartAppTask` L559: `OS_TaskCreate` **L570**(진입 `CFE_ES_AppEntryPoint` L572), 락 L578, `CFE_ES_TaskRecordSetUsed` L604, 해제 L621 | `CFE_ES_StartAppTask` L566: `OS_TaskCreate` **L577**(진입 `CFE_ES_TaskEntryPoint` L579), 락 L585, `TaskRecordSetUsed` L615, 해제 L632 |
| 앱 레코드 확정 | `CFE_ES_AppCreate` L637: 락 L666, `AppRecordSetUsed(RESERVED)` L736, 해제 L742, `StartAppTask` 호출 L763, **재획득 L774**, `AppRecordSetUsed(PendingResourceId)` **L783**, 해제 L799 | `CFE_ES_AppCreate` L648: 락 L671, `RESERVED` L733, 해제 L739, `StartAppTask` 호출 L760, **재획득 L772**, `AppRecordSetUsed` **L780**, 해제 L796 |
| `GetAppID` | `cfe_es_api.c` L812–L836: `CFE_ES_GetAppRecordByContext()`가 NULL이면 `CFE_ES_ERR_RESOURCEID_NOT_VALID`(L829) | 같음 |

[I] `58abd38d^`에서는 새 task의 진입 대기가 **앱 레코드 확정**(L783)까지 이어지므로, 앱의 진입 함수 안에서 `GetAppID`가 실패할 창이 닫혀 있다. `58abd38d`에서는 대기가 태스크 레코드 게시(L615)에서 끝나므로 L615–L780 사이에 `GetAppID`가 `CFE_ES_ERR_RESOURCEID_NOT_VALID`를 반환하는 창이 생길 수 있다. 이는 C5 기록 §9의 잔여 창 후보와 같은 줄거리이며 코드 읽기만의 추론이다. 실행으로 도달 여부를 확인한 적이 없다.

### 5.3 입력 대응 비용의 목록 (PI가 요구한 "비용 확인")

| 항목 | 내용 | 비용 |
|---|---|---|
| 소스 쌍 | 위 두 커밋의 `fsw/cfe-core/src/es/*`를 가져옴. 파일 해시는 부록 A.4 | 읽기(완료) |
| cFE·OSAL·PSP 조합 | 공개 번들 nasa/cFS `a41549cb201b8d53b9e5aea81b1e7bb660e6cb8b`("Integration Candidate 2021-02-02", 2021-02-16)가 **cfe = `6b9c267e`**(`58abd38d^`와 같음), osal = `ff4f52312297a3cdf3ddc0cf5ead95f27954630b`(nasa/osal 태그 `v6.0.0-rc1`), psp = `a1a7e7a3bfd6d2d82bb5d099c1f757ff60ba2431`(2021-02-16 병합)을 고정한다[M: 신규 얕은 fetch로 확인]. `58abd38d` 자체를 고정한 번들 커밋은 확인하지 못했다. 번들 `8d0f9dc3ce9ce83ebd90da5a489094f5b497ed94`("Integration Candidate 2021-02-23")는 cfe `672b2dc4`(PR #1171 병합, `58abd38d`를 조상으로 포함[M])를 고정한다 | 읽기. 이 조합을 쓰는 것은 실험 선택이며 `unspecified_by_reference` |
| 빌드 방식 | 이 시점의 cFS 번들은 CMake 기반(`fsw/cfe-core/…`는 이 커밋에서 아직 `modules/` 이전 구조). 컴파일 DB는 CMake의 `CMAKE_EXPORT_COMPILE_COMMANDS`로 얻을 수 있다고 추정[I]. 미시험 | 빌드(설치 없음 추정) |
| Φ 입력 | 이 쌍의 API(`CFE_ES_GetAppRecordByContext`, `CFE_ES_TaskRecordSetUsed`, `CFE_ES_AppRecordSetUsed`, `CFE_ES_APPID_UNDEFINED`)는 Φ_6.4.x에 없다. 별도 버전 가드·결과 맵이 필요 | 읽기(Φ 작성) |
| 관측 | 앱 레코드 확정(L783/L780)과 `GetAppID`의 반환을 사건으로 기록해야 하며, 생성자를 `StartAppTask` 반환 직후·재획득 직전에 보류하는 사건 기반 제어가 필요하다(SPEC 부록 A.3 확인 4와 같은 위치) | 실행 |
| 보고 | #72의 결과와 합산하지 않는다. "맹검 아님" 표시. D16(App-record 보류 실험)은 파생 실험으로 따로 표시 | — |

## 6. 항목별 비용 총괄 — 설치 / 플랫폼 / 읽기

| 항목 | 읽기 | 파싱 | 빌드 | 실행 | 설치 | 플랫폼 | 메모 |
|---|:-:|:-:|:-:|:-:|:-:|:-:|---|
| C2 코드 비교(§2.2) | ● | | | | | | 완료 |
| C2 역할·2×2 설계(§2.3–§2.6) | ● | | | | | | 결정 대기 |
| C2 셀 C·D (ENV 위) | | | ● | ● | | | 새 설치 없음 추정[I]. 관측은 gdb 있음 |
| C2 셀 A (v6.5.0a) | ● | | ● | ● | | | OSAL·PSP 확보는 가져오기(설치 아님). CMake 3.28.3 호환은 미시험 |
| C2 셀 A (6.4.2) | ● | ● | ● | ● | | | D13과 같은 6.4.x 플랫폼. 큐 깊이 조건(§3.5) |
| C2 셀 B | 제외(심볼 부재) | | | | | | |
| C3·C5 tarball 확보·해시(§3.1) | ● | | | | | | 완료 |
| 컴파일 DB 재구성·파싱(§3.2–§3.3) | ● | ● | | | | | 완료: 6.4.1·6.4.2 각 58 TU(코어 33·OSAL 6·PSP 14·앱 5). 단위 시험·링크는 미포함 |
| C3·C5 관측 사건 설계(§4.3) | ● | | | | | | 설계 완료. 기록은 6.4.x 실행 이후 |
| C3·C5 사건 기록 | | | ● | ● | | (●) | §3.5의 권한·`msg_max`·CPU 고정 조건 |
| 헤더 민감도 (ii) CentOS 6.5 헤더 | ● | ● | | | | | 압축 해제만, 설치 아님. 헤더 3종만 완료. 전체 sysroot·gcc 4.4.7의 실행은 미시험[U] |
| 목록 host 자체 재현(CentOS 6.5-x86 등) | | | | (●) | (●) | (●) | 시험하지 않음[U] (§3.3.1). 사용자 영역 파일은 가져와지고 정적 의존이 닫혔으나 실행·이미지·VM은 미시험이다. 필요한 비용 분류도 확정하지 않았다 |
| Microblaze (C3 원 플랫폼) | | | | | | ● | OS·PSP·OSAL·툴체인이 레퍼런스에 없음. C3의 파생 기반은 `deviation`, C5(#72)의 플랫폼은 `unspecified_by_reference`(§3.3.2) |
| 6.4.1/6.4.2 pc-linux 실행(§3.5) | | | ● | ● | (●) | (●) | root 큐 제한 때문에 `msg_max` 상향 또는 권한이 필요할 수 있음 |
| Q16 표 골격·앵커(§4) | ● | | | | | | golden 시험 코드는 이 문서의 범위 밖 |
| P-RGIT 소스 대응(§5) | ● | | | | | | 완료 |
| P-RGIT 파생 실행 | | | ● | ● | | | 비용 확인 뒤 수행(PI) |

● = 필요. (●) = 필요할 수 있음(미시험).

## 7. 남은 항목

**PI 결정이 필요한 것**

1. **P-C2 세부**: 단일 단계 쪽 개정(6.4.2 대 v6.5.0a)과 2단계 쪽(v6.6.0a 대 v7.0.1), 6.5.0a·6.6.0a configure 결정, 요청 방식(C2-D4), 앱 수·우선순위·timeout 인자, 어느 대조를 RQ9에 세는가(§2.4, §2.6). 추가로 두 가지를 PI가 확인해야 한다.
   - **2×2의 실현 가능성**: PI 결정문 §3.5는 "교차하는 2×2", SPEC §8.2·§9.3은 provider 축을 "on the ENV cFE"로 서술한다. 셀 B(단일 장벽 × 새 provider)는 새 provider가 부르는 심볼이 6.4.2·v6.5.0a에 없어 구성할 수 없다(§2.4 셀 B; grep 0건 [M], 이번에 재확인). 서술된 교차 2×2는 그대로는 실현되지 않는다. 3셀(A·C·D)과 B의 제외 사유를 문서화하는 것으로 충분한지, 아니면 B를 별도로 정의할지(예: 심볼을 추가한 변형은 "다른 코드"이므로 별도 정의 필요) 확인이 필요하다.
   - **판정 단위(oracle)**: `E-P2` 대 `E-R2`의 선후(`C2.md` A4)는 미승인 가정이다. `ledger.md` X7(사례 전반의 주 결과 정책)과 함께 정한다(§2.6).
2. **X3 (D13 (c)의 후속)**: (c)는 "F1 전에 배치"로 이미 결정되었다(PI_DECISIONS_RESOLVED §3.4)[D]. 남은 PI 몫은 `ledger.md` §5 X3의 항목이다: 공유 플랫폼의 OSAL artifact 배치(SourceForge 파일을 기준 artifact로 쓰는 안은 §3.1), 호스트(§3.3.1: 목록 host 재구성은 미시험), 권한, 순서 제어 방식. 그리고 기반을 F1 전에 두는 비용(§6의 실행 열: `msg_max`·권한·순서 제어, 목록 host 재현 여부).
3. **(b)(i) 민감도 판정**: `sched_priority` 멤버 이름 별칭을 민감성으로 볼지, 보류 대상(§3.3 영향 후보)을 확정할지, (ii)로 확대할지. (ii)는 PI_DECISIONS D13이 "obtaining the literature host's sysroot (an install decision)"(영문 원문)로 서술하므로 그 문구대로 PI가 정한다. 이 문서는 헤더 추출만 시험했고 전체 sysroot·gcc의 비용은 미시험이다[U].
4. **6.4.x 실행 조건**: 깊이 10 초과 큐에 대한 `msg_max` 상향 또는 `CAP_SYS_RESOURCE` 부여(§3.5). 호스트 공유 sysctl이라 시험하지 않았다.
5. **P-RGIT 수행 시점**: 입력 대응 비용(§5.3)을 본 뒤.
6. **목록 host 재구성 시험의 허용 여부**: 데몬 기동·이미지 pull·내려받은 외부 바이너리 실행을 허용할지(§3.3.1). 허용되기 전까지 §3.3 빌드 host 행의 불가피성은 [U]로 남는다.
7. **C2 보류 감시 한도**: C1의 P-C1HOLD와 같은 원칙(중단 조건·감시 한도를 본실험 전에 기록)을 C2에 적용할지(§2.6).
8. **C3·C5 관측 방식**: §4.3 사건을 어떤 도구로 기록할지(gdb 중단점은 관측 영향 점검 PI-D1 대상), C3의 순서 제어(우선순위 대 사건 기반 보류, C3-D3), C3의 주 판정 단위(C3-D4 (a)/(b); 사례 전반은 `ledger.md` X7). 6.4.2 diff에 섞인 Trac #40·#41·#42를 떼어 보는 부분 diff는 `deviation`이며 계획에 없다(SPEC rev 1.2 L2829의 C5 행: PI가 D13에 추가할 때만).
9. **이슈 원문 접근(ledger X1)**: nasa/cFE 이슈 페이지와 API가 이 세션에서 403이다(§2.1). 원문·댓글을 다시 읽으려면 사용자가 이 저장소의 GitHub 접근을 세션에 추가해야 한다.

**이 문서가 확인하지 못한 것**

- 이슈 #198·#72·#73의 페이지와 댓글(C3 14개 중 4개 미식별, C5 9개, C2는 댓글 없음)은 이 세션에서 읽지 못했다(GitHub 403, ledger X1). 보고 서술은 사례 기록의 인용에 근거한다.
- 6.4.2 VDD, 6.4.1 VDD의 본문은 다시 읽지 않았다(기록). 6.6.0 VDD의 Trac #167 행만 재확인했다.
- 6.4.1·6.4.2 번역 단위 중 단위 시험·링크는 파싱하지 않았다(앱 5개와 6.4.2 번역 단위는 2026-10-16에 파싱함, §3.3).
- §4.3의 식별 방법(OS thread ID + 생성 관계 → 나중의 `AppId` 연결)은 어떤 관측 도구로도 시험하지 않은 설계이다.
- 목록 host의 gcc 버전·기본 C 표준은 레퍼런스가 정하지 않는다. CentOS 6.5 GA의 `gcc-4.4.7-4.el6` RPM man 페이지에서 `gnu89`가 C의 기본이라는 문구를 읽었으나(§3.3), 목록 host가 그 gcc였는지는 [I]이다.
- 풀어낸 CentOS 6.5 사용자 영역(gcc 4.4.7 포함)의 **실행** 시험, 컨테이너 데몬·이미지 경로, QEMU·VM 경로(§3.3.1).
- C3의 순서 제어(우선순위 대 사건 기반 보류), 주 oracle(C3-D4; 사례 전반은 ledger X7), C3와 R2를 한 패턴으로 세는지(ledger X4), 현재 R2 기록을 C3 증거로 세지 않는다는 원칙은 이 문서가 건드리지 않았다.

**기존 기록과의 불일치 후보**

- C3.md A2·§3의 "git 태그 `osal-4.1.1`과 SourceForge tarball의 동일성 미확인"은 C5가 해소했다(`osapi-version.h`만 다름). C3 쪽 서술을 맞춰야 한다(`ledger.md` §6 이미 지적).
- C5.md §4.3의 "`94362464…`"는 sha256이 아니라 git blob SHA-1이다(§3.1). 혼동 방지용 표기 정정을 제안한다.

## 부록 A. 재현 명령과 해시

모든 명령은 읽기 전용이거나 새 디렉터리에 쓴다. `$WORK`는 비어 있는 새 디렉터리이다. 이 부록은 스크래치 경로를 쓰지 않는다.

### A.1 내려받기와 해시

```sh
cd "$WORK" && mkdir dl && cd dl
curl -sS -L --max-time 240 -o cFE-6.4.1.tgz "https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.1/cFE-6.4.1-OSS-release.tar.gz/download"
curl -sS -L --max-time 240 -o cFE-6.4.2.tgz "https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.2-OSS-release.tar.gz/download"
curl -sS -L --max-time 240 -o osal-4.1.1.tgz "https://sourceforge.net/projects/osal/files/OSAL-4.1.1/osal-4.1.1-release.tar.gz/download"
sha256sum *.tgz     # §3.1 표와 대조
cd .. && mkdir x641 x642 xosal
tar -xzf dl/cFE-6.4.1.tgz -C x641 && tar -xzf dl/cFE-6.4.2.tgz -C x642 && tar -xzf dl/osal-4.1.1.tgz -C xosal
```

문헌상 host 헤더(프로브용, 설치 아님): `https://vault.centos.org/6.5/os/i386/Packages/` 의 `glibc-headers-2.12-1.132.el6.i686.rpm`·`glibc-devel-2.12-1.132.el6.i686.rpm`·`kernel-headers-2.6.32-431.el6.i686.rpm`. rpm2cpio가 없어서 RPM 머리와 cpio(newc) 페이로드를 직접 풀었다(파이썬 `gzip`·`lzma` 모듈, 새 디렉터리 안에서만 쓰기). sha256: `989ed41829ed46d2b2f2df6c7073ff3ade987c94d7d0c1e0911a908288b03fd3`(headers), `6d0b2cbf484d8e4f72202387852c342b932174c10eeade4589505755e1ffef0a`(devel), `216ee716b0449fd758825db880e6e5461d8268d8db5f016f3261797dc1c4ab93`(kernel-headers). gcc·binutils 등 나머지 RPM과 그 sha256은 A.6이다. 이 헤더 3개의 sha256은 이번 검토 반영에서 다시 내려받아 같은 값을 확인했다[M].

### A.2 컴파일 DB 재구성(요약) — make를 실행하지 않는다

```python
# python3 -I gen_cdb.py <WORK>   (Makefile·*.mak를 읽고 파일 복사만 한다)
import sys, os, re, json, shutil, glob
W=sys.argv[1]; A=W+"/x641/cFE-6.4.1-OSS-release"; OSAL=W+"/xosal/osal-4.1.1-release"
GI=W+"/gen/cpu1_inc"; GM=W+"/gen/mission_inc"; os.makedirs(GI,exist_ok=True); os.makedirs(GM,exist_ok=True)
CORE=A+"/cfe/fsw/cfe-core/src"; PSPS=A+"/psp/fsw"; OSS=OSAL+"/src/os"
for f in glob.glob(A+"/cfe/fsw/mission_inc/*.h")+glob.glob(A+"/apps/inc/*.h"): shutil.copy2(f,GM)   # cfs_copy_core_incs / app_shared_incs
for f in glob.glob(A+"/cfe/fsw/platform_inc/cpu1/*.h")+[OSAL+"/build/inc/osconfig.h"]: shutil.copy2(f,GI)
COPTS=["-D_EL","-DENDIAN=_EL","-DSOFTWARE_LITTLE_BIT_ORDER","-D__ix86__","-D_ix86_","-D_LINUX_OS_","-Dposix","-DX86PC",
       "-DBUILD=","-D_REENTRANT","-D_EMBED_","-m32","-Wall","-Wstrict-prototypes","-g","-O0","-DOS_DEBUG_LEVEL=3"]
def objs(mak):                      # OBJS = a.o b.o \ ...  ->  a.c b.c
    t=open(mak).read().replace("\\\n"," "); m=re.search(r"^\s*OBJS\s*=\s*(.*)$",t,re.M)
    return [o[:-2]+".c" for o in m.group(1).split() if o.endswith(".o")]
def I(*p): return ["-I"+x for x in p]
ents=[]
for s in ["es","sb","evs","tbl","time","fs"]:
    inc=[OSS+"/inc",CORE+"/"+s]+([CORE+"/es"] if s in("evs","time") else [])+[CORE+"/inc",PSPS+"/inc",PSPS+"/pc-linux/inc",GM,GI]
    for n in objs(f"{CORE}/{s}/cfe_{s}.mak"): ents.append(["gcc"]+I(*inc)+COPTS+["-c",f"{CORE}/{s}/{n}"])
inc=[OSS+"/inc",CORE+"/inc",OSS+"/posix",GI]
for n in objs(OSS+"/posix/osal.mak"): ents.append(["gcc"]+I(*inc)+COPTS+["-c",f"{OSS}/posix/{n}"])
t=open(PSPS+"/pc-linux/src/psp.mak").read().replace("\\\n"," "); src=re.search(r"^\s*SOURCES\s*=\s*(.*)$",t,re.M).group(1).split()
inc=[OSS+"/inc",CORE+"/inc",PSPS+"/inc",PSPS+"/pc-linux/inc",GM,GI]
for n in src:
    d=PSPS+"/pc-linux/src" if os.path.exists(PSPS+"/pc-linux/src/"+n) else PSPS+"/shared"
    ents.append(["gcc"]+I(*inc)+COPTS+["-c",f"{d}/{n}"])
json.dump(ents,open(W+"/compile_args_641.json","w"),indent=1); print(len(ents),"TU")   # 53
```

### A.3 구문 분석 프로브

```sh
# 각 항목에서 'gcc'를 컴파일러로 바꾸고 '-c <src>'를 '-fsyntax-only <src>'로 바꾼다. 작업 디렉터리는 새 빈 디렉터리.
clang       -fsyntax-only ...                              # 기본: 2 오류 (osapi.c:647, cfe_psp_start.c:270)
clang       -Wno-error=incompatible-function-pointer-types -fsyntax-only ...   # 53/53, 0 오류, 20 경고
clang -std=gnu89 -Wno-error=incompatible-function-pointer-types -fsyntax-only ... # 동일
gcc         -fsyntax-only ...                              # 53/53, 22 경고
# -m32를 뺀 clang                                           # 53개 모두 실패 (common_types.h:256)
# CentOS 6.5 헤더: clang … -nostdinc -isystem /usr/lib/llvm-18/lib/clang/18/include -isystem <압축 해제한 usr/include> -fsyntax-only ...
# 전처리 토큰열 비교: <compiler> … -E 의 출력에서 '# N "file"' 줄표시를 따라 프로젝트 파일의 줄만 모아 공백을 지우고 이어 붙여 비교
# 헤더 닫힘: clang … -M       구조체 크기: clang -m32 -fsyntax-only -Xclang -ast-dump (char s_x[sizeof(T)] 선언을 읽음)
```

컨테이너 환경 [M, 2026-10-15]: Ubuntu 24.04.4 LTS, 커널 6.18.44, gcc 13.3.0, clang 18.1.3, cmake 3.28.3, make 4.3, python 3.11.15, gdb 15.1, glibc 2.39-0ubuntu8.9, `libc6-dev-i386`·`libc6-i386` 2.39, `nproc` 4, uid 0, `fs.mqueue.msg_max` 10.

### A.4 읽은 파일의 sha256 앞 16자리

| 개정 | 파일 | sha256[:16] |
|---|---|---|
| 6.4.1 | `cfe/fsw/cfe-core/src/es/cfe_es_apps.c` / `cfe_es_api.c` / `cfe_es_start.c` | `0a54974ba77af3db` / `0cb4c1c740f897a3` / `d36b486d093f5372` |
| 6.4.1 | `…/sb/cfe_sb_task.c` / `…/evs/cfe_evs_task.c` / `…/evs/cfe_evs_utils.c` / `…/time/cfe_time_task.c` | `89bafa8827cb1b4f` / `28ad8edde34f2c90` / `7ca920717e407f06` / `0a4e1eb8e6a3d453` |
| 6.4.2 | `…/es/cfe_es_apps.c` / `cfe_es_api.c` / `cfe_es_start.c` | `2a1e2f4ce32d05a9` / `8a67d0502452c0c6` / `d145d294f4203e06` |
| 6.4.2 | `…/sb/cfe_sb_task.c` / `…/evs/cfe_evs_task.c` / `…/evs/cfe_evs_utils.c` / `…/time/cfe_time_task.c` | `a7e740e401008e9c` / `c0fdca8367ead863` / `be6793ce959ed36e` / `dffa375ae272bde2` |
| 6.4.1 / 6.4.2 동일 | `es/cfe_es_objtab.c` / `sb/cfe_sb_api.c` / `evs/cfe_evs.c` | `a9da09fabd8317e2` / `a969462dbe06e2aa` / `22bf2c12f7a2fe78` |
| OSAL 4.1.1 | `src/os/posix/osapi.c` / `src/os/inc/common_types.h` / `build/inc/osconfig.h` | `32649c6f1cf3b586` / `69a2989805f4638c` / `086d06c0da44e121` |
| v6.5.0a `b2765d9f` | `cfe/fsw/cfe-core/src/es/cfe_es_api.c` / `cfe_es_start.c` / `cfe_es_apps.c` / `cfe_es_apps.h`, `…/inc/cfe_es.h`, `cfe/cmake/sample_defs/cpu1_platform_cfg.h` | `3d3ba6b3e84ec1d8` / `f5d89f3f25774a49` / `97f03cfd78a8945e` / `dcd513bcf054ae16`, `61d06bab4605d5ca`, `584a64076104c5c1` |
| v6.6.0a `8811c920` | `fsw/cfe-core/src/es/cfe_es_api.c` / `cfe_es_start.c` / `cfe_es_apps.c`, `…/inc/cfe_es.h` / `cfe_es_extern_typedefs.h`, `cmake/sample_defs/cpu1_platform_cfg.h` | `46fd0c13190518c2` / `a36184b35684b08f` / `190cd0eb532bc5d1`, `31f9c95fbb4920a1` / `f357ce0274cb2dba`, `f2c7709a4152c6d2` |
| v7.0.1 `c5fb2b4d` | `modules/es/fsw/src/cfe_es_api.c` / `cfe_es_start.c` / `cfe_es_apps.c`, `modules/core_api/fsw/inc/cfe_es.h`, `modules/es/config/default_cfe_es_extern_typedefs.h`, `cmake/sample_defs/example_platform_cfg.h` | `e1b3bcdb3fd884e2` / `2a5b72788177282c` / `8fa28cd9748d1e9d`, `0ee6a236b9dd1b44`, `c2653c5eb86e6216`, `aeceefdaa0cfd5aa` |
| v7.0.1 `c5fb2b4d` (ENV가 컴파일하는 모듈 기본값, `env/CONDITIONS.md` C8) | `modules/es/fsw/inc/cfe_es_internal_cfg.h` / `modules/core_private/config/default_cfe_core_private_internal_cfg.h` | `5bfb645c2ca73782` / `ab509cedcfc2ca91` |
| `58abd38d^` (`6b9c267e`) | `fsw/cfe-core/src/es/cfe_es_api.c` / `cfe_es_apps.c` / `cfe_es_resource.c` (§5.2가 인용하는 3파일; 이 쌍에서 해시를 적은 파일은 이것이 전부이다) | `f64d3cc75faa16ee` / `4620510e2048c555` / `90372af17eb6a12e` |
| `58abd38d` | 같은 경로 | `f9fb412391d38571` / `adda7298c9fe0db8` / `90372af17eb6a12e` |

### A.5 Q16 앵커 정규식(발췌)

각 앵커는 `(파일, 정규식)`이며 6.4.1과 6.4.2 각각에서 줄 번호를 돌려준다. 경로는 `cfe/fsw/cfe-core/src/` 아래이다.

| 앵커 | 파일 | 정규식 |
|---|---|---|
| SB AppId 게시 | `sb/cfe_sb_task.c` | `CFE_ES_GetAppID\(&CFE_SB\.AppId\)` |
| EVS AppId 전역 대입(F2) | `evs/cfe_evs_task.c` | `CFE_EVS_GlobalData\.EVS_AppID = AppID;` |
| 무검사 인덱스 | `evs/cfe_evs_utils.c` | `AppDataPtr = &CFE_EVS_GlobalData\.AppData\[AppID\];` |
| F1 가드 | `evs/cfe_evs_utils.c` | `EVS_AppID < CFE_ES_MAX_APPLICATIONS` |
| F3 ES 대기 | `es/cfe_es_start.c` | `CFE_ES_ApplicationSyncDelay\(CFE_ES_SYSTEM_STATE_UNDEFINED, CFE_CORE_MAX_STARTUP_MSEC\)` |
| 코어 task 생성 | `es/cfe_es_start.c` | `ReturnCode = OS_TaskCreate\(` |
| 앱 task 생성(C5) | `es/cfe_es_apps.c` | `ReturnCode = OS_TaskCreate\(` |
| `GetAppID` 검사 | `es/cfe_es_api.c` | `TaskTable\[TaskId\]\.RecordUsed == TRUE` |
| OSAL 생성(OSAL artifact 루트 기준 `src/os/posix/osapi.c`) | 위 경로 규칙의 예외 | `pthread_create\(` / `\*task_id = possible_taskid` / `geteuid\(\) == 0` |

### A.6 문헌상 host 재구성 가능성 프로브(§3.3.1)의 재현

모두 읽기 전용이거나 새 빈 디렉터리에만 쓴다. 내려받은 바이너리는 실행하지 않는다.

```sh
# 런타임·도구 존재 확인
command -v docker podman qemu-i386 qemu-system-i386 qemu-system-x86_64 proot bwrap debootstrap rpm rpm2cpio cpio yum dnf chroot unshare
docker version; docker info      # 클라이언트만 응답: "Cannot connect to the Docker daemon at unix:///var/run/docker.sock"
ls /dev/kvm                      # 없음
# 레지스트리 도달성(HEAD만; 이미지는 조회·pull하지 않는다)
for u in https://registry-1.docker.io/v2/ https://quay.io/v2/ https://ghcr.io/v2/; do curl -sS -o /dev/null -I --max-time 20 -w "%{http_code}\n" "$u"; done   # 401 401 405
# 패키지 색인과 RPM 가져오기(새 디렉터리)
B=https://vault.centos.org/6.5/os/i386/Packages
curl -sS -o idx.html "$B/"
for f in <아래 표의 파일명>; do curl -sS --max-time 120 -o "$f" "$B/$f"; done
sha256sum *.rpm                  # 아래 표와 대조
# 페이로드 풀기(파이썬 -I, 새 디렉터리에만 쓰기): RPM 리드 96바이트 → 서명 헤더 → 주 헤더(각 8바이트 경계) → 압축된 cpio(newc).
#   cpio newc는 하드링크 파일의 데이터를 마지막 링크에만 두므로(예: gcc와 i686-redhat-linux-gcc) 이를 처리해야 0바이트 파일이 생기지 않는다.
# 정적 의존 점검(실행 없음)
readelf -h -l -d <풀어낸 트리>/usr/bin/gcc <풀어낸 트리>/usr/libexec/gcc/i686-redhat-linux/4.4.7/cc1 …   # ELF32, 인터프리터, DT_NEEDED
strings <풀어낸 트리>/usr/libexec/gcc/i686-redhat-linux/4.4.7/cc1 | grep -m1 'Red Hat'
zcat <풀어낸 트리>/usr/share/man/man1/gcc.1.gz | grep -n -B1 -A2 'gnu89'   # "… This is the default for C code."
```

| RPM (`vault.centos.org/6.5/os/i386/Packages/`) | sha256 |
|---|---|
| `bash-4.1.2-15.el6_4.i686.rpm` | `81bc62e6d2396a462ea898f2c91c97578ad2d744af4588686602ffc3bec47420` |
| `binutils-2.20.51.0.2-5.36.el6.i686.rpm` | `b520e5ac9244021fd9838cf27419d4f445de4a7e2f4729d7afc3d4e2434b61c7` |
| `cloog-ppl-0.15.7-1.2.el6.i686.rpm` | `8ffff07954a85469563407291fc97c284a3e498dd1c8ddb5cb7d5a4a6e94dee6` |
| `coreutils-8.4-31.el6.i686.rpm` | `2fc78a19ac357050da7874150851c9c5dc3997e2d26d6e05de05b5bf49a86987` |
| `cpp-4.4.7-4.el6.i686.rpm` | `b03d35b8ef0da6e6a9ba662fe3c5e127684461c496b192ede835d823c994cc69` |
| `gcc-4.4.7-4.el6.i686.rpm` | `347e4ab6e3cdc91da738ac148240e1dc164150d302f07d6640137cdf07f81fb4` |
| `glibc-2.12-1.132.el6.i686.rpm` | `f24c8a14fdf9022b2545f00df6ac2f44356c85dde8ab0039f6fb6aaba7aae5cf` |
| `glibc-common-2.12-1.132.el6.i686.rpm` | `770f5259fd5f8a850e6276ef6fbe4b59591b22d072b7306a8e4801e7dc1ba9f4` |
| `glibc-devel-2.12-1.132.el6.i686.rpm` | `6d0b2cbf484d8e4f72202387852c342b932174c10eeade4589505755e1ffef0a` |
| `glibc-headers-2.12-1.132.el6.i686.rpm` | `989ed41829ed46d2b2f2df6c7073ff3ade987c94d7d0c1e0911a908288b03fd3` |
| `gmp-4.3.1-7.el6_2.2.i686.rpm` | `f1855126a943ed4aac412006af98490248b739b4f9f78487f17c3275557948e2` |
| `kernel-headers-2.6.32-431.el6.i686.rpm` | `216ee716b0449fd758825db880e6e5461d8268d8db5f016f3261797dc1c4ab93` |
| `libgcc-4.4.7-4.el6.i686.rpm` | `4f4621a86bca9873d8676182f5d36e7edd1f3ac1e393ed43ffc17cd8b54dd024` |
| `libgomp-4.4.7-4.el6.i686.rpm` | `9114c827c055cdd2e64588cc3317aee9b1303665ca57d3542e03362f0e59c3fb` |
| `make-3.81-20.el6.i686.rpm` | `d7e90e0b37964d8757fae155419f6c1c386849730f9e4a07fe51b8a125b554d1` |
| `mpfr-2.4.1-6.el6.i686.rpm` | `e732d3292785d1590841b569af10536fa5d99bccb7ade7079c955cb35b02a192` |
| `ncurses-libs-5.7-3.20090208.el6.i686.rpm` | `8e8c71cf6e1b45a592bf5705714e34c167b6e2b9e9b4cf4b6c8e41f6ebd5a392` |
| `ppl-0.10.2-11.el6.i686.rpm` | `64d7641fc2b85314708776774c4c1792608328a7bbdefbb5ebbed55ee99af7d8` |
| `zlib-1.2.3-29.el6.i686.rpm` | `1e40dce8a497f740b22d2035a76a92a34885613652c4b33262f04e2d12a00caa` |

실행 시험은 하지 못했다(§3.3.1 마지막 행). 허용되면 시험할 내용은 다음과 같다: (1) 풀어낸 트리의 `ld-2.12.so --version`, (2) 풀어낸 트리로의 `chroot`에서 `gcc --version`, (3) 같은 53개 번역 단위의 `-fsyntax-only`. 아무것도 시험하지 않았으므로 결과를 추정으로 적지 않는다.

## 부록 B. 이 문서가 인용하는 고정 출처

- nasa/cFE: 태그 `v6.5.0a` → `b2765d9f905aa11f0c3b419b62233597366a7db9`(주석 태그 객체 `40e7d511575937789b0b5afd356ace8fff0680d8`가 가리키는 커밋, 2026-10-16 `git ls-remote`), 태그 `v6.6.0a` → `8811c92094512975e14fd545df355705141560b2`, 태그 `v7.0.1` → `c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd`(`git ls-remote`로 확인[M]). 쌍 `6b9c267e6ac539ae17fb79a94467928f44559844` / `58abd38d04390cbe123d89b57819af013464d57f`.
- nasa/cFS 번들: `a41549cb201b8d53b9e5aea81b1e7bb660e6cb8b`(cfe `6b9c267e`, osal `ff4f5231`, psp `a1a7e7a3`), `8d0f9dc3ce9ce83ebd90da5a489094f5b497ed94`(cfe `672b2dc4`), 태그 `v6.6.0a` `05d60929cc9bd9e789f4db24687d66b3058b32f2`(cfe `99038d54`, osal `75beb864`, psp `8f420020`).
- nasa/osal: 태그 `v6.0.0-rc1` `ff4f52312297a3cdf3ddc0cf5ead95f27954630b`, 7139592f = `osal-4.2.1a-1-g7139592`.
- SourceForge 릴리스 파일: §3.1.
- CentOS vault: `https://vault.centos.org/6.5/os/i386/Packages/` (§3.3, §3.3.1, A.6).
- 저장소 기록: `cases/ledger.md`, `cases/C2.md`, `cases/C3.md`, `cases/C5.md`, `env/ENV.md`, `env/CONDITIONS.md`(C8), `env/logs/f_root_capability_probe.log`.
- PI 결정 문서: `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`(§2 D14, §3.3 P-C1HOLD, §3.4 D13, §3.5 P-C2, §4.2 D15), `Architecture/spec/PI_DECISIONS.md`(D13 하위 항목 (b)), `Architecture/spec/SPEC_FINAL.md` §8.2·§9.3·§10.1.

## 부록 C. 검토 반영 기록 (2026-10-15)

검토 지적 11건을 원본(결정문, 사례 기록, 6.4.1 tarball, 고정 SHA의 소스, CentOS vault)과 대조했다. 모두 사실로 확인되어 반영했다.

| # | 지적 | 확인한 것 | 처리 |
|---|---|---|---|
| 1 | 빌드 host `deviation`의 근거 §3.5가 무관하고 시험이 없음 | §3.5는 실행 조건만 다룬다. gcc-4.4.7 포함 RPM 19개가 vault에서 가져와지고 sha256이 기존 값과 일치, 정적 의존이 닫힘, docker 데몬 없음·qemu 없음[M] | §3.3 행 수정(§3.5 인용 삭제, 불가피성 [U]), §3.3.1 프로브 신설, §6·§1·§7 문구를 "불가"에서 "시험하지 않음"으로 변경, A.6 추가. **풀어낸 바이너리의 실행은 세션 권한 정책이 거부해 하지 못했다** |
| 2 | pc-linux 값이 원 사례 조건처럼 읽힘 | readme L100(지원 런타임), `cfe-config.mak`; `C3.md` §10 행 6·7·27, A1; `C5.md` §1·§3; ROADMAP §6.1; PI_DECISIONS D13 (b) | §3.2 서두와 상태 열 재표기, §3.3.2 신설(C3 `deviation`, C5 `unspecified_by_reference`, C2 가정), §1 요약 행 추가 |
| 3 | D13 (c)를 미결정으로 서술 | RESOLVED §3.4 "(c) F1 전에 배치한다", PI_DECISIONS D13 Resolution | §3 서두와 §7 항목 2를 결정됨[D] + 남은 X3·비용으로 수정 |
| 4 | D14·D15 오인용 | RESOLVED §2(D14: 목적별 N), §4.2(D15: 작업량 단위 탐색 예산), §3.3(P-C1HOLD) | §2.2 말미, §2.6 행 분리: N은 PI-D14, 감시 한도는 `unspecified_by_reference`(P-C1HOLD는 유추[I]) |
| 5 | C2 oracle이 미승인 가정인데 확정처럼 서술, §2.6·§7에 없음 | `C2.md` §11 A4 "needs approval… None is set yet", `ledger.md` X7 | §2.5 문장을 제안[U/I]으로 수정, §2.6 행 추가, §7 항목 1에 추가 |
| 6 | 준비 상태 전제의 근거 태그 과대 | QT-3(호출자 요구는 연구자 층), `C5.md` §7의 로드맵 인용 | §2.1·§4.1·§4.2 전제를 [I]/[D]로, [F]는 인용·코드 줄에만 |
| 7 | 패키지 수준·프로브 옵션을 레퍼런스처럼 기재 | readme L88은 "CentOS 6.5-x86"만 적음. 6.4.1 make 트리에 `-std=`·`-Wno-error` 없음(grep) | §3.3 호스트 패키지를 GA 세트 가정·`unspecified_by_reference`로, 프로브 옵션 두 개를 행으로 추가(`deviation`/`unspecified_by_reference`), gnu89 근거 [U]를 RPM man 페이지 확인으로 부분 보강 |
| 8 | "(ii) 확대에 설치는 필요 없다" 단정 | 시험한 것은 헤더 3종 추출뿐. D13이 (ii)를 설치 결정으로 서술 | §1·§3.3 비용·§7 항목 3을 [I]/[U]로 한정, 설치 여부는 PI |
| 9 | "20쌍 일치" 과장 | `C2.md` §1.3·§12 Q2: 18쌍 동일 + 2쌍 근사 | §2.1 표 수정 |
| 10 | 2×2를 SPEC/PI 2×2와 같다고 서술, 셀 B 불가를 상신하지 않음 | 6.4.2 tarball 전체와 v6.5.0a ES 파일 5개에서 새 provider 심볼 0건[M, 재확인]; SPEC §9.3 "on the ENV cFE" | §2.4 서두·문구 수정(3셀 + B 제외), §7 항목 1에 PI 확인 요청 추가 |
| 11 | 편집·인용 오류 6건 | (a) §0.4 없음, (b) `D1`·`D3(a)`·`D4`·`X1`·`X4` 모호, (c) 6b9c267e는 병합(부모 2), 58abd38d는 부모 1 [M: git cat-file], (d) ENV는 모듈 기본값 컴파일(C8; 값은 샘플과 같음 [M]), (e) 한국어 요약에 따옴표, (f) A.4는 쌍마다 3파일 | (a) §0.3으로, (b) 접두사 규약을 서두에 두고 해당 ID 수정, (c) §5.1 수정, (d) §2.2 표·본문 수정과 A.4 해시 추가, (e) 영문 원문 인용 + 번역 표시, 결정문 요약은 따옴표 제거, (f) 요약 행을 3파일로 수정 |

### 거부한 지적 (Rejected findings)

전부 거부한 지적은 없다. 일부만 수용한 것은 하나다.

- 지적 1의 "실행 가능한 프로브" 해법 중 풀어낸 sysroot의 실행 시험은 수행하지 못했다. 이유: 이 세션의 권한 정책(자동 모드 분류기)이 내려받은 외부 바이너리의 실행을 거부했다. 지적이 허용한 다른 해법(미시험을 [U]로 표시, §3.5 인용 삭제, "불가"를 "시험하지 않음"으로 변경)을 적용했다. 거부된 실행을 다른 도구로 우회하지 않았다.


## 부록 D. 2026-10-16 보완 기록

이번 보완도 읽기, 파일 가져오기, 해시 계산, 압축 해제, 구문 분석 전용 프로브만 했다. cFS를 빌드·실행하지 않았고 설치·삭제·git 조작을 하지 않았다. 스크래치 경로는 인용하지 않는다.

### D.1 SPEC rev 1.2에서 다시 찾은 인용

SPEC이 rev 1.2(헤더 "Revision 1.2 (2026-10-15)", 2026-10-16 일관성 편집 포함)로 바뀌었으므로 이 문서가 인용한 문구를 현재 파일에서 다시 찾았다[M: grep]. 절 번호는 바뀌지 않았다.

| 이 문서의 인용 | rev 1.2의 위치 |
|---|---|
| C2 행 "a **2 × 2**, not a pair", "the old against the new role-derived provider on the ENV cFE" (§2.4) | §9.3 표 C2 행, L2827 |
| C2 기반 "the 6.4.2 tarball, or v6.5.0a", 4셀을 독립 결함으로 세지 않음, Q22 (§2.4) | §8.2 표 C2 행, L2505. VP-D4 L2803. §10.1 P-C2·Q22 행 L2898 |
| H-V "Allowed `unspecified_by_reference` until Φ_6.4.x cites 6.4.1 references" (§4.1) | §7.6 L2470 |
| VP-D5, P-RGIT "not a substitute … not blind" (§5) | §9.3 L2804, C5 행 L2829 |
| LP-CASE-C2 (buggy)/(fixed)/(fixed, fail) (§2.4) | 부록 A.8 L3618 |
| 부록 A.3 확인 4(생성자 보류 위치) (§5.3) | L3542–L3550 |
| C3 행 "Not independent C3 evidence"(§4.3) | §9.3 L2828 |
| 부분 diff는 PI가 D13에 추가할 때만(§7 항목 8) | §9.3 C5 행 L2829 |

rev 1.2는 T1 이름 변경(Spec-C)을 적용했다(SPEC §0.11, 부록 C). 이 문서는 처음부터 새 이름을 쓴다.

### D.2 6.4.2·앱 번역 단위로 넓힌 컴파일 DB와 프로브

부록 A.2 스크립트와의 차이는 다음뿐이다(모두 make 조각을 **읽어서** 만든다):

```python
# 1) 판 선택: A = <WORK>/x64<v>/cFE-6.4.<v>-OSS-release  (v = 1, 2). 생성 헤더 디렉터리도 판마다 따로 둔다.
# 2) make 변수 읽기: 주석(# 이후)을 지우고, '=' 뒤 같은 줄만 읽는다(빈 'LOCAL_COPTS =' 다음 줄의 주석을 값으로 읽는 오류를 막음).
def var(t,name):
    t=re.sub(r"#.*","",t.replace("\\\n"," ")); m=re.search(r"^\s*"+name+r"[ \t]*=[ \t]*(.*)$",t,re.M)
    return m.group(1).split() if m else []
# 3) THE_APPS (build/cpu1/Makefile L42-L43)의 앱마다: fsw/public_inc·platform_inc/*.h -> cpu1 inc, fsw/mission_inc/*.h -> mission_inc
# 4) 앱 번역 단위: apps/<app>/fsw/for_build/Makefile의 OBJS, LOCAL_COPTS, INCLUDE_PATH(본문 §3.2 표의 순서), 같은 COPTS
```

결과는 §3.3의 확장 표이다. 실행한 명령의 형식은 부록 A.3과 같고(`-c <src>`를 `-fsyntax-only <src>`로 바꿈), CentOS 6.5 헤더는 부록 A.1의 RPM 3개를 압축 해제한 것이다. 구조체 크기는 `cfe.h`·`cfe_evs_task.h`·`cfe_es_global.h`·`cfe_sb_priv.h`·`<pthread.h>`·`<time.h>`를 포함하고 `char s_T[sizeof(T)]`를 선언한 탐침 파일 하나를 EVS 번역 단위의 include 집합(+ `sb` 디렉터리)으로 `-ast-dump`해 읽었다.

Q16 앵커(부록 A.5)는 재수신 tarball에서 §4의 줄 번호를 그대로 돌려주었다[M]: 예) 6.4.1 `cfe_sb_task.c` L198 / 6.4.2 L206, 6.4.1 `cfe_evs_task.c` L385 / 6.4.2 L436, `cfe_evs_utils.c` L244(두 판), 6.4.2 가드 L620, 6.4.2 `cfe_es_start.c` L936, `cfe_es_apps.c`의 `OS_TaskCreate` L614 / L619, `cfe_es_api.c` 검사 L861 / L800, OSAL `pthread_create` L645·`*task_id` L690·`geteuid() == 0` L587.

### D.3 이번에 하지 못한 것

- 이슈 #198·#73·#72 원문(§2.1): 403. 저장소 접근 추가는 사용자 승인 사항이라 요청하지 않았다.
- 목록 host의 gcc 4.4.7 실행, 컨테이너 이미지·VM 경로(§3.3.1): 이번에도 시험하지 않았다.
- 6.4.x 실행과 §4.3 사건 기록: 실행 기반이 없다(§3.5).

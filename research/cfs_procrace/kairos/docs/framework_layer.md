# 프레임워크 지식 층: runtime OSAL 어댑터가 부르는 OSAL API (2026-11-10, 단계 2a 8c) [F][M]

근거: `plan/RV3_REVIEW_ANALYSIS_20261105.md` §6.3 O9("runtime의 OSAL API 호출을 P0처럼 판·출처 표로 둔다"). 결과는 `plan/step2a_results_20261030.md` §16.

## 1. 무엇인가

- runtime의 OSAL 어댑터(`runtime/src/kz_rt_osal.c`, `runtime/include/kz_rt_osal.h`)는 프레임워크의 OSAL API를 이름으로 부른다. 이것은 프레임워크 지식이다. P0 표(`lib/llvm/p0.cpp`)가 C 라이브러리·스레드 라이브러리 지식을 판과 원문 줄로 적듯, 이 표가 이 지식을 판과 원문 줄로 적는다.
- lint `case-ids`는 `runtime/`을 보지 않는다(`tests/lint/data/case_identifiers.txt` 머리). 이 층은 lint로 막지 않고 이 표로 드러낸다.
- 어댑터는 뼈대다. f34759d 헤더로 컴파일되고, 대상에서 돈 적이 없다(`kz_rt_osal.h:4-5`). 이 표는 그 상태를 바꾸지 않는다.
- 표의 판 셋:
  - **f34759d**: C1 번들 `1df262a`의 OSAL(`c1_build/bundle_1df262a/osal`, HEAD `f34759d6d6f7…`). 판 매크로 5.0.99, build 161(`osapi-version.h:37`, `:43-45`). 어댑터가 컴파일되는 판이다. 원문: `/home/user/work/expect_src/osal_f34759d/src/os/inc`.
  - **d2d877a**: R2 기준 cFS의 OSAL(`cfs_ref/cFS/osal`, `git --git-dir=…/.git/modules/osal rev-parse HEAD` = `d2d877a69cff…`). 판 매크로 7.0.1(`osapi-version.h:45-47`).
  - **4.1.1**: C5 cFE 6.4.1 번들의 OSAL(`c5tools/build/641/keep/cFE-6.4.1-OSS-release/osal`). 판 매크로 4.1.1(`osapi-version.h:38-40`).

## 2. 표

줄은 `src/os/inc/` 아래 파일의 선언 줄이다. "뜻"은 어댑터가 기대는 것이고, 원문 줄의 `@retval`·`@brief`에서 왔다(f34759d). 어댑터의 줄은 `kz_rt_osal.c`다.

| API | 어댑터에서 | 기대는 뜻 (f34759d 원문) | f34759d | d2d877a (7.0.1) | 4.1.1 (C5) |
|---|---|---|---|---|---|
| `OS_MutSemCreate` | 109, 111 (제어 잠금, 대기 표 잠금) | 성공이면 `OS_SUCCESS`(`osapi-mutex.h:53`) | `osapi-mutex.h:60` | `osapi-mutex.h:60` | `osapi-os-core.h:195`, id가 `uint32 *` |
| `OS_MutSemTake` | 33, 44, 61, 70 | 잠금을 얻음(`:82`), 성공 `OS_SUCCESS`(`:92`). 어댑터는 반환을 보지 않는다 | `osapi-mutex.h:96` | `osapi-mutex.h:96` | `osapi-os-core.h:197`, id가 `uint32` |
| `OS_MutSemGive` | 37, 46, 52, 63, 73 | 잠금을 놓음(`:64`), 성공 `OS_SUCCESS`(`:74`). 반환을 보지 않는다 | `osapi-mutex.h:78` | `osapi-mutex.h:78` | `osapi-os-core.h:196`, id가 `uint32` |
| `OS_BinSemCreate` | 116 (gate 대기, 초기값 0) | 성공이면 `OS_SUCCESS`(`osapi-binsem.h:64`) | `osapi-binsem.h:71` | `osapi-binsem.h:71` | `osapi-os-core.h:172-173`, id가 `uint32 *` |
| `OS_BinSemTake` | 55 (시간 제한 없는 대기) | 값을 줄임(`:110`), 성공 `OS_SUCCESS`(`:121`) | `osapi-binsem.h:125` | `osapi-binsem.h:125` | `osapi-os-core.h:176` |
| `OS_BinSemTimedWait` | 55 (시간 제한 대기, ms) | 시간 안에 못 얻으면 `OS_SEM_TIMEOUT`(`:141`), 성공 `OS_SUCCESS`(`:140`) | `osapi-binsem.h:145` | `osapi-binsem.h:145` | `osapi-os-core.h:177` |
| `OS_BinSemGive` | 72 (깨우기) | 값을 늘림(`:91`), 성공 `OS_SUCCESS`(`:102`) | `osapi-binsem.h:106` | `osapi-binsem.h:106` | `osapi-os-core.h:175` |
| `OS_GetLocalTime` | 27 (시계 `osal-local`) | 지금 시각, 성공 `OS_SUCCESS`(`osapi-clock.h:75`, `:84`) | `osapi-clock.h:87` | `osapi-clock.h:142` | `osapi-os-core.h:208` |
| `OS_TimeGetTotalNanoseconds` | 28 | `OS_time_t`를 ns로(`osapi-clock.h:221`; 그 아래 `@returns` 글은 "microseconds"라 적는다, `:232`) | `osapi-clock.h:234` (inline) | `osapi-clock.h:332` (inline) | 선언 없음(grep 0) |

부르지 않고 쓰는 이름과 주석에만 있는 이름:

| 이름 | 어댑터에서 | f34759d | d2d877a | 4.1.1 |
|---|---|---|---|---|
| `OS_SUCCESS` | 27, 56, 109, 111, 116 | `osapi-error.h:96` (0) | `osapi-error.h:96` (0) | `osapi.h:87` (0) |
| `osal_id_t` | 13, 14 | `common_types.h:108` (`uint32`) | `common_types.h:108` | 선언 없음(grep 0) |
| `OS_time_t` | 24 | `osapi-clock.h:44-48` (`int64 ticks`) | `osapi-clock.h:48` | `osapi-os-core.h:95-99` (`uint32 seconds`, `uint32 microsecs`; 필드가 다름) |
| `OS_API_Init` | `kz_rt_osal.h:5` (주석: 이것이 성공한 뒤에만 씀) | `osapi-common.h:139` | 대조하지 않음 | 대조하지 않음 |
| `OS_TaskGetId` | `kz_rt_osal.h:16` (주석: 부르지 않는다) | `osapi-task.h:192` | 대조하지 않음 | 대조하지 않음 |

## 3. 읽기

- f34759d와 d2d877a는 표의 함수 선언이 글자 그대로 같다(시계 둘은 줄만 다름) [M: grep].
- 4.1.1은 같은 이름의 함수가 `uint32` id를 받는다. `osal_id_t`와 `OS_TimeGetTotalNanoseconds`의 선언이 없다 [M: grep]. 이 어댑터의 소스는 그 판의 헤더와 맞지 않는다 [I]. C5의 항목은 단계 3에서 그 판으로 따로 적는다.
- `OS_TimeGetTotalNanoseconds`의 `@returns` 글("microseconds")은 함수 이름·`@brief`("nanosecond units")와 다르다. 어댑터는 이름과 `@brief`를 따른다. 본문은 대조하지 않았다 [U].
- 수: lint 사본으로 센 `kz_rt_osal.c` 56건과 `kz_rt_osal.h` 19건, 합 75건이 이 층이다(`plan/step2a_results_20261030.md` §16). 부르는 함수는 9개다.
- 사례 이름은 이 표에 없다. 판의 출처는 사례 빌드의 번들이다. 그래서 이 표는 `docs/`에 두고 엔진 코드(`include/`, `lib/`, `tools/`)에 넣지 않는다.

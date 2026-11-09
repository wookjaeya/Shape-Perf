# 로컬 묶음과 도구 사본의 기록 (2026-11-09)

- 무엇: `plan/RV4_REVIEW_ANALYSIS_20261109.md`(개정 1) §7.2의 1·2단계 기록이다. PI 지시 "타당한 만큼 수용하고 진행해"(11-09 08:40:43Z)로 lead가 결정 없이 하는 일이다.
- 여기에는 텍스트만 둔다. 조각(`bundle.tar.xz.NN`)과 도구 사본은 저장소에 없다. 저장소는 public이다. 원격 보존의 위치는 PI의 답 뒤다(RV4 분석 §7.3). push하지 않았다.
- 그래서 아래 묶음은 CM §5의 "보존됨"이 아니다. 원격에서 다시 받아 확인하지 않았다. 컨테이너 손실은 막지 못한다. 재빌드·재실행의 덮어쓰기와 실수로 지우기는 막는다 [I].
- 묶음 디렉터리 `../<id>/`의 파일: `INFO.txt`, `MANIFEST.sha256`(preserve_bundle.sh와 같은 꼴), `MANIFEST.tsv`(파일마다 sha256·크기·경로), `PARTS.sha256`, `verify.log`, `VERIFIED_LOCAL.txt`. 조각은 `/home/user/work/preserve/bundles-20261109/<id>/`에 있다.

## 1. 묶음 다섯 [M]

| id | 원본 (RV3A §6.1) | 파일 | 원본 바이트 u | tar.xz 바이트 c | 조각 | tar.xz sha256 | 검증 |
|---|---|---|---|---|---|---|---|
| `step2a-20261109-stages` | RD `step2a/` s1 s2 s3 s4 s5dev (P2 stages) | 869 | 821,979,713 | 27,846,816 | 1 | `4362b735bb2d2899…` | VERIFIED_LOCAL 10:22:20Z |
| `step2a-20261109-tests` | RD `step2a/` tests(T/) tests_fix1 tests_fix2 tests_fix3 tests_s5 (P2 tests) | 665 | 797,286,155 | 31,536,344 | 1 | `7a65f18e10357e14…` | VERIFIED_LOCAL 10:23:47Z |
| `step2a-20261109-eb` | RD `extract_baseline/` (P2 EB) | 383 | 325,344,562 | 7,141,564 | 1 | `984394afd6c6e197…` | VERIFIED_LOCAL 10:25:05Z |
| `ck-20261109-rv3check` | CK `rv3_check/` (P2 CK, `gaps/ctest_20261030/` P11, `rv4/`·`rv4/pv/` 포함) | 97 | 19,134,662 | 16,008,324 | 1 | `e04b7946d5150a48…` | VERIFIED_LOCAL 10:25:31Z |
| `c5-20261109-regen-inputs` | P4 C5 facts 재생성 입력 (아래 목록) | 78 | 6,025,309 | 157,628 | 1 | `269f81b7001bbc0f…` | VERIFIED_LOCAL 10:25:39Z |

- 합: 파일 2,092, u 1,969,770,401 B, c 82,690,676 B. 묶음 디렉터리 전체 du -sb 83,160,669 B.
- 전체 sha256은 각 `INFO.txt`의 `tar_xz_sha256`과 `PARTS.sha256`에 있다. 조각이 하나라 둘이 같다.
- P4 목록(`c5-20261109-regen-inputs.list`, `/home/user/work` 기준):
  - compile DB 다섯(`env/redesign/step2a_tests.sh:63-76`): C1 pre·post, R2 `default_cpu1`, C5 641·642 `est/`.
  - EB `inputs/` 19개.
  - `c5tools/build/641|642`의 `*.record`·`*.log`·`objects_*.sha256`·`binds_instr.kzs` 46개(`est/`, `est_instr/`, `rt32/` 포함; `keep/`·`pristine/`·`b/` 트리는 뺐다).
  - `c5/ref/glibc` 5개, `c5/dl`의 `sha256.txt`·`source_hashes.txt`·`fetch_log.txt`.
  - 뺀 것: C5 keep 트리(82·80 MB)와 tarball 자체. keep 트리는 Q2·Q3이다(RV3A §6.1 P4, §6.6 Q2). tarball은 `c5/dl/sha256.txt`의 해시와 받는 경로로 대신한다.
- 같은 날 원본에 바뀐 파일이 없다: `step2a/`·`extract_baseline/`은 10-31 뒤 바뀐 파일 0 [M]. 검증 끝에 원본 전체를 `MANIFEST.sha256`으로 다시 대조했다(다섯 모두 OK).

## 2. 만든 방법

- 스크립트: `bundle_local.sh`(이 디렉터리에 사본), 멤버 해시는 `member_sha256.sh`. 원본은 `/home/user/work/preserve/bundles-20261109/bin/`에 있다.
- tar 인자는 `env/tools/preserve_bundle.sh:44`와 같다(`--sort=name --owner=0 --group=0 --numeric-owner`). xz -9(-T4). 45 MB 조각(:45와 같다).
- 차이: 파이프에서 바로 조각을 쓴다. 임시 사본(preserve_bundle.sh:44의 `$TMP/bundle.tar.xz`)이 없다. 그래서 만드는 동안의 추가 공간은 c다.
- 자격 증명 패턴 검사: preserve_bundle.sh:31과 같은 패턴. 걸린 것은 하나다. CK의 `rv3_check/rv4/scan/bundlescan.py` 8줄, 검사 정규식 자체다(값이 아니다; 줄을 보고 확인). 그 파일만 허용 목록에 두고 만들었다. 정정(11-09, 정정 단계): 이 검사는 자격 증명 정규식 하나뿐이다. `grep -I`라 이진 멤버(.kz facts·basis 등)는 건너뛰었다. 이메일 꼴과 환경 변수 값은 찾지 않았다. 그래서 INFO.txt의 `credential_pattern_hits=none`은 그 정규식과 텍스트 멤버에 대한 것이다. 검증자가 다섯 묶음을 스트림으로 풀어 이진 멤버까지 다시 훑었다: 자격 증명 패턴 3건(모두 `rv3_check/rv4/scan/bundlescan.py`의 패턴 글자), 환경 변수 값 0건, 이메일 꼴 2건. 두 건은 P4의 glibc 사본 `c5/ref/glibc/pthread_create_2.12.c:3`·`pthread_create_2.17.c:3`의 upstream 저자 줄("Contributed by …")이다(우리도 원본 파일에서 줄 수를 다시 셈 [M]; 값은 적지 않는다). push(PD D4 뒤) 전에 RV4 분석 §4.5와 같은 검사(이메일 꼴, `<value not recorded>`, 환경 변수 값, 이진 멤버 포함)를 다섯 묶음에 다시 한다.
- 원본은 고치거나 지우지 않았다.

## 3. 검증 방법과 verify_bundle.sh와 다른 점

- `env/tools/verify_bundle.sh`: 원격 clone(:23) → `PARTS.sha256` → 조각을 xz 하나로 잇기(:29) → `xz -t` → 묶음 전체를 풂(:31-32) → `MANIFEST.sha256` 대조.
- 여기서 한 것(스트림 검증, 디스크에 풀지 않음):
  1. `sha256sum -c PARTS.sha256`
  2. `cat bundle.tar.xz.?? | xz -t`
  3. 빈 디렉터리에서 `cat bundle.tar.xz.?? | xz -dc | tar -xvvf - --to-command="sh member_sha256.sh"`. 멤버마다 sha256·크기(헤더)·경로를 한 줄로 적는다. 파이프 셋의 상태가 모두 0인지 본다.
  4. 그 디렉터리에 아무것도 쓰이지 않았는지 본다(다섯 모두 0개).
  5. 같은 스트림의 `-vv` 목록으로 멤버 종류를 센다. 정규 파일과 디렉터리만 허용한다(다섯 모두 그렇다).
  6. 멤버 줄을 경로로 정렬해 `MANIFEST.tsv`와 줄마다 대조한다. 다섯 모두 차이 0줄, 멤버 수 = MANIFEST 줄 수.
  7. 원본을 `MANIFEST.sha256`으로 다시 대조한다.
- 다른 까닭은 디스크다. 하한은 300 MB(314,572,800 B)로 두었다.
  - verify_bundle.sh의 로컬 부분만 해도 조각(c) 위에 이은 xz(c)와 풀어 놓은 묶음(u)이 더해진다. 순간 최대 약 2c + u다.
  - stages: 2c + u = 877,673,345 B. 그때 여유 1,151,959,040 B에서 274,285,695 B가 남는다. 하한보다 작다.
  - tests: 860,358,843 B. 여유 1,123,602,432 B에서 263,243,589 B가 남는다. 하한보다 작다.
  - EB(339.6 MB), CK(51.2 MB), P4(6.3 MB)는 풀어도 들어갔다. 다섯 모두 같은 방법으로 했다.
  - 스트림 검증의 추가 공간은 멤버 해시 목록뿐이다(수백 KB).
- 같은 것: 조각 해시, xz 무결성, 묶음 안 파일마다 sha256. 
- 하지 않은 것: 원격에서 다시 받기(CM §5의 첫 조건; push하지 않았다). 분석 입력을 열어 재생하는 점검(CM §5). 디렉터리 멤버의 mode·mtime 대조(verify_bundle.sh도 하지 않는다).
- 음성 점검(작은 시험 묶음, 시험 뒤 지움): 원본 파일 하나를 바꾸면 멤버 대조가 2줄 차이를 냈다. 조각의 한 바이트를 바꾸면 `PARTS.sha256`과 `xz -t`가 실패했다.
- push 뒤에는 `MANIFEST.sha256`을 그대로 써서 verify_bundle.sh로 원격 검증할 수 있다. 그때는 조각을 이 디렉터리의 `../<id>/`에 더한다. 공간은 RV3A §6.1 메모(개정 4)의 5c + u로 다시 잰다.

## 4. 도구 사본 PV (RV4 분석 §7.2 1단계)

- 자리: `/home/user/work/preserve/tools-20261109/`. 기록: `pv/README.txt`(CK `rv4/pv/README.txt`와 같다).
- 처음 다섯(lead, 08:42–08:46Z, 세션 기록 SL:3281 명령 `xz -9 -T4`; `pv/sl3281_3282.txt`): kz-static `db172b74`, kz `b254483e`, kz-dump `05623054`, W6 kz `5d64256a`, E-ST kz-static `3ec20abf`. 목록 `pv/ORIGINAL_SHA256`, `pv/XZ_SHA256`.
- 더한 넷(10:14–10:16Z): kz-rt-dump `4390d4f1`, MC `CMakeCache.txt` `bc2f43b4`, kz-static `flags.make` `181c55a0`, W6 kz-static `16b284cc`. 목록 `pv/ADDED_ORIGINAL_SHA256`, `pv/ADDED_XZ_SHA256`. 처음 두 목록은 고치지 않았다.
- round-trip:
  - 3ec20abf와 더한 넷은 임시 파일로 풀어 sha256과 `cmp`로 원본과 대조했다. 모두 OK. 임시 파일은 지웠다.
  - 다섯 모두 스트림으로 다시 해시했다. 모두 OK.
  - 앞선 기록: lead `db172b74`·`5d64256a`(SL:3282), 검사자 `db172b74`·`b254483e`·`05623054`(RV4 분석 §4.6).
- `pv/pv_step1.log`의 FAIL 한 줄은 스크립트의 case 패턴 오류다. 자료 불일치가 아니다. 고쳐 다시 잰 줄이 로그 끝에 있다(OK).
- MC에서는 아무것도 빌드하지 않았다. PV du -sb 139,091,159 B.

## 5. df -B1 / 의 여유 (B) [M]

다른 작업도 같은 디스크를 쓰므로 단계 사이의 값은 이 작업만의 변화가 아니다.

| 시점 (UTC) | 여유 |
|---|---|
| 0단계 앞 10:12:07 | 1,185,701,888 |
| 0단계 뒤 | 1,185,325,056 |
| 1단계 앞 10:14:55 | 1,185,206,272 |
| 1단계, 3ec20abf 임시 파일이 있을 때 | 1,040,592,896 |
| 1단계, 16b284cc 임시 파일이 있을 때 (이 작업의 최저) | 1,009,422,336 |
| 1단계 뒤 10:16:37 | 1,152,663,552 |
| 2단계 앞 10:20:51 | 1,152,008,192 |
| stages 앞 / 만든 뒤 / 검증 뒤 | 1,151,959,040 / 1,123,631,104 / 1,123,237,888 |
| tests 앞 / 만든 뒤 / 검증 뒤 | 1,123,602,432 / 1,091,735,552 / 1,091,379,200 |
| EB 앞 / 만든 뒤 / 검증 뒤 | 1,091,686,400 / 1,084,194,816 / 1,084,002,304 |
| CK 앞 / 만든 뒤 / 검증 뒤 | 1,084,194,816 / 1,068,142,592 / 1,068,077,056 |
| P4 앞 / 만든 뒤 / 검증 뒤 | 1,068,118,016 / 1,067,917,312 / 1,067,855,872 |
| 2단계 뒤 10:25:39 | 1,067,917,312 |

- 순서는 큰 것부터다(stages → tests → EB → CK → P4). 모두 하한 위에서 들어갔다. 건너뛴 묶음은 없다.

## 6. 하지 않은 것

- push와 원격 검증: 보존 위치의 답 뒤(RV4 분석 §7.3).
- 도구 바이너리·cFS 실행 파일의 묶음(P1·P3)과 옛 판 셋의 묶음: Q3·보존 위치 뒤. 지금은 PV 사본뿐이다.
- 지우기: 하지 않았다. scratchpad `rv3/`, `gaps/vb_*`, `rv4/`의 원본은 그대로다(Q2). 이 작업이 만든 임시 파일만 지웠다.
- 여덟 기존 묶음의 재검증(P8): 이 단계의 일이 아니다.
- AR §0의 C5 재생성 절차 한 단락(RV3A §6.1 P4의 완료 조건): 이 단계에서 빠졌다. 정정 단계(11-09)에서 AR §0 항목 7로 더했다. 그 절차를 끝까지 돌려 보지는 않았다.

## 7. 이 디렉터리의 파일

- `bundle_local.sh`, `member_sha256.sh`: 만든 스크립트와 멤버 해시.
- `c5-20261109-regen-inputs.list`: P4의 파일 목록.
- `run.log`: 묶음마다 끝난 시각과 여유.
- `pv/`: PV의 해시 목록 넷, 기록(`README.txt`), 1단계 스크립트와 로그, SL:3281–3282의 명령과 결과.

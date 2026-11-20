PV 기록 (RV4 분석 §7.2 1단계, 2026-11-09)
PV = /home/user/work/preserve/tools-20261109/ (MC 밖의 도구 사본, xz -9). MC에서 아무것도 빌드하지 않았다.

1. 처음 다섯 (lead, 2026-11-09 08:42:16Z-08:46:44Z) [F: 세션 기록 SL:3281 명령, SL:3282 결과; sl3281_3282.txt]
   명령: cd /home/user/work && xz -9 -T4 -c <원본> > PV/<이름>.xz, 원본 sha256 -> ORIGINAL_SHA256, (cd PV && sha256sum *.xz > XZ_SHA256)
   파일 mtime 08:43:59-08:46:40Z. du -sb 106,649,381 B(11-09 08:46, 다섯 xz와 목록 둘).

   원본 sha256 (ORIGINAL_SHA256)                                         원본 경로 (/home/user/work 기준)                 xz 파일 (바이트)
   db172b74dc973de6a7e9c5b75e7f6655d26574023d746741606f458df5e1ffc4  merge-check/kz-llvm/tools/kz-static/kz-static   kz-static.db172b74.xz (39,227,324)
   b254483e25873902ec6408b1764c7e9b71238fe4c400e237ae49f88f55fc824c  merge-check/kz-llvm/tools/kz/kz                 kz.b254483e.xz (17,478,296)
   05623054296bbb993a66308c9fd8e710873f064e7672318e495eabba74fbaf43  merge-check/kz-llvm/tools/kz-dump/kz-dump       kz-dump.05623054.xz (608,596)
   5d64256abd78e489f1a9bed5b6ac1e45b399b85802d02f45dbfe4d47a586ab3f  kairos-build/tools/kz/kz                        kz.5d64256a.xz (17,427,972)
   3ec20abf59343da92873e684ad46f8fd67c887055554ab85134c6cbf4bae5b77  c5tools/kz-llvm/tools/kz-static/kz-static       kz-static.3ec20abf.xz (31,906,252)

   풀어서 다시 해시한 것(round-trip):
   - lead, 08:46Z [F: SL:3282]: db172b74, 5d64256a (xz -dc | sha256sum = 원본 sha256sum)
   - 검사자, 11-09 [F: RV4 분석 §4.6, §8]: db172b74, b254483e, 05623054
   - 이번(10:14-10:17Z) [M: pv_step1.log]:
     - 3ec20abf: 임시 파일로 풀고 sha256과 cmp를 원본과 대조. OK. 임시 파일은 지웠다.
     - 다섯 모두: xz -dc | sha256sum = ORIGINAL_SHA256. OK(디스크에 풀지 않음).
     - XZ_SHA256 다섯 OK. 원본 다섯이 제자리에서 ORIGINAL_SHA256과 같다(sha256sum -c).
   - 주의: pv_step1.log의 "roundtrip_stream FAIL kz.5d64256a.xz ... want=3ec20abf" 한 줄은 스크립트 오류다. 자료 불일치가 아니다.
     case 패턴(*/c5tools/*)이 상대 경로 c5tools/...에 맞지 않아 c5tools 줄을 앞 파일과 비교했다. 고친 뒤 다시 잰 줄이 로그 끝에 있다(OK).

2. 더한 넷 (이번, 10:14:55Z-10:16:37Z) [M: pv_step1.log, pv_step1.sh]
   명령: 같은 꼴. xz -9 -T4 -c <원본> > PV/<이름>.xz. 원본 sha256을 압축 앞뒤로 재어 같음을 확인. 목록은 ADDED_ORIGINAL_SHA256, ADDED_XZ_SHA256(처음 두 목록은 고치지 않았다).
   각각 임시 파일로 풀어 sha256과 cmp를 원본과 대조했다. 넷 모두 OK. 임시 파일은 지웠다.

   원본 sha256                                                         원본 경로 (원본 바이트, mtime)                                              xz 파일 (바이트)
   4390d4f13b5bcae3145ddd4b3decb089790d18d6ebbdc3104de99beb66e5063a  merge-check/kz-llvm/runtime/kz-rt-dump (3,350,272, 10-29 07:31:59)              kz-rt-dump.4390d4f1.xz (828,296)
   bc2f43b405fb16bfe8a12974af2be70ce2600dd43aa5550bff1c5600e1a996da  merge-check/kz-llvm/CMakeCache.txt (17,680, 10-29 07:31:40)                    mc.CMakeCache.txt.bc2f43b4.xz (3,692)
   181c55a0dbd99fcbc80d2a312239e9ffa09c53ddb1a2fcaa4e2155a8495833f0  merge-check/kz-llvm/tools/kz-static/CMakeFiles/kz-static.dir/flags.make (665, 10-30 17:15:40)  mc.kz-static.flags.make.181c55a0.xz (528)
   16b284ccc915e1109e9d1d96e05809fdedbbbbacbbe9712bf3beeb5badd56600  kairos-build/k1r3-llvm/tools/kz-static/kz-static (143,235,016, 10-29 04:41:33)  kz-static.16b284cc.xz (31,608,428)

   어느 파일인지의 근거: RV3A §6.1 P1(MC의 kz-rt-dump, CMakeCache.txt, flags.make; W6 kz-static 16b284cc), CK gaps/space_20261105.txt:30-31·:63(경로), RV4 분석 §8(flags.make 경로).
   flags.make에는 소스 식별자 줄(KZ_TOOL_SOURCE_SHA256=1ccee6ce...)이 있다.

3. 크기와 공간
   PV du -sb: 139,091,159 B (처음 106,649,381 + 더한 32,441,778).
   df -B1 / 여유: 1단계 앞 1,185,206,272, 3ec20abf 임시 파일이 있을 때 1,040,592,896, 16b284cc 임시 파일이 있을 때 1,009,422,336, 1단계 뒤 1,152,663,552.

4. 이것이 막는 것과 막지 못하는 것
   MC의 재빌드가 2a 도구를 덮는 위험은 kz-static·kz·kz-dump·kz-rt-dump·빌드 설정 둘에 대해 없어졌다 [I].
   컨테이너 손실은 막지 못한다. 원격 보존은 보존 위치 답 뒤다(RV4 분석 §7.3).

5. 확인 명령
   cd /home/user/work/preserve/tools-20261109 && sha256sum -c XZ_SHA256 ADDED_XZ_SHA256
   cd /home/user/work && sha256sum -c preserve/tools-20261109/ORIGINAL_SHA256 preserve/tools-20261109/ADDED_ORIGINAL_SHA256  (원본이 남아 있는 동안)
   xz -dc <xz 파일> | sha256sum  (ORIGINAL_SHA256 또는 ADDED_ORIGINAL_SHA256의 값과 같아야 한다)

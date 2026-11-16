# 도구 빌드 디렉터리 둘의 삭제 기록 (2026-11-16)

- **무엇을 지웠나**: `/home/user/work/kz-dev`(1,552,187,739 B)와 `/home/user/work/c5tools/verify`(2,328,289,094 B). 둘 다 옛 Kairos 도구의 빌드 디렉터리(목적 파일, 실행 파일, 빌드 로그)다.
- **근거**: PI 결정(2026-11-16, 이 세션의 질문에 대한 답 "지워도 됨"). 질문에는 지우기 전에 실행 파일 해시, 도구가 내는 식별, 빌드 설정 파일을 텍스트로 남긴다는 조건을 붙였다.
- **지우기 전에 남긴 것** (이 디렉터리):
  - `kz-dev.executables.sha256`, `c5tools_verify.executables.sha256`: 실행 파일과 라이브러리의 sha256 목록.
  - `*.CMakeCache.txt`, `*.config_files.txt`, `c5verify.HEAD.txt`: 빌드 설정과 설정 파일의 목록, 소스 판.
  - `tool_identity.txt`: 도구가 스스로 내는 사용법 출력(판 출력 명령이 없는 도구는 그 사실).
  - `*.size.txt`: 파일 수와 바이트.
  - `free_before.txt`, `free_after.txt`: 여유 공간 482,578,432 B → 4,369,088,512 B.
- **저장소가 해시로 가리키던 실행 파일의 처리** [직접 확인: 해시 앞 8자리를 `git grep`으로 찾음]:
  - `kzgcc.so 27a49a6a`, `kzgcc-events 7545a811`, `libkzrt-posix.a d0fbd6a4`, `libkzrt-decode.a a4fbbe03`, `libkzrt.a 718acdfd`, `libkzrt-pool4.a 664c68d7`: 같은 해시의 사본이 `c5tools/eid1/`, `c5tools/plugin-build/`, `instr/gcc/`에 남아 있다.
  - `kz-static de1fd4c8`(`c5tools/verify/kz-llvm`): 다른 사본이 없었다. 저장소에서는 RV3A:415 한 곳만 이 해시를 적는다(C5 도구 브랜치 `74e271d`의 확인 빌드이고 C5 기록에는 없다). 그래도 지우기 전에 xz 사본을 두고 풀어서 해시가 같은지 확인했다: `/home/user/work/preserve/tools-20261109/kz-static.de1fd4c8.xz`(31,802,716 B), 해시는 그 폴더의 `ADDED_ORIGINAL_SHA256`·`ADDED_XZ_SHA256`에 더했다.
  - 나머지 해시 일치 넷은 CMake가 만드는 컴파일러 확인용 파일이라 연구 기록과 무관하다.
- **지우지 않은 것**: `kairos-build`, `c5tools/kz-llvm`, `merge-check`(지금 도구), cFS 빌드 트리, 2a 출력, 묶음, PV, CK.

# v3 후속 검토: E0–E1 결과 (실행 식별)

- 작성일: 2026-09-30
- 근거 계획: [`docs/research_plan_v3_followup.md`](research_plan_v3_followup.md) ("Shape-Perf v3 추가 실험 권고안과 조건부 연구 로드맵", 원본 그대로 보존)
- 범위: **E0(증거 보존)과 E1(실행 구현 식별)만** 수행했다. E2 이후, 새 VM, 전 범위 sweep, D8/D1, 새 모델은 시작하지 않았다.
- **E0·E1에서는 아무것도 시간 측정하지 않았다.** 추적(gdb, LD_DEBUG) 실행은 성능 자료가 아니다.
- 원자료 `results/v3/`는 수정하지 않았다. 새 결과는 모두 `results/v3_followup/`에 있다.

## 0. 결론

1. **v3의 공동 로딩 비교는 무효다.** v3 산출물은 모두 `--tag` 없이 `model.so`라는 같은 이름으로 컴파일되었다. 이런 두 라이브러리를
   한 프로세스에 올리면, **나중에 로드된 arm의 호출이 먼저 로드된 arm의 계산 코드를 실행했다.** L=64에서 15개 호출 모두 그랬고,
   호출 순서와 결합 시점(lazy, `LD_BIND_NOW`)에 관계없었다.
2. **단독 실행은 자기 코드를 실행했다.** 한 프로세스에 모델 하나만 올린 S8, S1, AA는 모두 자기 계산 함수를 실행했다.
3. **원인은 심볼 결합이다(H-bind).** 런타임은 모델을 `dlopen(RTLD_LAZY | RTLD_GLOBAL)`로 열고 entry만 자기 handle에서 찾는다.
   entry 안의 `_mlir_ciface_main_graph_model`·`main_graph_model` 호출은 PLT를 거쳐 전역 범위에서 **처음 정의된 것**에 결합된다.
   이것은 문서화된 loader 동작이며, ONNX-MLIR의 "모델마다 다른 tag" 지침을 v3 하니스가 어긴 결과다. 새로운 원리가 아니다.
4. **고유 tag로 해결된다.** tag만 더해 다시 컴파일한 4개 빌드(S8-alpha, S1-bravo, S8-bravo, S1-alpha)로 공동 로딩 16개 case를 돌렸다.
   두 tag 배정, 두 로드 순서, 두 첫 호출 순서에서 56개 호출이 모두 자기 코드를 실행했다. 같은 tag를 일부러 겹치면 오결합이 다시 생긴다.
   tag를 더해도 lowering IR과 계산 함수의 명령열은 바뀌지 않았다(주소만 다름).
5. **v3의 "낮은 L의 큰 커널 효과는 측정 artifact였다"는 철회를 다시 철회한다.** 무효였던 것은 공동 로딩 진단 쪽이다.
   단독 실행 비교는 각자 자기 코드를 실행한 비교였다. L=64에서는 추적으로 확인했고, L=63·65·96은 같은 하니스·같은 메커니즘으로
   추정할 뿐 추적하지는 않았다. 단독 실행에서 L=63–65와 96의 S8이 느렸던 관측은 **미확정 관측**으로 되돌린다.
   개발 컨테이너 수치이므로 결과는 아니며, E2의 재측정 대상이다.
6. **E2로 갈 수 있다.** 실행 식별 게이트는 단독 실행 경로에서 통과했다. 공동 로딩은 고유 tag일 때만 통과했다.
   단 E2의 확인 자료를 모으기 전에 protocol을 동결해야 한다(7절).

## 1. 직전 계획 대비

| 계획 항목 | 상태 | 비고 |
|---|---|---|
| E0-1 v3 원자료 보존, 별도 디렉터리 | 완료 | `results/v3/` 무수정, 새 결과는 `results/v3_followup/` |
| E0-2 L=64 artifact·입력·argv·출력·시간 기록 식별 | 완료(일부 복원) | compile argv는 기록되지 않았으나 복원한 argv로 **비트 단위 재현**. AA 파일은 설계상 삭제됨 |
| E0-3 compiler·runtime 바이너리 식별 | 완료 | 측정 당시의 runtime 해시는 기록되지 않았다. 현재 파일·mtime·빌드 이력으로 동일성을 추정했다(2.3절) |
| E0-4 저장소 상태·diff·미추적 파일 | 완료 | `repo_state/` |
| E0-5 공동 로딩 하니스의 순서 복원 | 완료 | 2.4절 |
| E0-6 프로세스별 모델 로드 목록 | 완료 | 2.4절 (기록의 pid·코드로 확인) |
| E1-A 정적 조사 | 완료 | 무태그·tag 빌드 모두 |
| E1-B 실제 호출 경로 | 완료 | gdb(ASLR 켬), 실행 바이트 해시, LD_DEBUG(lazy, BIND_NOW) |
| E1-C 고유 tag 재컴파일·재검증 | 완료 | S8-alpha/S1-bravo와 교환 쌍, AA-tag, 같은 tag 진단 |
| E1-D 출력 식별용 진단 모델 | 대체 | 서로 다른 값을 반환하는 C fixture로 loader 경로를 시험(테스트). 실제 K형 산출물은 별도로 추적함 |
| loader 회귀 테스트 | 완료 | `tests/test_loader_identity.py` |
| E2 이후 | 미실행 | 7절 |

## 2. E0 — 원자료 보존과 실행 명세 복원

전체 목록은 `results/v3_followup/e0/manifest.json`에 있다(생성: `scripts/record_e0_followup.py`).

### 2.1 L=64 K형 셀

| 항목 | 값 | 상태 |
|---|---|---|
| 입력 `input.npy` | 바이트 해시가 `build.json`의 `input_sha256`과 일치 | 보존(`e0/artifacts_L0064/`) |
| S8 `.so` | `6e8faed1…`, build-id `fce9b3dd…` | 보존, `build.json`과 일치 |
| S1 `.so` | `01328b45…`, build-id `ce55e3f1…` | 보존, `build.json`과 일치 |
| AA `.so` | S8의 `shutil.copyfile` 사본 | **측정 후 삭제(설계상)**. AA 기록의 `artifact_hash`는 파일에서 계산한 값이 아니라 S8 해시를 **복사한 값**이다. 바이트 동일은 복사 코드로만 보장된다 |
| compile argv | 기록 안 됨 | 빌드 커밋 `ebd96be`의 코드로 복원했다. **복원한 argv로 다시 컴파일하니 S8·S1 모두 기록된 SHA-256과 비트 단위로 같았다** |
| run argv | 코드와 기록 필드로 복원 | `python -m shapeperf.paired --worker <item>`를 매 프로세스 새로 `exec`했다(`measure.run_item`) |
| 워커 stdout/stderr | 없음 | 드라이버 로그만 있다(`e0/logs/`) |

### 2.2 컴파일러

S8 `e34bcd6a…`(`variants/orig`), S1 `fcea1caa…`(`variants/cap1`). v3 G0 기록과 같다.

### 2.3 런타임

`PyRuntimeC.cpython-311-x86_64-linux-gnu.so` `2187aa58…`. mtime은 v3 측정보다 앞선다. 측정 당시 해시는 기록되지 않았다.
Loader 동작은 고정 커밋 `src/Runtime/ExecutionSession.cpp`에서 확인했다. 파일명에서 tag를 만들고(88–119행),
`dlopen(RTLD_LAZY | RTLD_GLOBAL)`(132행)으로 연 뒤, entry를 `dlsym(handle, "run_main_graph_<tag>")`(238–247행)로 찾는다.

- **런타임 식별**(독립 감사). `PyRuntimeC`의 mtime은 2026-09-28T11:17:55Z다. `.ninja_log`상 유일한 빌드이고, v3 측정 창(09-29 05:39–08:40Z)보다 앞선다.
  그래서 측정에 쓰인 것이 현재 파일이라는 것은 **거의 확실하지만 기록이 아니라 추정**이다.
  - v3 provenance 스키마는 런타임 경로·해시를 기록하지 않았다.
  - `libcruntime.a`(모델 `.so`에 정적으로 들어가는 C 런타임)도 한 번만 빌드됐다. cap1 빌드와 원복 빌드는 이것을 다시 만들지 않았다.
  - 주 빌드 트리의 컴파일러는 원복 때(09-29 05:34Z) 다시 링크됐고, `variants/orig`와 바이트가 같다(`e34bcd6a…`).
  - Python 3.11.15, numpy 2.2.6, onnx 1.23.0, onnxruntime 1.23.2, glibc 2.39-0ubuntu8.7이다. 패키지 파일은 RECORD 해시와 `dpkg -V`로 온전함을 확인했다.
  - 커널은 컨테이너 재시작으로 `fc-v49`에서 `fc-v50`으로 바뀌었다(v3 기록은 v49).

### 2.4 v3 측정 도구별 프로세스 구성

v3의 각 측정·검증 도구가 프로세스마다 어떤 모델을 몇 개, 어떤 순서로 올렸는지 코드와 기록(pid·artifact 필드)으로 따로 감사하고 대조했다.
감사 결과 원문은 `results/v3_followup/e0/audit.json`에 있다.

| 도구 (결과 파일) | 프로세스 생성 | 프로세스당 모델 | 공동 로딩 | H-bind 영향 | 근거 |
|---|---|---|---|---|---|
| G1 모델 전체 timing (`g1/timing/measurements.jsonl`) | 매번 새 `exec` | 1 | 없음 | 없음 | 기록 450개 = pid 450개. pid마다 artifact 1개 |
| G1 정확성 (`g1/verify/*`) | 매번 새 `exec`, arm별 따로 | 1 (부모는 ORT만) | 없음 | 없음(코드 근거) | 기록에 pid가 없어 기록으로는 확인 불가 |
| G1 L=128 점검 (`g1/check128/verify_L0128.json`) | 알 수 없음 | 알 수 없음 | 알 수 없음 | 알 수 없음 | 당시 코드가 커밋 전이었고 보존되지 않음 |
| G2 단독 timing (`g2/timing/fresh_process.jsonl`) | 매번 새 `exec` | 1 | 없음 | 없음 | 기록 900개 = pid 900개 |
| G2 반복 (`fresh_process_replicate.jsonl`) | 매번 새 `exec` | 1 | 없음 | 없음 | 기록 432개 = pid 432개 |
| G2 scan (`interleaved_in_process_test.jsonl`) | 새 `exec`, 한 프로세스에 3 arm | 3 (S8 → S1 → AA) | **있음** | **있음**: S1·AA가 S8 코드 실행 | pid 9개 × 3 arm. L=64에서 S1 27.30 µs(단독 18.98 µs) |
| G2 진단 `context_L64_*` | 새 `exec` | `*_alone`은 1, 나머지 2 | 25개 중 15개 프로세스 | 2개 올린 프로세스는 **있음** | 기록에 pid·provenance 없음. 코드와 블록×변형 수로 셈 |
| G2 진단 `loadorder_factorial_*` | 새 `exec` | L=64는 모두 2, L=63·96은 30개 중 20개가 2 | **있음** | **있음**: 시간이 잰 arm이 아니라 로드 순서를 따름 | 위와 같음 |
| direction probe (`direction_probe/`) | 한 번 실행 | 1(계측 빌드, S8/S1 아님) | 없음 | 해당 없음 | ORT 수치는 원자료 없음 |

모든 단독 timing 파일(G1, G2 본 측정, 반복, 규모 시험; 기록 1,785개)에서 한 pid가 모델을 두 개 이상 쓴 적이 없다.
한 pid가 여러 arm을 쓴 기록 파일은 scan 하나뿐이다.


### 2.5 provenance 복원

v3 provenance는 코드 상태를 해시(`diff_sha256`, `untracked_sha256`)로만 남겼다. 독립 에이전트가 후보 코드 내용으로 같은 해시를 다시 계산해
**dirty 실행의 코드 상태를 모두 복원**했다(`results/v3_followup/e0/audit.json`).

| 실행 | 당시 상태 | 복원 결과 |
|---|---|---|
| G1 timing 배치 1·2, G1 검증 배치 1·2, G1 빌드 배치 1·2 | `28382a9` + 미커밋 변경 | **복원**: 코드 상태는 정확히 `1d781b6` 트리다(추적 파일 4개의 diff와 미추적 파일 16개가 해시 일치). 덮어써진 스냅샷 4개도 본문을 재구성해 id가 일치한다(생성 시각만 복원 불가) |
| G1 배치 3 | `1d781b6` clean | 복원 불필요 |
| check128 빌드·검증·timing·AA 시험 | `28382a9` + 미커밋 변경 | **복원**: 세션 기록 재생으로 해시 일치 |
| G2 빌드 | `ebd96be` clean | 복원 불필요 |
| G2 규모 시험, G2 본 측정(`provenance_measure_11`) | `ebd96be` + 미커밋 변경 | **복원**: `run_subgraph_benchmark.py@eefb40d`를 얹은 상태(288개 조합 중 유일 일치). 단 본 측정 도중(07:21:55) `paired.py`에 분석 함수만 추가하는 수정이 있었고, 그 뒤 시작된 워커는 수정본을 import했다 |
| G2 Q 측정, 반복 측정 | `eefb40d` clean | 복원 불필요 |
| G2 scan | `eefb40d` + 미커밋 변경 | **복원**: 세션 기록 재생으로 해시 일치 |
| 진단 `context_*`, `loadorder_*` | provenance **없음** | **해시로 확인 불가**. 스크립트는 실행 당시 미커밋이었다 |

남는 공백:
- 진단 파일의 코드 상태.
- 덮어써진 G1 스냅샷 4개의 생성 시각.
- 측정 기록에 컴파일러 식별(`compilers: []`)과 런타임 해시가 없다는 점.
- `.gitignore` 대상 입력(모델 `.so` 등)은 기록의 `artifact_hash`로만 식별된다는 점.


## 3. E1 — L=64 실행 구현 식별

### 3.1 E1-A 정적 조사 (무태그 산출물)

`results/v3_followup/e1/elf_orig/`: `readelf --dyn-syms/-rW/-dW/-nW`, `objdump -drwC`.

- 두 `.so`가 `run_main_graph_model`, `_mlir_ciface_main_graph_model`, `main_graph_model`을 포함한 51개 함수를
  **같은 이름·GLOBAL·DEFAULT**로 내보낸다.
- 호출 사슬은 `run_main_graph_model` → `_mlir_ciface_main_graph_model@plt` → `main_graph_model@plt`이다.
  세 호출 모두 `R_X86_64_JUMP_SLOT`, 즉 교체 가능한 PLT 호출이다. `DT_SYMBOLIC`·`BIND_NOW`는 없다.
- `main_graph_model`은 두 파일에서 같은 오프셋(0x23b0)에 있지만 크기(S8 3026 B, S1 5728 B)와 바이트가 다르다.
  그래서 실행된 바이트의 해시로 둘을 구별할 수 있다.

### 3.2 E1-B 실행 식별 표

각 case는 새로 `exec`한 프로세스 3개에서 돌렸다.
- gdb: entry·wrapper·compute에 breakpoint를 걸어, 멈춘 PC가 속한 DSO와 module-relative 주소, 메모리에서 읽은 함수 바이트의 SHA-256을 기록했다. ASLR은 켰다.
- `LD_DEBUG=bindings`: 기본 lazy 결합.
- `LD_DEBUG=bindings` + `LD_BIND_NOW=1`: 보조 조건.

세 방법이 모든 호출에서 일치했다. 증거는 `results/v3_followup/e1/identity_{orig,tagged}/<case>/`에 있고,
요약 표는 `identity_table.json`이다(생성: `scripts/verify_execution_identity.py`, 커밋 `14674fa`). LD_DEBUG 교차 확인은 94개 호출 모두 "agrees"다.

| artifact/tag 체계 | load order | requested arm | actual compute artifact | output | 판정 | 증거 경로 |
|---|---|---|---|---|---|---|
| 기존(무태그, tag `model`) | S8만 | S8 | S8 `6e8faed1…`, `main_graph_model+0` @0x23b0, 실행 바이트 `49916aec…` | pass | verified_own (2/2) | `identity_orig/I8` |
| 기존 | S1만 | S1 | S1 `01328b45…`, @0x23b0, 실행 바이트 `7f634c25…` | pass | verified_own (2/2) | `identity_orig/I1` |
| 기존 | AA만 | AA | AA(바이트는 S8과 같음, 매핑 경로로 판정) | pass | verified_own (2/2) | `identity_orig/IAA` |
| 기존 | S8 → S1 | S8 | S8 | pass | verified_own (6/6) | `C81-8`, `C81-1`, `SEQ81` |
| 기존 | S8 → S1 | S1 | **S8** `6e8faed1…`, 실행 바이트 `49916aec…` | pass | **verified_other (5/5)** | `C81-8`, `C81-1`, `SEQ81` |
| 기존 | S1 → S8 | S1 | S1 | pass | verified_own (6/6) | `C18-8`, `C18-1`, `SEQ18` |
| 기존 | S1 → S8 | S8 | **S1** `01328b45…`, 실행 바이트 `7f634c25…` | pass | **verified_other (5/5)** | `C18-8`, `C18-1`, `SEQ18` |
| 기존 (v3 scan 순서) | S8 → S1 → AA | S1, AA | **S8** | pass | **verified_other (4/4)** | `SCAN-S8-S1-AA` |
| 기존 (A/A 공동) | S8 → AA | AA | **S8** | pass | verified_other (1/1) | `CAA-S8-AA` |
| 고유 tag | 각각 단독 | S8α, S1β, S8β, S1α | 각자 | pass | verified_own (8/8) | `identity_tagged/I*` |
| 고유 tag | S8α ↔ S1β (두 순서 × 두 첫 호출 + 순차) | S8α, S1β | 각자 | pass | verified_own (22/22) | `C8a1b-*`, `C1b8a-*`, `SEQ*` |
| 고유 tag 교환 | S8β ↔ S1α (두 순서 × 두 첫 호출) | S8β, S1α | 각자 | pass | verified_own (16/16) | `C8b1a-*`, `C1a8b-*` |
| AA-tag | S8α ↔ S8β | S8α, S8β | 각자(바이트 동일, 경로로 판정) | pass | verified_own (8/8) | `CAA-*` |
| 같은 tag (진단 전용) | S8α → S1α / S1α → S8α | 나중 로드 arm | **먼저 로드 arm** | pass | **verified_other (2/2)** | `SAMETAG-*` |

집계: 무태그 36개 호출 중 own 21, other 15, unresolved 0. tag 58개 호출 중 own 56, other 2(같은 tag 진단만), unresolved 0.

**출력은 모든 호출에서 기대한 정확한 순열과 같았다.** 오결합된 호출도 마찬가지였다. S8과 S1은 같은 계산을 하기 때문이다.
그래서 **출력 일치는 실행 식별의 증거가 될 수 없다**(계획서 §6 E1-D).

**추적기 없는 독립 확인(적대적 검증, `results/v3_followup/e1/tracer_free/`)**. 별도 에이전트 둘이 위 판정을 반박하려고 시도했다.
gdb·LD_DEBUG·저장소 코드는 쓰지 않았다. 두 검증 모두 판정이 **유지**됐다.

- **GOT 직접 읽기**: 평범한 Python 프로세스(TracerPid=0, `LD_*` 없음)에서 호출 뒤 각 라이브러리의 `_mlir_ciface_*`·`main_graph_*` GOT 슬롯을
  ctypes로 읽었다. 주소는 `dladdr`와 `/proc/self/maps`로 DSO에 대응시켰다. 무태그 두 순서 × 두 첫 호출 × {lazy, BIND_NOW}에서,
  나중 로드 라이브러리의 `_mlir_ciface` 슬롯이 먼저 로드된 라이브러리(예: S8+0x2f90)를 가리켰다. 단독과 고유 tag에서는 자기 DSO였다
  (프로세스 33개, 호출 86개).
- **`ud2` 트랩 행렬**: S8·S1 복사본의 entry(E), wrapper(C), compute(M) 시작에 `ud2`를 심었다.
  6개 변형 × 2 로드 순서 × 2 호출 대상 × {lazy, BIND_NOW}로 새 프로세스 48개를 돌렸다.
  - E를 심은 복사본은 자신이 호출될 때만 트랩했다.
  - C나 M을 심은 복사본은 **먼저 로드됐을 때** 어느 쪽을 호출하든 트랩했다. 나중에 로드됐을 때는 자신이 호출돼도 트랩하지 않았고, 출력도 정확했다.
  - 48개 모두 (B)의 예측과 일치했다. "각자 자기 코드" 모델은 32개만 맞았고, 두 모델이 갈리는 16개는 모두 (B) 쪽이었다.
- **정밀화**: 나중 로드 라이브러리의 **entry는 자기 것**이 실행된다. 오결합은 entry가 `_mlir_ciface_*@plt`를 부르는 지점에서 일어난다.
  그 wrapper가 먼저 로드된 라이브러리 **자신의** GOT로 `main_graph`를 부르므로, 나중 라이브러리의 `main_graph` 슬롯은 쓰이지 않는다
  (lazy에서는 끝까지 미해결로 남는다). 표의 entry=own / wrapper=other / compute=other와 같다.


### 3.3 E1-C 고유 tag 빌드

`scripts/run_subgraph_benchmark.py build-tagged`로 빌드했다. 결과는 `results/v3_followup/e1/tagged_build/`에 있다.

- **입력 고정**: 같은 `model.onnx`·`input.npy`·`expected.npy`를 해시 확인 후 사용했다. compile argv는 v3와 같고 `--tag=<tag>` 하나만 더했다.
- **lowering**: tag 표기(심볼 접미사, `compile_options`의 `--tag`, `symbol-postfix`)를 정규화하면 4개 모두 기존 무태그 probe IR과 같다.
- **최종 계산 코드**: 절대 주소·RIP 상대 변위·objdump 주석을 빼면 명령열이 기존과 같다(S8 419개, S1 982개 명령).
  바이트는 배치 때문에 다르다.
- **결정성**: 두 번 빌드한 해시가 같았다.
- **여전히 공유되는 심볼**: tag를 달리해도 런타임 도우미 함수 44개와 데이터 객체 5개(`OM_DATA_TYPE_NAME`, `OM_DATA_TYPE_SIZE`,
  `bufferIndex`, `startReportPrinted`, `timing_nest_level`)는 같은 이름으로 남는다.
  - 공동 로딩 실행에서 실제로 상대 라이브러리로 결합된 것은 15개였다(텐서 도우미 함수 11개와 데이터 객체 4개).
  - 공유 함수는 모두 주소를 빼면 같은 코드다.
  - 계산 함수 `main_graph_<tag>`는 `malloc` 외에 아무것도 호출하지 않는다. 따라서 계산 경로에는 닿지 않는다.
  - 다만 공동 로딩에서는 이것이 **상태·배치 차이(H-state/H-layout)** 로 남는다(`e1/shared_symbols_tagged_*.json`).
    나중 로드 모델의 entry wrapper(`run_main_graph_<tag>`)는 먼저 로드된 모델의 텐서 도우미 코드를 부른다.
    `LD_BIND_NOW`에서는 `getInstrumentFile`, `omTensorDestroy`, `om_f16_to_f32`, `om_f32_to_f16`도 넘어간다(독립 검증).
  - tag 빌드도 `run_main_graph`, `omQueryEntryPoints` 등 tag 없는 이름을 계속 내보낸다. 우리 런타임 경로는 tag 붙은 이름만 쓴다.
    `--tag=NONE`이나 tag 없는 entry를 쓰는 소비자는 검증하지 않았다(guard는 `--tag=NONE`의 `main_graph` 충돌도 거부한다).

### 3.4 회귀 방지

- **guard**: `shapeperf.identity.assert_no_shared_model_symbols`는 모델 고유 심볼(`main_graph_<tag>` 등)을 공유하는 라이브러리를
  한 프로세스에 올리기 **전에** 거부한다. 공동 로딩 worker(`paired.subgraph_pair_worker`)에 넣었다.
  그래서 v3의 scan과 진단 스크립트는 이제 무태그 산출물로는 실행되지 않는다.
- **tag 전달**: 모든 worker가 `tag`를 받아 런타임에 넘긴다. tag 빌드를 tag 없이 열면 런타임이 `omQueryEntryPoints_model`을
  찾지 못해 **요란하게 실패한다**. 조용히 오결합하지 않는다(테스트로 확인).
- **테스트 11개**(`tests/test_loader_identity.py`):
  - C fixture로 RTLD_GLOBAL 오결합을 재현한다(같은 tag: 먼저 로드된 쪽 값, 다른 tag: 각자 값).
  - guard의 거부·허용, worker가 로드 전에 거부하는지와 tag를 전달하는지 확인한다.
  - 실제 L=64 산출물로 무태그 오결합, tag 해결, tag 누락 실패를 확인한다. 이 3개는 산출물·gdb가 없으면 건너뛴다.
  - `--tag=NONE` 형태 라이브러리와 심볼을 읽을 수 없는 라이브러리에 대해 guard가 거부하는지(fail closed) 확인한다.
  - 합성 증거로 판정 로직을 검사한다. own/other를 가르는지, 워커가 끝나지 않으면 호출을 버리지 않고 unresolved로 남기는지,
    LD_DEBUG 교차 확인이 비면 "missing"으로 표시하는지 본다.
  - 이 보강은 코드 리뷰에서 나온 사소한 결함 4개를 고친 결과다. 현재 결과표의 판정은 바뀌지 않았다.

## 4. 원인 판단

| 가설 | 판정 | 근거 | 남은 대안 |
|---|---|---|---|
| H-bind (내부 계산 심볼이 먼저 로드된 라이브러리에 결합) | **공동 로딩(무태그)에서 지지** | gdb PC→DSO, 실행 바이트 해시, LD_DEBUG(lazy·BIND_NOW)가 15/15 호출에서 일치. 고유 tag로 사라지고 같은 tag로 재현됨 | 없음(L=64 K형). 다른 길이·Q형·전체 모델은 같은 메커니즘으로 추정하되 추적하지 않음 |
| H-bind가 단독 실행의 S8/S1 차이를 설명 | **반박** | 단독 실행은 6/6 호출이 자기 코드(+AA 2/2). v3 단독 측정 기록 1,785개도 pid당 모델 하나(2.4절) | — |
| H-state / H-layout | 판단 불가 | E1 범위 밖(시간 측정 없음). 단독 실행 차이의 대안 설명으로 남는다. tag 공동 로딩에서는 공유 런타임 심볼이 남아 있다 | E2 |
| H-policy (lowering 선택이 실행 비용에 영향) | 판단 불가 | 단독 실행의 개발 컨테이너 관측(L=63–65, 96에서 S8이 느림)이 후보로 남는다 | E2–E3 |

"로드 순서가 시간 수준을 정한다"는 v3 관측은 이제 설명된다. **실제로 실행된 계산 코드의 수준**이었다.
S8을 먼저 로드한 프로세스에서는 두 arm 모두 S8 코드를, S1을 먼저 로드한 프로세스에서는 두 arm 모두 S1 코드를 돌렸다.

## 5. v3 해석 정정 (원 관측은 보존)

v3 문서(`docs/STATUS_v3.md`, `design_amendment.md` 8.3절)는 원문을 지우지 않고, 머리에 이 문서를 가리키는 정정 안내만 달았다.

| v3의 문장 | 정정 | 근거 |
|---|---|---|
| "낮은 L의 큰 커널 효과(+12~+45%)는 측정 장치 artifact로 판명, 철회" | **철회를 철회**. 공동 로딩 진단이 무효였다. 단독 실행 비교는 자기 코드를 실행한 비교였다(L=64 추적 확인). 단독 실행의 L=63–65, 96 차이는 **미확정 관측**(개발 컨테이너)이다 | E1-B, 2.4절 |
| "먼저 로드한 `.so`가 수준을 정한다, 원인 미규명" | 원인은 H-bind. 수준은 실제로 실행된 코드의 것이었다 | E1-B |
| "로드 순서를 고정하면 S8/S1 +1~+7%, 모든 구간이 0 포함" | **정책 대조로서 무효**. 두 arm이 같은 계산 코드(먼저 로드된 것)를 실행했다. entry만 다른 사실상의 코드 A/A다 | E1-B |
| "로드 순서를 통제한 효과로는 모델 영향 ≲ 0.01–0.02%" | **철회**(무효 비교에서 나온 값) | 위와 같음 |
| "모델 수준 상한 ≲ 0.05%" | "추출 커널 차이가 모델 안에서도 유지되고 12개가 가산된다는 가정 아래의 1차 산술 추정"으로 부른다. 상한·직접 관측이 아니다. 13번째(마지막 출력) Transpose도 빼지 않는다 | 계획서 §2-5, §9.1 |
| "G1도 프로세스당 arm 하나라 같은 교락을 배제 못한다" | **철회**. G1 기록 450개는 pid마다 모델 하나였다. H-bind는 G1에 해당하지 않는다(다른 상태·배치 요인은 따로 판단) | 계획서 §2-6, 2.4절 |
| "G1의 구조적 결정 조건도 미충족" | 구조적 결정과 코드 변화는 확인했다. 성능 후보와 예산 내 확인 가능성은 확보하지 못했다 | 계획서 §2-2 |
| "이 환경에서는 1% 미만을 볼 수 없다" | 현재 측정 설계와 확보한 표본으로는 그 크기를 신뢰성 있게 판단하기 어렵다 | 계획서 §2-3 |
| "A/A 3/15이므로 구간이 반보수적" | A/A 이상 신호가 있으므로 구간 계산과 측정 설계를 점검해야 한다. 다중 비교·의존성·우연 변동을 검토하기 전에는 실제 coverage를 확정하지 않는다 | 계획서 §2-4 |
| scan 기록(`interleaved_in_process_test.jsonl`) | S8/S1 비교로서 **무효**(S1·AA가 S8 코드를 실행) | E1-B `SCAN-S8-S1-AA` |
| 진단 파일 `context_L64_*`, `loadorder_factorial_*` | 두 라이브러리를 올린 프로세스는 S8/S1 비교로서 **무효**. `*_alone` 변형(모델 하나)만 단독 실행 관측으로 남는다. 파일에 provenance가 없다 | 2.4절 |
| AA 기록의 `artifact_hash` | 파일에서 계산하지 않고 S8 해시를 복사한 값 | 2.1절 |

## 6. 연구 판단 표

| 질문 | 지지 / 반박 / 판단 불가 | 근거 | 남은 대안 설명 | 다음 행동 |
|---|---|---|---|---|
| 실행 구현 식별 | **해결**: 단독 실행과 고유 tag 공동 로딩은 own, 무태그 공동 로딩은 other | E1-B의 3중 증거(gdb, 실행 바이트, LD_DEBUG) + 추적기 없는 GOT 읽기와 `ud2` 트랩 행렬 | L=64 K형만 추적함 | E2에서도 모든 측정 산출물의 식별을 기록 |
| 정책 효과(S8 대 S1) | 판단 불가 | 유효한 비교는 v3의 단독 실행뿐이다(개발 컨테이너, protocol 미동결) | H-state, H-layout, 잡음 | E2 calibration → 동결 → 재측정 |
| lowering 원인 | 판단 불가 | IR·기계어 차이는 확인됐다. 실행 비용과의 연결은 E2 결과에 달렸다 | 명령 수·gather·zmm 차이는 설명 후보일 뿐이다 | E3(조건부) |
| 모델/커널 연구 가치 | 판단 불가 | K형 12개는 모델 시간의 0.1–0.3%(단독 실행 커널 시간 기준 산술)다. 모델 수준 효과는 작을 가능성이 크다 | — | E2 이후 결정. BERT 성능 개선 주장은 하지 않는다 |

## 7. E2 진행 여부와 다음 작업

**진행 가능.** 실행 식별 게이트는 단독 실행 경로에서 통과했다. 공동 로딩은 고유 tag일 때만 통과했다. E2 확인 자료를 모으기 전에 아래를 먼저 한다.

1. **산출물 고정**: E2 비교 산출물을 tag 빌드로 통일한다. 주 쌍은 S8-alpha/S1-bravo, 교환 쌍은 S8-bravo/S1-alpha다.
   L=41(u=1) K형과 Q형 L=64도 같은 tag 체계로 다시 빌드한다(`build-tagged`). 그러면 단독 실행과 공동 로딩 민감도 분석이 같은 바이트를 쓴다.
2. **AA 두 종**: AA-byte는 매 기록에 파일에서 **실제로 계산한** SHA-256을 남긴다(현재는 S8 해시를 복사한다: `cmd_measure` 수정 필요).
   AA-tag는 S8-alpha 대 S8-bravo다.
3. **측정 경계 명시**: 지표 이름은 "Python `session.run` 호출 지연"이다. 입력 wrapping, 출력 버퍼 `malloc`(계산 함수 안), 출력 텐서
   생성·해제가 포함된다. 순수 kernel time이 아니다. 필요하면 C entry 보조 측정을 추가한다.
4. **calibration(탐색 자료)**: 시간 순서별 지연과 drift, timer·wrapper 비용, iteration/process/block 분산, 계층별 비용, 목표 CI 폭별 비용을 잰다.
   calibration 자료는 확인 자료와 분리한다.
5. **`protocol_v3_followup.json` 동결**: 계획서 §12의 필수 항목(scope, provenance, identity 증거, contrast family, 격리·경계, tag·순서·seed,
   반복 수와 근거, 요약·효과·추론, 정밀도 목표, CPU·affinity, 정확성·제외 규칙, 예산·종료 규칙)을 **새 자료 수집 전에** 고정한다.
   예산과 정밀도 목표는 연구자가 정해야 한다.
6. **식별을 측정과 함께 기록**: E2의 모든 측정 프로세스에 산출물 SHA-256, tag, 로드 목록을 남긴다. 공동 로딩 민감도 분석을 한다면
   `verify_execution_identity`로 같은 조합을 먼저 검증한다.

## 8. 미해결·한계

- 추적은 **L=64 K형만** 했다. L=63·65·96, Q형, 전체 모델은 같은 로더·같은 하니스이므로 결과가 같다고 추정할 뿐 추적하지 않았다.
- G1 전체 모델 산출물은 삭제되어 추적할 수 없다. 단독 실행이었다는 사실은 기록(pid)과 코드로만 확인했다. 전체 모델 식별은 E4의 L=64 점검에서 한다.
- `check128` 정확성 검증은 코드가 보존되지 않아 프로세스 구성을 알 수 없다(2.4절).
- 진단 파일(`context_*`, `loadorder_*`)에는 provenance·pid·시각이 없다.
- 단독 실행에서 작은 L의 S8이 왜 느렸는지는 조사하지 않았다(E2–E3).
- 모든 작업은 개발 컨테이너에서 했다. E0·E1에는 시간 수치가 없다.
- 이 계획서 범위 밖의 관측(Gelu의 scalar `tanhf`/`powf` 호출, `results/v3/direction_probe/`)은 이번 작업에서 다루지 않았다.

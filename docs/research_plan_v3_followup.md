# Shape-Perf v3 추가 실험 권고안과 조건부 연구 로드맵

- 작성일: 2026-09-30, Asia/Seoul
- 검토 대상: 사용자 제공 `Shape-Perf_v3_결과보고서 (1).md`, 2026-09-29, G0–G2 [P1]
- 연구 기준: 기존 `Shape-Perf_수정_연구실험안_20260929.md` [P2]
- 문서 성격: **추가 실험의 실행 명세 및 의사결정 권고안**. 새 실험을 수행한 결과 보고서가 아니다.
- 검토 범위: 제공 보고서와 공개된 고정 버전 소스를 대조했다. 비공개 저장소의 현재 harness, 실제 ELF 파일, 원시 timing을 직접 검사하거나 재실행하지는 않았다.

## 1. 현재 결정과 이번 추가 실험의 목적

**연구를 즉시 폐기하지 않는다. 다만 전 범위 성능 측정을 재개할 근거도 아직 없다. 먼저 L=64에서 실행 구현의 식별을 검증하고, 검증된 측정 경로로 제한된 재실험을 수행한다.**

v3 보고서는 컴파일러 개입과 코드 차이를 상당히 구체적으로 확인했다. 그러나 “먼저 로드한 라이브러리가 시간 수준을 결정한다”는 관측의 원인이 해결되지 않았다. 특히 공동 로딩 과정에서 두 세션이 같은 계산 구현을 호출했다면, 공동 로딩 결과로 기존 단독 실행의 차이를 측정 오류라고 판정할 수 없다.

이번 추가 실험은 다음 세 질문에 순서대로 답한다.

1. **실행 식별:** S8과 S1이라고 표시된 측정이 실제로 서로 다른 의도된 계산 코드를 실행하는가?
2. **국소 효과:** 실행 식별을 보장했을 때 같은 shape·입력의 S8/S1 시간 차이가 재현되는가?
3. **연구 가치:** 재현된 차이를 MLIR lowering 결정과 연결할 수 있으며, 기존 지식에 비해 설명·일반화·실용적 의미가 추가되는가?

현재 바로 진행할 범위는 **E0 증거 보존 → E1 실행 식별**이다. E2 이후는 앞 단계 결과에 따라 진행한다. 새 VM, 216개 길이 sweep, D8/D1, 새 모델 도입은 처음부터 일괄 실행하지 않는다.

## 2. v3 결과의 재판정

아래 “확인”은 별도 표기가 없으면 **제공 보고서에 증거가 제시되었다는 뜻**이다. 외부 검토자가 원자료를 재실행했다는 뜻은 아니다.

| 항목 | 현재 판정 | 추가로 필요한 증거 |
|---|---|---|
| cap 8→1의 국소 개입 | 소스·복원·빌드 검증이 보고됨 | 원본 패치, 전체 hash, build argv의 실제 파일 확인 |
| 15개 길이의 lowering 구조 | 약수 규칙 예측과 IR 변화가 보고됨 | 관련 op와 IR 단계의 대응 보존 |
| 최종 코드 차이 | S8/S1 assembly 차이가 보고됨 | 실제 실행된 함수가 이 assembly에 대응하는지 확인 |
| S8/S1 출력 | 30/30 비교에서 비트 동일이 보고됨 | 입력·출력 계보 및 독립 실행 경로 확인 |
| 모델 전체 성능 손해 | 현재 근거로 확인되지 않음 | 유효한 실행 경로, 목표 정밀도, 새 확인 자료 |
| 낮은 L의 큰 커널 효과 | **미확정 관측** | 단독 실행 및 공동 로딩의 실제 호출 구현 확인 |
| 로드 순서 현상 | 관측 자체는 보고됨 | symbol binding, 공유 상태, 배치 효과 중 원인 분리 |
| 컴파일 정보의 성능 예측 기여 | 입증되지 않음 | shape 산술만 사용하는 기준선 대비 추가 가치 |
| 논문 수준 신규 공헌 | 미확정 | 재현 가능한 현상·원인·적용 범위 및 선행연구 대비 |

### 보고서에서 수정할 해석

1. **“큰 커널 효과는 artifact였다” → “로드 순서 의존성이 관측되어 인과 해석을 보류했다.”** 원인 규명 전에는 단독 실행 또는 공동 로딩 중 어느 비교가 잘못되었는지 확정하지 않는다.
2. **“G1의 구조적 결정 조건도 미충족” → “구조적 결정과 코드 변화는 확인했으나, 성능 후보와 예산 내 확인 가능성은 확보하지 못했다.”**
3. **“이 환경에서는 1% 미만을 볼 수 없다” → “현재 측정 설계와 확보한 표본으로는 해당 크기를 신뢰성 있게 판단하기 어렵다.”** 환경 전체의 불가능성으로 일반화하지 않는다.
4. **“A/A 3/15이므로 CI가 반보수적” → “A/A 이상 신호가 있어 구간 계산과 측정 설계를 점검해야 한다.”** 다중 비교, 의존성, 우연 변동을 검토하지 않은 채 CI의 실제 coverage를 확정하지 않는다.
5. **“모델 수준 상한” → “추출 커널 차이가 모델 안에서도 유지된다는 가정하의 1차 산술 추정.”** 전체 모델의 엄밀한 상한으로 사용하지 않는다.
6. **“프로세스당 arm 하나이면 로드 순서 교락”이라는 일반화는 철회한다.** 각 새 프로세스가 해당 arm만 올리는 설계는 유효할 수 있다. 그 프로세스에 다른 모델 코드가 실제로 들어왔는지, 측정 상태가 공정했는지를 별도로 확인한다.

## 3. 최우선 가설: 로드 순서와 심볼 결합

### 3.1 소스에서 확인한 사실

보고서가 사용한 ONNX-MLIR commit은 다음과 같다.

```text
1e017c9fcd7ae7218731ae799f13a957e0cbc80b
```

이 버전의 Linux `ExecutionSession.cpp`는 모델을 `dlopen(..., RTLD_LAZY | RTLD_GLOBAL)`로 연다. entry point는 라이브러리 handle을 전달한 `dlsym`으로 찾는다 [R1]. 같은 버전의 Python runtime 문서는 한 프로세스에서 여러 모델을 쓸 때 서로 다른 model tag를 사용하거나, tag를 생략한다면 **서로 다른 출력 파일명으로 컴파일**하도록 안내한다 [R2].

ELF 동적 로딩에서는 내부의 전역 심볼 참조가 먼저 로드된 객체의 정의에 결합할 수 있다 [R3]. 따라서 다음을 구분해야 한다.

- S8·S1의 entry point 주소가 다르다.
- 두 entry point에서 실제로 도달하는 **계산 함수의 구현**도 각각 S8·S1이다.

첫 번째만 확인해서는 충분하지 않다. symbol export·visibility·relocation·직접 호출 여부에 따라 결과가 달라지므로, 실제 ELF와 호출 경로를 확인해야 한다.

### 3.2 경쟁 가설과 판정 자료

| 가설 | 설명 | 구별할 자료 |
|---|---|---|
| H-bind | 내부 계산 심볼이 먼저 로드된 S8 또는 S1으로 결합 | 실제 호출 대상 주소·소속 DSO·relocation·binding trace |
| H-state | 각각 올바른 구현을 실행하지만 allocator·runtime 전역 상태 등이 공유 | 실행 식별 통과 후 초기화·할당 조건을 통제한 비교 |
| H-layout | 코드·데이터 배치, 정렬, cache 상태 등이 시간 차이에 영향 | 실행 식별 통과 후 tag 교환·주소/정렬 기록·독립 실행 |
| H-policy | 해당 lowering 선택이 실제 실행 비용에 영향을 줌 | 단독 실행 재현, 대조군, IR→기계어→runtime 연결 |

가설은 상호 배타적이지 않다. **공동 로딩에는 H-bind가 존재하고, 단독 실행에는 H-policy가 존재하는 경우도 가능하다.** 출력이 같은 순열이라는 사실과 A/A가 0에 가깝다는 사실만으로 H-bind를 배제할 수 없다.

이 단계에서 관측을 “race condition”으로 부르지 않는다. 단일 스레드의 결정적인 심볼 결합 문제일 수 있으며, 동시 접근에 의한 race의 증거는 제시되지 않았다.

## 4. 벤치마크·컴파일러·환경 범위

### 4.1 기존 자산 유지

| 구성 | 이번 권고 |
|---|---|
| 모델 | 기존 BERT-SQuAD `A_reexport`와 원 모델 계보 유지 |
| 데이터 | 기존 SQuAD 전처리·feature ID·padding 규칙 유지 |
| 컴파일러 | 위 ONNX-MLIR commit 및 기존 LLVM revision 유지 |
| S8 | 원래 scalar Transpose unroll cap 8 |
| S1 | 같은 경로의 cap만 1로 변경. LLVM의 모든 unroll을 끈다는 뜻이 아님 |
| 첫 실험 | 기존 L=64 K형 추출 그래프 및 실제 activation |
| 주 정확성 oracle | Transpose는 입력의 정확한 순열. 모델은 S8/S1 및 기존 ORT 검증 경로 |
| 주 측정 환경 | E1은 기존 개발 환경. 성능 확인 환경 확대는 E2의 비용·정밀도 점검 이후 |

BERT·SQuAD의 출처는 원 논문과 기존 모델 artifact에 둔다 [R9–R11]. 연구자가 추출한 단일 op와 자연 길이별 변형은 **해당 모델에서 유래한 연구용 실험**이지, 공식 MLPerf 평가 결과가 아니다. hidden size, head 수, dtype, permutation을 편의상 변경하지 않는다.

L=64를 먼저 고른 이유는 v3에 단독/공동 로딩 비교와 기계어 자료가 이미 있기 때문이다. “표준 벤치마크 길이”라서가 아니다. 이 선택은 기존 결과를 본 뒤 한 **진단용 선택**이며, 새 shape에 대한 성능 예측 검증으로 계산하지 않는다.

### 4.2 클라우드 개발 도구와 측정 환경의 역할

- Claude Code/Codex 환경: 증거 정리, ELF 검사, tag 빌드, harness 수정, correctness, 실행 식별, calibration 코드 작성.
- 기존 개발 컨테이너: loader 현상 재현과 탐색적 timing. CPU/호스트 통제의 제한을 기록한다.
- 측정용 클라우드 VM: 실행 식별을 통과한 프로토콜의 독립 성능 확인. 새 할당이 다른 물리 호스트임을 보장한다고 가정하지 않는다.

별도 HW 구매는 필요하지 않다. debugger·PMU·CPU affinity 권한은 환경마다 확인한다. PMU를 못 써도 ELF와 함수 경로 검증은 다른 방법으로 시도할 수 있으나, 검증하지 못한 항목을 통과로 처리하지 않는다.

### 4.3 실행 환경의 최소 기록과 통제

| 항목 | 기록·적용할 내용 |
|---|---|
| CPU 및 가상화 | vendor/model/family/stepping, ISA flags, vCPU, SMT/NUMA 노출 상태, VM/container 식별 정보 |
| 실행 제한 | cgroup CPU quota·cpuset, 실제 affinity, 메모리 제한, CPU migration 관측 가능 여부 |
| 스레드 | v3의 1 CPU·1 thread 조건을 유지하되 실제 허용 CPU를 선택. 사용 runtime의 thread 설정과 실제 적용 여부 기록 |
| 소프트웨어 | kernel, libc/loader, Python, ONNX/ORT, native runtime, compiler/linker 버전·전체 hash |
| 부하 | 측정과 compilation/build를 겹치지 않음. 관측 가능한 load·steal time·주파수 상태 기록. 호스트 이웃 부하 미관측 여부 명시 |
| target 호환성 | 기존 compiler flags가 요구하는 ISA와 실제 CPU 일치 여부. compiler의 경고도 보존 |

VM 사양을 같게 신청했다는 사실은 동일 성능이나 독립 물리 호스트의 증거가 아니다. 접근 권한이 없는 governor·주파수·호스트 정보는 `unavailable`로 기록한다. 성능 측정을 위해 임의의 privileged 설정을 강제하지 않는다.

## 5. E0 — 원자료 보존과 실행 명세 복원

**목적:** 기존 결과를 지우거나 원인을 추정으로 덮지 않고, 새 실험이 무엇을 바꾸는지 추적한다.

1. `results/v3/` 및 기존 protocol을 보존하고 추가 결과를 별도 `results/v3_followup/`에 기록한다.
2. 기존 L=64 S8/S1/AA `.so`, 추출 ONNX, activation, compile argv, run argv, stdout/stderr, 시간 기록을 식별한다. 없는 항목은 `missing`으로 표시한다.
3. compiler와 Python native runtime 각각의 실제 파일 경로·전체 SHA-256·의존 라이브러리를 기록한다. 소스 commit만으로 사용된 runtime 바이너리를 보증하지 않는다.
4. 저장소 HEAD, `git diff --binary`, 미추적 구현 파일의 실제 내용, dependency lock, 환경 정보를 보존한다. **hash만으로 사라진 diff를 복원할 수는 없다.**
5. 기존 공동 로딩 harness의 세션 생성·warmup·첫 호출·timed call·출력 해제 순서를 복원한다. `del session`만으로 DSO와 전역 상태가 초기화된다고 가정하지 않는다.
6. 각 프로세스의 모델 로드 목록을 확인한다. fork로 모델 코드가 상속되거나, 검증용 모델·이전 arm이 이미 올라온 경우를 찾아 기록한다.

**통과 조건:** L=64의 입력, 양쪽 artifact, 실제 runtime, 실행 순서를 식별할 수 있다. 기존 정보가 부족해 완전 재현이 안 되면 그 한계를 기록하고, 복원 가능한 새 기준에서 후속 실험을 시작한다. v3의 누락된 provenance를 소급하여 완전하다고 표시하지 않는다.

## 6. E1 — L=64 실행 구현 식별 검증

### E1-A. 기존 artifact의 정적 조사

기존 S8/S1에 대해 다음을 저장한다.

| 자료 | 확인 사항 |
|---|---|
| `readelf --dyn-syms --wide` | export된 entry·계산 함수, GLOBAL/WEAK, visibility, 정의/미정의 구분 |
| `readelf -rW` | 내부 계산 호출과 관련한 relocation 및 외부 심볼 참조 |
| `readelf -dW` | NEEDED, SONAME, RPATH/RUNPATH 등 로딩에 관련된 설정 |
| `readelf -nW` | build ID가 있으면 기록. 없으면 hash로 식별 |
| `objdump -drwC` | entry에서 계산 함수까지의 직접·간접 호출 및 PLT 경로 |
| 실제 compile argv | 명시적 `--tag` 여부, 출력 basename, compiler/linker flags |

두 ELF의 공통 심볼은 **후보 목록**이다. runtime 공용 함수가 공통으로 존재한다는 사실만으로 오동작이라 판정하지 않는다. S8/S1이 달라야 하는 계산 경로와 관련되는지 확인한다.

다음은 도구 명령 예시다. 환경 변수는 E0에서 확인한 실제 경로로 설정하며, 아직 존재하지 않는 분석 스크립트의 실행 명령은 아니다.

```bash
: "${S8_SO:?Set the existing S8 shared-library path}"
: "${S1_SO:?Set the existing S1 shared-library path}"
mkdir -p results/v3_followup/e1/elf
sha256sum "$S8_SO" "$S1_SO" > results/v3_followup/e1/elf/sha256.txt
readelf --dyn-syms --wide "$S8_SO" > results/v3_followup/e1/elf/s8.dynsym.txt
readelf --dyn-syms --wide "$S1_SO" > results/v3_followup/e1/elf/s1.dynsym.txt
readelf -rW "$S8_SO" > results/v3_followup/e1/elf/s8.reloc.txt
readelf -rW "$S1_SO" > results/v3_followup/e1/elf/s1.reloc.txt
readelf -dW "$S8_SO" > results/v3_followup/e1/elf/s8.dynamic.txt
readelf -dW "$S1_SO" > results/v3_followup/e1/elf/s1.dynamic.txt
objdump -drwC "$S8_SO" > results/v3_followup/e1/elf/s8.disasm.txt
objdump -drwC "$S1_SO" > results/v3_followup/e1/elf/s1.disasm.txt
```

### E1-B. 실제 호출 경로 검증

각 조건은 새 `exec`로 시작하는 프로세스에서 실행한다. 하나의 프로세스에서 unload/reload하여 다음 조건으로 재사용하지 않는다.

| case | 모델 로드 순서 | 두 모델을 로드한 뒤 첫 계산 호출 | 반드시 관찰할 것 |
|---|---|---|---|
| I8 | S8만 | S8 | S8 계산 구현 |
| I1 | S1만 | S1 | S1 계산 구현 |
| C81-8 | S8 → S1 | S8 | 이후 양쪽 호출의 실제 계산 구현 |
| C81-1 | S8 → S1 | S1 | 동일 |
| C18-8 | S1 → S8 | S8 | 동일 |
| C18-1 | S1 → S8 | S1 | 동일 |

공동 로딩 case에서는 첫 호출 다음에 반대 arm도 호출한다. 기존 harness가 모델을 하나씩 로드하면서 즉시 warmup했다면 그 순서를 별도 재현 case로 추가한다. load order와 first-call order를 구별하는 이유는 lazy binding 때문이다 [R1, R3].

검증 방법은 실제 바이너리 상황에 맞춰 다음 중 가능한 것을 조합한다.

1. 기본 로딩 설정에서 `LD_DEBUG=bindings`를 사용하여 관련 binding을 기록한다 [R4]. 로그가 없다는 사실만으로 binding 문제가 없다고 판정하지 않는다. 직접 호출은 동적 binding 로그에 나타나지 않을 수 있다.
2. 실제 실행 경로에서 entry와 계산 함수의 주소를 확인하고, 메모리 mapping 또는 `dladdr` 등으로 DSO와 module-relative offset에 대응시킨다. PLT stub 주소를 최종 구현 주소와 혼동하지 않는다.
3. 필요한 경우 debugger에서 entry 이후 호출을 따라가거나 계산 함수에 breakpoint를 둔다. 권한 때문에 못 하면 그 사실을 기록한다.
4. 주소·offset을 E1-A의 S8/S1 assembly에 연결한다. 임의의 함수 포인터를 별도로 `dlsym`한 결과만으로 세션의 실제 호출 경로를 대신하지 않는다.

`LD_BIND_NOW=1`은 결합 시점을 바꾸는 **보조 진단 조건**으로만 사용한다. 기본 lazy-binding 조건의 검증을 대체하지 않는다. tracing·breakpoint·진단 instrumentation을 켠 시간은 성능 결과에 포함하지 않는다.

**최소 기록 단위**

```text
case_id, process_id, requested_arm, load_order, first_call_order
artifact_sha256, runtime_sha256, tag, entry_symbol
actual_compute_module_sha256, actual_compute_symbol_or_offset
binding_or_trace_evidence_path, output_check, identity_verdict
```

`identity_verdict`는 `verified_own`, `verified_other`, `unresolved`로 구분한다. `unresolved`를 성공으로 변환하지 않는다.

### E1-C. 고유 tag로 다시 컴파일

같은 입력 ONNX·기존 flags를 유지하고, S8에는 `--tag=alpha`, S1에는 `--tag=bravo`를 추가하여 **재컴파일**한다. runtime에도 각각 같은 tag를 전달한다 [R2].

```python
# 고정 버전의 공식 Python API를 사용하는 세션 생성 예시.
# path_s8_alpha와 path_s1_bravo는 실제 재컴파일된 파일 경로이다.
from PyRuntime import OMExecutionSession

sess8 = OMExecutionSession(shared_lib_path=path_s8_alpha, tag="alpha")
sess1 = OMExecutionSession(shared_lib_path=path_s1_bravo, tag="bravo")
```

주의 사항:

- 기존 `.so` 파일 이름만 바꾸는 것은 이미 생성된 심볼을 다시 만드는 조치가 아니다.
- tag 두 개는 설명용 명명 선택이다. 동일한 글자 수를 사용해도 코드 배치가 같아진다고 보장하지 않는다.
- 고유 tag를 지정했다는 사실만으로 모든 내부 심볼이 격리되었다고 가정하지 않는다. E1-A/B를 다시 수행한다.
- 동일한 정책의 tag 변경 전후 lowering·계산 구조를 비교한다. tag에 따른 relocation·주소 차이는 보존하고 설명한다.
- tag 수정과 `RTLD_LOCAL`, linker 옵션, allocator 변경을 한꺼번에 적용하지 않는다. 원인 해석이 불가능해진다.

성능 측정으로 넘어가면 **S8-bravo/S1-alpha도 추가 빌드**한다. 한 쌍 안에는 항상 서로 다른 tag가 존재하도록 하며, 정책과 tag의 대응을 교환한 결과도 확인한다. S8-alpha와 S1-alpha를 같은 프로세스에 넣는 비교를 새 정상 설계로 사용하지 않는다.

### E1-D. 필요한 경우에만 사용하는 출력 식별용 진단

원래 S8/S1은 의미가 같으므로 출력만으로 실행 구현을 구별할 수 없다. 호출 추적을 구현하는 과정에서 보조 장치가 필요하면 서로 다른 알려진 출력을 내는 작은 진단 모델 쌍으로 loader 경로를 시험할 수 있다.

이는 **harness 진단 fixture**이며 연구 성능 벤치마크가 아니다. 진단 모델의 성공이 실제 K형 artifact의 올바른 실행을 보장하지도 않는다. 실제 artifact의 trace 검증을 별도로 끝내야 한다.

### E1 판정과 다음 행동

| 관측 | 판정 | 후속 행동 |
|---|---|---|
| 기존 공동 로딩이 상대 arm의 계산 코드를 호출 | 해당 공동 로딩 S8/S1 비교 무효 | 오결합된 symbol·주소 증거 보존, tag 수정 후 재검증 |
| 고유 tag 이후 양쪽 모두 의도된 구현을 실행 | 실행 식별 게이트 통과 | E2 진행 |
| tag 수정 후에도 오결합 | 아직 해결되지 않음 | 계산 심볼 범위 조사. 검증된 단독 실행 경로만 별도 사용 가능 |
| 모든 조건에서 원래부터 올바른 구현 | H-bind가 관측 현상의 설명이라는 근거 없음 | E2에서 state/layout 요인 조사 |
| 실제 호출 경로를 확인하지 못함 | 판단 불가 | 공동 로딩을 주 성능 비교로 사용하지 않음. 성능 규모 확대 보류 |

E1의 실행 횟수는 검정력 계산으로 정하지 않는다. 위 논리적 조건을 빠짐없이 검사하는 것이 목적이며, 조건별 확인에 필요한 실행을 수행한다. 관찰이 불일치하면 그 불일치 원인을 해결하기 위한 반복을 추가한다.

## 7. E2 — 검증된 경로의 측정 설계와 L=64 재측정

### 7.1 주 비교는 새 프로세스의 단독 실행

우선 비교는 **같은 VM·가까운 시간 블록 안에서 S8과 S1을 각각 새 프로세스로 실행**하는 방식으로 둔다. 같은 프로세스에서 두 구현을 비교하는 방식이 자동으로 더 옳거나 더 공정한 것은 아니다.

- 각 측정 프로세스에는 평가할 모델 arm 하나만 올린다.
- S8→S1과 S1→S8의 실행 순서를 균형 있게 배정하고, 그 배정 순서를 무작위화한다.
- 한 블록 안에 양 arm과 사전에 정한 대조군을 포함한다. 모델 로드·입력 준비 시간과 steady-state 추론 시간을 분리한다.
- 프로세스 내부 반복은 독립 실험 표본으로 세지 않는다. 프로세스 요약을 만든 뒤 block의 paired difference를 계산한다.
- 기존 artifact의 단독 실행이 E1을 통과했다면 그 결과도 보존한다. 새 tag artifact와 차이가 있을 경우 정책 효과와 artifact 배치 효과를 구별한다.

공동 로딩은 E1 통과 뒤 **보조 민감도 분석**으로만 추가한다. tag 대응 2종 × load order 2종을 균형 배정하고, timed call order도 균형화한다. 조건별 효과를 먼저 제시한다. 순서에 따라 결과가 달라지는데 전체 평균 하나로 숨기지 않는다.

### 7.2 대조군의 역할을 구별한다

| 대조군 | 확인 목적 | 한계 |
|---|---|---|
| AA-byte | 바이트 동일 S8 복사본을 별도 fresh process에서 비교 | 같은 의미의 서로 다른 코드가 뒤섞이는 현상은 검출하지 못함 |
| AA-tag | 같은 정책으로 alpha/bravo를 각각 컴파일 | tag·심볼·배치 변경에 따른 민감도. 바이트 동일 대조군이 아님 |
| K형 L=41, u=1 | 해당 lowering 개입이 없는 구조적 음성 대조 | tag가 다르면 전체 ELF byte equality를 그대로 요구할 수 없음 |
| Q형 L=64 | 패치 대상과 다른 Transpose 경로 | 실제 IR에서 패치 영향이 없는지 다시 확인해야 함 |

AA-tag는 가능하면 기존의 네 개 L=64 빌드(S8-alpha, S8-bravo, S1-alpha, S1-bravo)를 재사용한다. 대조군 차이가 의심되면 정책 효과의 해석부터 보류한다.

### 7.3 timing 범위와 정확성

최초 주 지표는 현재 harness가 실제 제공하는 **model runtime 호출 지연**으로 명명한다. Python 호출·tensor wrapper·출력 할당/해제가 포함되었다면 순수 kernel time이라고 쓰지 않는다.

1. warmup, timed call, 출력 소비, 출력 해제 위치를 고정하고 양 arm에 동일하게 적용한다.
2. 입력 shape·dtype·stride·contiguity·alignment, output ownership, 매 호출 할당 여부를 기록한다.
3. 작은 커널에서 Python wrapper가 해석을 지배하면 C/C++ runtime entry를 부르는 측정을 보조로 추가한다. 실제 ABI와 ownership을 확인하고, Python 수치와 다른 측정 범위임을 명시한다.
4. 여러 호출을 timer 한 구간에 묶을 경우 호출 수로 나눈 시간을 보고하되 이를 독립 표본 여러 개로 세지 않는다. cache가 데워진 반복 호출 workload임을 명시한다.
5. 연산을 그래프 안에서 인위적으로 수백 번 복제해서 효과를 키우지 않는다. 이는 원래 모델과 다른 workload가 된다.
6. Transpose 출력은 원 입력의 순열과 shape·dtype·byte representation을 확인한다. 비교 전에 finite 여부도 검사한다. 모델 전체의 수치 허용치는 관측 오차 바로 위에 사후 설정하지 않는다.

### 7.4 calibration과 반복 수 결정

warmup 2회, timed 16회, block 5개를 자동으로 계승하지 않는다. 반복 계층의 분산과 비용을 살펴 실험을 배분한다는 원칙은 Kalibera–Jones에 근거하지만, 이 연구의 구체적인 횟수는 별도로 정해야 한다 [R5].

calibration 단계에서 다음을 기록한다.

- 시간 순서별 호출 지연과 초기 drift. steady-state에 도달하지 않으면 평균 하나로 덮지 않는다.
- timer·wrapper 비용의 규모와 측정 범위. 비용을 기계적으로 빼서 kernel time으로 만들지 않는다.
- iteration, process, block 수준의 변동과 block 간 시간 의존성.
- 각 반복 계층을 늘리는 데 드는 실제 비용.
- 목표 CI 폭을 얻는 데 필요한 비용 범위.

calibration은 확인 자료와 구분한다. 정밀도와 비용을 보고 횟수를 정한 뒤, **새 자료 수집 전에** warmup·calls/process·processes/block·blocks·분석법·예산 종료 조건을 동결한다.

독립적이고 대략 안정적인 block 차이라는 단순 근사 아래, log 효과의 CI 반폭 목표를 `h`, block 표준편차를 `s_d`라 두면 `n ≈ (z × s_d / h)^2`를 초기 비용 계산에 쓸 수 있다. 실제 n은 작은 표본, 시간 의존성, 다중 비교, VM 할당 계층을 반영해야 한다. v3의 “69 block”은 새 환경에 그대로 적용하는 보장값이 아니다.

**임의의 1%·5%를 연구 성공 기준으로 발명하지 않는다.** 서비스 요구에서 최소 중요 효과를 정할 수 없으면, 절대·상대 효과와 CI 및 달성 가능한 정밀도를 보고한다. 사전 정의한 실용적 허용구간이 없을 때 CI가 0을 포함한다는 이유만으로 “동등”이라고 쓰지 않는다.

### 7.5 효과 정의와 추론

block b에서 각 arm의 사전에 정한 프로세스 요약을 `T8,b`, `T1,b`라 둔다.

```text
d_b = log(T8,b) - log(T1,b)
relative_effect_percent = 100 × (exp(mean_b(d_b)) - 1)
```

양수는 S8이 느리다는 뜻이다. 절대 차이 `T8,b − T1,b`도 따로 추정하고 CI를 보고한다. 평균 log-ratio를 변환한 효과, 산술 평균 시간의 비, 평균 절대 차이는 일반적으로 같지 않으므로 표의 열 정의를 명시한다.

- 개발 환경의 확인 결과는 해당 환경에 조건부인 추정이다.
- 여러 VM allocation에 관한 주장을 하려면 allocation 계층을 분석에 반영한다. 많은 호출을 적은 allocation 수의 대체물로 사용하지 않는다.
- 최소한 독립적인 새 실행 환경/할당에서 재현을 시도하되, 할당 2개만으로 population-level 정밀도가 충분하다고 주장하지 않는다 [R7].
- 확인할 shape·op·contrast family를 새 확인 자료 전에 고정한다. 기존 계획의 family-wise alpha 0.05와 Holm 절차를 계승한다면 그 선택을 명시한다 [P2, R14]. 보정된 검정과 보정되지 않은 기술적 CI를 혼동하지 않는다.
- 실패·missing·판단 불가 case도 분모와 범위 표에 남긴다. 유의해질 때까지 반복하거나 가장 좋은 tag/order만 선택하지 않는다.

## 8. E3 — 제한된 shape 재확인과 MLIR 원인 분석

### 8.1 확장할 조건

E1이 통과하고 E2에서 비교 가능한 측정 경로와 예산 내 정밀도를 확보했을 때만 다음 범위로 확장한다.

| case | u(L) | 선정 근거 | 성격 |
|---|---:|---|---|
| K, L=63 | 7 | v3에서 큰 단독 실행 차이를 보고한 인접 길이 | 기존 후보 재확인 |
| K, L=64 | 8 | 실행 식별과 calibration의 기준 case | 기존 후보 재확인 |
| K, L=65 | 5 | L=64 이웃의 다른 약수 선택 | 기존 후보 재확인 |
| K, L=96 | 8 | 같은 u=8에서 shape 크기를 바꾼 기존 후보 | 정책 배수만으로 설명되는지 점검 |
| K, L=41 | 1 | 해당 개입이 발생하지 않는 기존 길이 | 구조적 음성 대조 |
| Q, L=64 | 해당 경로 아님 | 패치가 닿지 않는 Transpose | 경로 음성 대조 |

이 집합은 전 shape 분포의 대표 표본도, 미관측 shape의 held-out 집합도 아니다. **기존 이상 현상을 최소 범위로 확인하는 진단 집합**이다. 전 범위 검출률·후보 빈도를 추정하는 데 사용하지 않는다.

### 8.2 MLIR이 담당할 실제 역할

각 case에 대해 다음 증거 사슬을 연결한다.

1. 원래 모델의 node ID, tensor shape/layout, 실제 activation provenance.
2. 고정 소스가 선택한 lowering 경로와 unroll factor [R12, R13].
3. 관련 MLIR 단계의 loop bound·step·unroll 구조. 정규화 전후 IR 모두 보존.
4. 후속 LLVM IR와 최종 계산 함수의 기계어 차이.
5. E1에서 확인한 실제 실행 함수의 module/offset.
6. E2의 같은 shape·입력 S8/S1 효과와 대조군.

명령어 정적 개수, `vgatherdps` 개수, zmm 사용 유무는 **설명 후보**이지 동적 비용 측정치가 아니다. 명령이 더 많으면 느리다고 단정하지 않는다. PMU를 사용할 수 있을 때만 별도 진단 run에서 실제 cycles·instructions 등의 지원 이벤트를 기록하고, sampling/계측이 없는 timing과 구분한다.

코드 경로의 원인을 더 좁히는 추가 compiler 개입은 위 사슬에 구체적인 불확실성이 남을 때만 수행한다. vectorization off, cap 2/4/8 전수 탐색, 여러 LLVM flags 조합을 처음부터 시행하지 않는다. 추가 개입 하나마다 가설·예상 IR 변화·correctness·반증 조건을 먼저 쓴다.

**이 단계의 공헌 후보는 MLIR을 이용해 성능 차이를 설명하고 검증하는 것이다.** 기존 u(L) 산술만으로 IR 변화를 설명할 수 있는 상황에서 “MLIR 정보가 shape보다 성능을 더 잘 예측한다”는 주장을 되살리지 않는다. 그 주장을 별도로 하려면 미관측 case에서 산술 기준선과 비교해야 한다.

### 8.3 아직 남는 load-order 효과의 제한적 조사

올바른 계산 구현을 호출하는데도 load order 효과가 남으면, 한 번에 한 축씩 조사한다.

1. 정책–tag 대응 교환: 효과가 정책을 따라가는가, tag/artifact를 따라가는가?
2. 입력·출력 주소, alignment, allocation lifecycle: 동일한 호출 경계에서 차이를 설명하는가?
3. 첫 호출·warmup 순서: 초기화 또는 cache history와 결합되는가?
4. fresh process와 공동 로딩의 차이: 특정 harness 구성에 국한되는가?

기존 malloc 환경 변수 조정이 실패했다는 사실만으로 allocator 원인을 모두 배제하지 않는다. 기본 ASLR 상태를 먼저 유지하고 기록한다. 시스템 보안 설정을 전역으로 바꾸거나 runtime·linker·allocator를 동시에 수정하는 방식은 주 프로토콜에 넣지 않는다. 설정·배치가 평가를 왜곡할 수 있다는 사실 자체는 이미 선행연구에 알려져 있다 [R6].

## 9. E4 — 전체 모델 관련성과 독립 확인

### 9.1 전체 모델을 다시 측정할 이유부터 확인한다

추출 K형에서 효과가 재현되어도 전체 BERT에 의미 있는 개선이 있다고 바로 주장하지 않는다. 먼저 해당 op의 **모델 내부 비중**과 개입 범위를 확인한다.

- 전역 cap 변경은 12개 K형뿐 아니라 마지막 출력 Transpose 1개에도 닿았다고 보고되어 있다 [P1]. 이 13번째 경로를 효과 연결에서 빼지 않는다.
- 여러 layer의 shape가 같아도 activation·cache·주변 연산은 다를 수 있다. 하나의 추출 op 지연을 12개에 동일 적용하는 것은 가정이다.
- 필요하면 고정 버전이 실제 제공하는 op instrumentation을 확인해 제한적으로 계측한다. 현재 공식 문서의 flags가 고정 revision에도 있다고 가정하지 않는다 [R8].
- 계측 build의 절대 시간을 release build와 섞지 않는다. instrumentation 없는 S8/S1 전체 모델 비교가 최종 효과의 기준이다.

조건부 계산식은 다음 수준으로만 사용한다.

```text
estimated_model_delta ≈ sum_i(standalone_delta_i)
estimated_relative_delta ≈ estimated_model_delta / measured_model_latency
```

이는 개별 op 차이의 전이·가산성을 가정한 탐색 계산이다. cache, 코드 배치, allocation, 주변 연산과의 상호작용을 통제하지 않았으므로 상한이나 직접 관측으로 부르지 않는다.

### 9.2 비용을 제한한 모델 확인

1. 처음에는 이미 연결 자료가 있는 **L=64 한 점**에서 전체 모델 실행 식별과 correctness를 점검한다.
2. 국소 효과와 모델 내부 비중을 근거로 전체 모델에서 달성해야 할 CI 폭을 정한다.
3. 필요한 비용이 예산 안이면 사전 고정한 S8/S1 fresh-process 비교를 수행한다.
4. 필요한 정밀도를 달성할 수 없으면 “전체 모델 효과 판단 불가”로 종료한다. 216개 길이를 넓게 재는 것이 한 점의 정밀도 부족을 해결하지 않는다.
5. 전체 모델 개선을 공헌으로 주장할 단계에만 기존 계획의 전체 SQuAD 정확성 검증 및 최종 answer 차이 확인을 수행한다 [P2].

전체 모델 효과가 작다고 커널 연구가 자동으로 무의미해지는 것은 아니다. 다만 **커널 lowering 연구로 주장 범위를 줄이려면**, 재현 가능한 기전과 다른 실제 case로의 전이 등 별도의 기여가 있어야 한다. BERT 성능 개선이라는 주장을 유지할 수는 없다.

### 9.3 독립 확인과 확장 순서

- 탐색에서 고정한 artifact·입력·분석법으로 새 allocation/시점의 확인 자료를 수집한다.
- 먼저 같은 CPU class에서 재현성을 확인한다. 다른 ISA로 옮기면 새 compiler target과 새로운 실험 축이 추가된다.
- 새 shape 또는 모델 전이는 원인 가설이 예측하는 경우를 **측정 전에** 선정하고 기대 방향을 기록한다.
- 새 compiler revision은 호환성과 해당 source path 변화를 먼저 확인한다. 기존 버전과 조용히 혼합하지 않는다.
- BERT-Large 같은 같은 계열 전이는 다른 아키텍처 전반의 일반화 증거로 계산하지 않는다.
- D8/D1은 static/dynamic 문제가 새 핵심 가설로 구체화될 때만 별도 보조 실험으로 도입한다.

## 10. 연구 지속·축소·중단 로드맵

| 단계 | 진행에 필요한 결과 | 다음 행동 | 멈추거나 범위를 줄일 조건 |
|---|---|---|---|
| E0 증거 보존 | artifact/runtime/input/실행 순서 식별 | E1 | 재현 누락을 명시하고 새 기준만 평가 |
| E1 실행 식별 | 각 arm의 계산 구현을 확인 | E2 | 공동 로딩 미확인·오결합이면 그 경로 사용 중단 |
| E2 L=64 측정 | 대조군·측정 범위·정밀도 계획이 유효 | E3 제한 집합 | 잡음·배치 민감성이 해석을 지배하면 방법 수정 또는 판단 불가 |
| E3 원인 분석 | 유효한 효과와 lowering 연결, 대조군 일관성 | E4 및 독립 확인 | 단순 코드 차이만 남으면 해당 방향의 확대 중단. 효과 부재를 확정하는 것은 아님 |
| E4 모델 연결 | 모델 효과 또는 독립적 가치가 있는 커널 기전 | 선행연구 재심사·제한 전이 | 모델 효과 미확인이면 모델 개선 주장 제외 |
| 최종 연구 심사 | 알려진 사실 이상의 설명·전이·유용성 | 논문/연구 artifact 구성 | 알려진 loader 문제의 재현이면 수정·재현 보고로 정리 |

### 결과별 구체적인 연구 방향

| 추가 실험 결과 | 유지할 수 있는 주장 | 권고 |
|---|---|---|
| 고유 tag가 오결합을 해결하고 이후 정책 효과는 미확인 | 특정 harness 구성의 무효 비교를 찾아 수정함 | harness 수정·regression test·재현 노트로 정리. 신규 논문 주제로 자동 승격하지 않음 |
| 단독 실행의 큰 효과가 올바른 구현에서 재현 | 특정 shape에서 해당 정책 간 실행 차이 | 원인·음성 대조·새 확인 자료 확보 후 lowering 연구 지속 |
| 효과가 tag/주소/로드 순서에 민감하고 정책과 분리되지 않음 | artifact 및 실행 문맥 의존성 | 일반적 정책 우열 주장 보류. 추가 분리가 예산 내 가능한지 판단 |
| 커널 효과는 확실하지만 모델 전체 기여가 미미/미확정 | 제한된 커널 수준의 사실 | 기전·전이의 신규성 평가. 없으면 사례 보고로 종료 |
| 전체 모델 효과와 기전이 재현되고 새 case 예측도 성공 | shape 의존 lowering 손해의 원인 및 적용 범위 | 본 연구 후보. 기존 자동 탐지·성능 버그 연구와 엄격히 비교 |
| 유효한 설계에서도 CI가 넓음 | 예산·환경 범위에서 판단 불가 | 중단. 효과 부재 또는 동등성을 주장하지 않음 |

## 11. 학술적 필요성과 차별점의 재심사

### 11.1 지금 추가 실험이 필요한 이유

E1은 새로운 현상을 억지로 만들기 위한 실험이 아니다. **현재 성능 비교가 실제로 두 구현을 비교했는지**라는 선행 조건을 확인한다. 이 조건이 성립하지 않으면 더 정밀한 통계나 더 많은 VM도 잘못된 비교를 고치지 못한다.

E2는 서로 다른 실행 방식의 결과가 충돌한 원인을 좁힌다. E3는 이미 확인한 MLIR/assembly 변화가 실행 비용과 관련되는지 검증한다. E4는 작은 kernel 결과를 데이터센터 전체 성능의 의미로 과장하지 않도록 한다. 각 단계는 다음 투자 여부를 결정하는 데 필요한 질문 하나를 해결한다.

### 11.2 이미 알려진 것과 새로 입증해야 할 것

| 주장 후보 | 선행 지식 | 이 연구에 추가로 필요한 것 |
|---|---|---|
| 심볼 이름·로드 순서가 호출 구현에 영향 | Linux loader의 문서화된 동작, ONNX-MLIR tag 지침 [R1–R4] | 실제 harness 위반 증거와 수정. 이것만으로 새로운 원리라고 주장하지 않음 |
| 설정·메모리 배치가 benchmark를 왜곡 | Mytkowicz et al., ASPLOS 2009 [R6] | 기존 설명을 넘어서는 구체적 범위·발생 조건·검증 기여 |
| 반복 계층과 불확실성이 중요 | Kalibera–Jones, ISMM 2013 [R5] | 새 통계 기법을 만들었다고 하지 말고 적절히 적용 |
| 클라우드의 성능 변동을 고려해야 함 | Laaber et al., EMSE 2019 [R7] | 현재 workload·환경에서 실제 가능한 정밀도와 한계 |
| compiler가 더 많은 정보로 더 나쁜 코드를 만들 수 있음 | RIDO, PLDI 2024 등 기존 검토 문헌 [R15, P2] | 실제 MLIR lowering의 원인·shape 조건·통제 개입·외부 확인 |
| IR/명령어가 다름 | 컴파일 산출물의 관찰 사실 | 그 차이의 실행상 의미와 분석이 유용한 이유 |

**현재 시점에서 독창성·논문 채택 가능성을 보장할 근거는 없다.** 이 문서는 v3의 타당성 문제를 해결하기 위한 추가 설계다. 새로운 방법론 주제로 전환하려면 기존 계획의 관련 연구를 다시 대조하고, 단일 알려진 loader 동작의 재현을 넘어서는 연구 질문을 별도로 수립해야 한다.

“컴파일러 패스를 만들지 않았다”는 것은 결격 사유가 아니다. 반대로 “MLIR을 사용했다”, “137개 테스트가 통과했다”, “명령어 개수가 크게 달라졌다”는 사실만으로 연구 공헌이 성립하지도 않는다.

## 12. 구현 작업 목록과 완료 기준

아래 경로 중 신규 항목은 **구현 제안**이다. 현재 저장소에 존재한다고 확인한 목록이 아니다. 기존 코드가 같은 역할을 수행하면 재사용하고 중복 framework를 만들지 않는다.

| 우선순위 | 작업 | 제안 산출물 | 완료 기준 |
|---|---|---|---|
| P0 | v3 해석 정정 및 기존 증거 보존 | `docs/followup_v3_review.md` | 원 관측 보존, 미확정/무효/유효 구분 |
| P0 | artifact·runtime·tag inventory | `results/v3_followup/e0/manifest.json` | 전체 hash와 argv, 누락 명시 |
| P0 | 실제 호출 식별 진단 | `scripts/verify_execution_identity.py` 및 필요 시 native helper | entry와 계산 구현의 대응 증거, unresolved 처리 |
| P0 | L=64 고유 tag 빌드 | 기존 build script 확장 | tag만 추가한 비교 가능 artifact, correctness 통과 |
| P0 | loader 회귀 검증 | 기존 tests에 좁은 test 추가 | 실제 오결합/태그 전달 누락을 재현하고 방지하는 테스트 |
| P1 | fresh-process paired 경로 점검 | 기존 paired harness 수정 | 한 프로세스 한 arm, 측정 범위·순서·출력 수명 명시 |
| P1 | calibration 및 protocol 동결 | `protocol_v3_followup.json` | 횟수·예산·추론법의 근거 기록 |
| P2 | 제한 집합과 원인 연결 | `results/v3_followup/e3/` | IR→실행 구현→효과 연결 및 대조군 |
| P3 | 조건부 모델 확인·전이 | 별도 확인 데이터 디렉터리 | 앞선 gate 통과 및 독립 자료 구분 |

테스트는 모든 문서 선택을 코드로 다시 적는 수준으로 늘리지 않는다. 잘못된 artifact 라우팅, 잘못된 tag 전달, 실제 계산 구현 불일치 등 **비교 자체를 무효화하는 오류**를 잡는 데 집중한다.

### Protocol에서 반드시 고정할 항목

```text
scope: exploratory_calibration | confirmatory
compiler/runtime/harness/input/artifact provenance
identity evidence and verified artifact IDs
shape/op/contrast family
process isolation and timing boundary
tag assignment, load order, call order, randomization seed
warmup and repetition counts, with calibration rationale
summary statistic, effect definition, CI/inference method
precision target and practical-effect rationale, if applicable
CPU class, affinity/thread policy, allocation scope
correctness rule and exclusion rule
maximum compute budget, stop rule, confirmation-data boundary
```

시간 예산이나 정밀도 목표가 아직 정해지지 않았으면 E1은 진행할 수 있지만, 이를 빈 값으로 둔 채 확인 실험을 시작하지 않는다. 개발 컨테이너의 기존 시간으로 새 VM 비용을 확정하지 않는다.

### 코딩 에이전트에 전달할 첫 작업 지시

> 기존 v3 원자료와 계획을 보존하고, 이 권고안의 E0와 E1을 먼저 구현·수행하라. 기존 L=64 K형 artifact와 실제 activation을 사용한다. 고정 compiler/runtime을 식별한 뒤 기존 단독/공동 로딩의 실제 계산 함수 호출을 추적하고, 고유 tag로 재컴파일하여 다시 확인한다. entry 주소 또는 출력 동일성만으로 실행 식별 성공을 선언하지 않는다. 진단용 실행을 timing 자료에 섞지 않는다. 새 파일·함수 이름은 저장소 구조를 확인하여 결정한다. 새 VM과 전 범위 sweep은 시작하지 않는다. 완료 시 실제 호출 식별 표, 원인 판단, 미해결 항목, E2 진행 가능 여부와 구체적인 다음 작업을 보고하라. 실행이 막히면 막힌 원인과 가능한 대체 검증을 보고하되 결과를 만들어내지 않는다.

## 13. 다음 결과 제출 형식과 이후 검토 원칙

다음 중간 보고에는 장문의 작업 일지보다 아래 결과를 우선 포함한다.

### 13.1 필수 표

**실행 식별 표**

| artifact/tag 체계 | load order | requested arm | actual compute artifact | output | 판정 | 증거 경로 |
|---|---|---|---|---|---|---|
| 기존 또는 고유 tag | 실제 값 | S8/S1 | SHA-256 및 symbol/offset | pass/fail | verified_own/verified_other/unresolved | trace |

**시간 결과 표 — E1 통과 시에만**

| shape/op | process mode | tag 배정 | S8/S1 지연 | 상대 효과 및 CI | 절대 차이 및 CI | AA/음성 대조 | 자료 지위 |
|---|---|---|---|---|---|---|---|
| 실제 값 | isolated/co-loaded | 실제 값 | 단위·요약 정의 | 분석법 명시 | 분석법 명시 | 결과 | calibration/confirmation |

**연구 판단 표**

| 질문 | 지지 / 반박 / 판단 불가 | 근거 | 남은 대안 설명 | 다음 행동 |
|---|---|---|---|---|
| 실행 구현 식별 |  |  |  |  |
| 정책 효과 |  |  |  |  |
| lowering 원인 |  |  |  |  |
| 모델/커널 연구 가치 |  |  |  |  |

### 13.2 향후 검토의 고정 절차

앞으로 중간 현황이나 결과를 받을 때마다 다음을 함께 작성한다.

1. 직전 계획 대비 완료·변경·미실행 항목.
2. 관측 사실과 원인 해석을 구분한 검토, 필요한 정정.
3. 실험 단위·대조군·정확성·provenance·통계 타당성의 결함 여부.
4. 기존 연구 대비 새로 확보한 공헌과 여전히 부족한 근거.
5. 우선순위가 있는 다음 실험: 목적, 최소 범위, 산출물, 진행/중단 기준.
6. 결과별 연구 지속·축소·전환·종료 로드맵.

검토만 남기고 다음 행동을 생략하지 않는다. 반대로 진척을 만들기 위해 타당성이 약한 연구를 무조건 연장하지 않는다.

## 14. 출처와 설계 선택의 구분

| 내용 | 근거 종류 |
|---|---|
| v3 수치·artifact 구성·현재 약점 | 사용자 제공 보고서 [P1]; 독립 재실행 아님 |
| RTLD_GLOBAL 및 tag 규칙 | 실제 고정 commit의 소스·문서 [R1, R2] |
| 심볼 결합의 일반 동작 | Linux 공식 man-pages [R3, R4] |
| shape별 u(L) | 고정 compiler source와 v3 관측 [R12, R13, P1] |
| benchmark 모델·데이터 계보 | 기존 artifact와 BERT/SQuAD 원 논문 [R9–R11] |
| 계층적 반복·환경 민감성 고려 | 측정 방법론 논문 [R5–R7] |
| E1의 조건 행렬·고유 tag 교환 | 관측과 loader 동작에서 도출한 본 연구의 진단 설계 |
| L=64 우선, 제한된 여섯 case | 기존 관측을 재검증하는 비용 제한 선택; 문헌 표준이 아님 |
| warmup·반복·VM 수 | calibration·정밀도·비용에 따라 사전 고정할 값 |
| 최소 중요 효과 | 요구사항 근거가 있을 때 설정. 현재 임의 수치 없음 |

학술적 타당성은 모든 숫자를 다른 논문에서 복사하는 것이 아니다. 검증 가능한 질문, 적절한 대조, 실험 단위, 데이터 계보, 독립 확인, 설계 선택의 공개를 통해 확보한다. 문헌이 제공하지 않는 숫자를 “학계 표준”이라고 쓰지 않는다.

## 15. 참고문헌·공식 소스

웹·소스 확인일: 2026-09-30. 고정 소스와 current 문서를 구분한다. 참고문헌은 설계 원칙을 뒷받침하며, 각 논문이 본 문서의 횟수·길이 집합을 권고했다는 뜻은 아니다.

- **[P1]** 사용자 제공. *Shape-Perf v3 실행 결과 보고서 (G0–G2)*, 2026-09-29. 제출 파일 `Shape-Perf_v3_결과보고서 (1).md`. 특히 §3.4–3.6, §4.1–4.5, §6–7.
- **[P2]** 기존 협의 문서. *Shape-Perf 수정 연구·실험안*, 2026-09-29. `Shape-Perf_수정_연구실험안_20260929.md`. 추가 실험은 이 문서의 원자료 보존·인과 개입·단계별 중단 원칙을 계승한다.
- **[R1]** ONNX-MLIR. [`src/Runtime/ExecutionSession.cpp`, commit `1e017c9fcd7ae7218731ae799f13a957e0cbc80b`](https://github.com/onnx/onnx-mlir/blob/1e017c9fcd7ae7218731ae799f13a957e0cbc80b/src/Runtime/ExecutionSession.cpp). `loadModel`, `setEntryPoint`, 실행 함수. 파일 blob SHA: `973f6f2d644e9135bedfd7e366837337a3d46116`.
- **[R2]** ONNX-MLIR. [`docs/UsingPyRuntime.md`, 동일 commit](https://github.com/onnx/onnx-mlir/blob/1e017c9fcd7ae7218731ae799f13a957e0cbc80b/docs/UsingPyRuntime.md). *Using model tags*. 파일 blob SHA: `9cdfdf0299c4b80c6e6eea5d2383eb3a3b040422`.
- **[R3]** Linux man-pages. [`dlopen(3)`](https://man7.org/linux/man-pages/man3/dlopen.3.html). RTLD_GLOBAL, symbol reference resolution, namespace 동작. 현재 문서이며 실행 환경의 libc 버전도 기록한다.
- **[R4]** Linux man-pages. [`ld.so(8)`](https://man7.org/linux/man-pages/man8/ld.so.8.html). LD_DEBUG, LD_BIND_NOW. 진단용 사용의 근거.
- **[R5]** Tomas Kalibera and Richard Jones. *Rigorous Benchmarking in Reasonable Time*. ISMM 2013. [DOI](https://doi.org/10.1145/2464157.2464160), [저자 소속기관 기록 및 원고](https://kar.kent.ac.uk/33611/).
- **[R6]** Todd Mytkowicz, Amer Diwan, Matthias Hauswirth, Peter F. Sweeney. *Producing Wrong Data Without Doing Anything Obviously Wrong!* ASPLOS 2009. [DOI](https://doi.org/10.1145/1508244.1508275), [저자 연구실 자료](https://sape.inf.usi.ch/publications/asplos09.html).
- **[R7]** Christoph Laaber, Joel Scheuner, Philipp Leitner. *Software microbenchmarking in the cloud. How bad is it really?* Empirical Software Engineering 24, 2469–2508, 2019. [DOI](https://doi.org/10.1007/s10664-019-09681-1), [저자 자료](https://joelscheuner.com/publication/laaber-19-emse/).
- **[R8]** ONNX-MLIR. [Performance Testing](https://onnx.ai/onnx-mlir/PerformanceTesting.html). **현재 문서**. 고정 revision의 지원 flags를 실행 환경에서 확인한 뒤 사용한다. 이번 확인에서 고정 commit의 `docs/PerformanceTesting.md` 경로는 찾지 못했으므로, 현재 문서와 동일한 CLI가 있다고 보증하지 않는다.
- **[R9]** ONNX Model Zoo. [BERT-SQuAD 모델 자료](https://github.com/onnx/models/tree/main/validated/text/machine_comprehension/bert-squad). 실제 실험은 이미 저장한 원 모델·재수출 모델의 hash를 기준으로 한다. current branch의 새 모델로 교체하지 않는다.
- **[R10]** Jacob Devlin et al. *BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding*. NAACL 2019. [ACL Anthology](https://aclanthology.org/N19-1423/).
- **[R11]** Pranav Rajpurkar et al. *SQuAD: 100,000+ Questions for Machine Comprehension of Text*. EMNLP 2016. [ACL Anthology](https://aclanthology.org/D16-1264/).
- **[R12]** ONNX-MLIR. [`Transpose.cpp`, 고정 commit](https://github.com/onnx/onnx-mlir/blob/1e017c9fcd7ae7218731ae799f13a957e0cbc80b/src/Conversion/ONNXToKrnl/Tensor/Transpose.cpp). `scalarTransposeOverOutputs`의 cap 선택.
- **[R13]** ONNX-MLIR. [`ONNXToKrnlCommon.cpp`, 고정 commit](https://github.com/onnx/onnx-mlir/blob/1e017c9fcd7ae7218731ae799f13a957e0cbc80b/src/Conversion/ONNXToKrnl/ONNXToKrnlCommon.cpp). `getNoLeftoverUnrollFactor`.
- **[R14]** Sture Holm. *A Simple Sequentially Rejective Multiple Test Procedure*. Scandinavian Journal of Statistics 6(2), 65–70, 1979. [논문 기록](https://www.jstor.org/stable/4615733). 기존 계획의 다중 비교 통제 근거.
- **[R15]** Theodoros Theodoridis and Zhendong Su. *Refined Input, Degraded Output: The Counterintuitive World of Compiler Behavior*. PLDI 2024. [DOI](https://doi.org/10.1145/3656404), [공식 프로그램](https://pldi24.sigplan.org/details/pldi-2024-papers/28/Refined-Input-Degraded-Output-The-Counterintuitive-World-of-Compiler-Behavior). 세부 관련 연구 비교는 [P2]도 참조한다.

# 관측기 (K2): perf-sample 백엔드, 충실도 F0

- 범위: `kz anchor`, `kz observe`, `kz hits`, 그리고 그 밑의 `probe/`(앵커, 계획, loader)와 `lib/trace/`(decode, identity). SPEC 의 대응 절은 §5.1(백엔드), §5.5(`.kzt` 실행 컨테이너), §5.6(손실 계수), §5.7(정체성 부트스트랩), §4.13(섭동 계수)이다. 이 문서는 구현에서 정한 세부와 **하지 않은 일**을 적는다.
- 표기: **[M]** 이번 구현에서 실행해 확인, **[D]** 설계 결정, **[I]** 계획·미확인.
- 이 문서의 값 가운데 단위 시험 값은 연구 조건이 아니다. cFS 에 대한 실행 한 번(연습)은 `observer_smoke_run.md` 에 따로 적는다.

## 1. 한 줄 요약

`perf_event_open` 의 uprobe PMU 로, **디스크의 바이너리를 바꾸지 않고** 함수 진입(+4)·복귀·소스 줄 직후에 표본을 받는다. 표본은 CPU 마다 하나의 ring 에 쌓이고, 관측기가 비우면서 시계(CLOCK_MONOTONIC)와 CPU 와 tid 를 함께 기록한다. 커널이 알려 주는 손실(LOST, THROTTLE)은 컨테이너에 그대로 남는다. 라이브러리 libbpf, gdb, 바이너리 수정은 쓰지 않는다 [D].

## 2. 구성

| 위치 | 내용 |
|---|---|
| `probe/anchor/` | 자체 ELF 읽기(`elf_image`), DWARF `.debug_line` 읽기(`dwarf_line`, DWARF 2–5), 기호·줄 → 파일 오프셋(`anchor_resolve`). LLVM 은 쓰지 않는다(`libkz-core`, `tools/kz` 는 LLVM 없이 빌드해야 한다는 규칙). |
| `probe/plan/` | 관측 프로필 읽기(`profile`), basis 컨테이너 만들기·읽기·바이너리 검증(`basis`). |
| `probe/loader/` | `PerfSession`(이벤트 열기, ring, 비우기), 대상 명령 실행(`run_target`), `/proc` 기록(`proc_capture`), 조건 기록(`run_record`), `.kzt` 쓰기(`run_container`). |
| `lib/trace/` | ring 바이트 해석(`decode_perf`), 사이트 슬롯 적용(`decode_sites`), 병합·손실(`decode_merge`), 호출 짝짓기(`decode_match`), 정체성 결합(`identity_join`), 반환 주소 귀속용 기호(`identity_symbols`). 관측 프로그램과 무관한 순수 함수라서 합성 ring 으로 시험한다. |
| `tools/kz/` | `kz anchor`, `kz observe`, `kz hits`. 각 명령은 `cmd_<이름>.cpp` 하나. |
| `include/kz/schema/kz_raw.h` | 관측 레코드(K2 소유). 바뀐 곳은 `schema.md` §9. |
| `profiles/cfs-v7.0.1/observe/sites.kzp` | cFS v7.0.1 의 프로브 계획(데이터). |

엔진(`include/`, `lib/`)에 사례 식별자는 없다. 프레임워크의 이름과 줄 번호는 프로필 파일에만 있다(lint 가 지킨다 [M]).

## 3. 사용법

```
kz anchor <binary> <profile.kzp> -o <basis.kz> [--source-root DIR] [--root NAME=DIR]... [--binary-path PATH]
kz observe <basis.kz> <profile.kzp> -o <run.kzt> [옵션] -- <명령> [인자...]
kz hits <run.kzt> [--site 라벨 --before 라벨] [--group-slot 역할] [--group-return-slot 역할] [--basis basis.kz --group-caller]
```

### 3.1 `kz anchor`

바이너리와 프로필에서 **basis 컨테이너**(앵커 표)를 만든다. 바이너리는 읽기만 한다. basis 에는 다음이 들어간다 [M]:

- BASIS 항목: 바이너리 경로·크기·SHA-256·build-id, 프로필 이름, 줄 표 요약, 앵커 읽기 도구, 소스 루트, 이 빌드의 identity.
- FILE_HASHES: 바이너리의 해시.
- PROVENANCE: 프로필의 `prov` 항목(층, 종류, 파일, 줄 범위, 인용 문구, 본문 해시). `--source-root` 를 주면 인용한 줄을 읽어 해시를 다시 계산하고, 프로필이 선언한 `sha256=` 과 다르면 **basis 를 만들지 않는다**. 루트를 주지 않은 항목은 선언한 해시를 기록만 하고 "검증하지 않음"으로 센다. 해시가 없는 항목은 "해시하지 않음"으로 센다.
- SYMBOLS: 함수 기호(주소, 크기, 파일 오프셋, 이름, 결합). 반환 주소 귀속에 쓴다.
- TARGETS: 사이트마다 프로브 주소(링크 시점 주소, 파일 오프셋, 함수 시작, 함수 안 거리, 줄, 처음 8바이트, 비용 부류).

헤더의 `profile_sha256` 은 프로필 **텍스트**의 해시이다. `kz observe` 는 다른 프로필로 만든 basis 를 거부한다(종료 코드 2).

### 3.2 `kz observe`

1. 프로필, basis 를 읽고 계획을 세운다.
2. 바이너리를 **한 번 열어** 읽고 해시한 뒤 basis 와 대조한다: 크기, SHA-256, build-id, 모든 프로브 주소의 바이트. 하나라도 다르면 **아무것도 붙이지 않고** 종료 코드 2 로 끝난다(`REFUSED`). 프로브는 이 열린 파일 기술자의 `/proc/self/fd/N` 로 지정한다. 해시한 파일과 프로브가 걸리는 파일이 같은 inode 이고, 검사와 부착 사이에 경로가 바뀌어도 영향이 없다 [D].
3. 관측기 자신의 조건을 기록한 뒤(§10), CPU 마다 이벤트를 열고 켠다.
4. 명령을 **관측기의 자식으로** 실행한다. 사용자, 환경, 스케줄링, 작업 디렉터리, 시그널 처리를 바꾸지 않는다. 명령이 다른 사용자로 내려가야 하면 명령이 스스로 한다(`env/run_cfs.sh` 가 그렇게 한다).
5. 명령이 끝나면 프로브를 끄고, ring 에 남은 것을 비우고, 해석한다(병합, 정체성 결합, 호출 짝짓기).
6. `<run.kzt>` 와 사람이 읽는 기록 `<run.kzt>.record.txt`(`--record` 로 바꿀 수 있다)를 쓴다.

옵션:

| 옵션 | 뜻 |
|---|---|
| `-o FILE` | 실행 컨테이너 |
| `--record FILE` | 사람이 읽는 기록(기본 `<run>.record.txt`) |
| `--conditions FILE` | 호출자가 선언하는 조건과 deviation (탭으로 구분; 형식은 `probe/loader/run_record.hpp`) |
| `--binary PATH` | basis 에 적힌 경로가 아닌 파일을 대상으로 한다. 같은 바이트여야 한다 |
| `--ring-pages N` | CPU 마다 ring 의 데이터 쪽 수, 2 의 거듭제곱(기본 512 = 2 MiB) |
| `--poll-ms N` | 비움 thread 가 자는 최대 시간(기본 10 ms) |
| `--drain-cpus LIST` | 비움 thread 를 이 CPU 에만 둔다(기본: 관측기의 자리를 그대로) |
| `--run-ordinal N` | 이 실행의 시리즈 안 번호(기본 0) |
| `--stratum cold|warm|none` | 호출자가 **선언**하는 층(탐지하지 않는다). 절차가 그 층을 **확립**했을 때만 `cold`/`warm` 을 준다. 확립하지 못했으면 `none` 을 주고 이유를 `--conditions` 의 deviation 으로 적는다(RUNMETA 의 값과 deviation 이 어긋나지 않게) |
| `--proc-capture full|status` | 명령이 도는 동안 관측기가 `/proc` 에서 읽는 범위(기본 `full`). `full` 은 프로세스의 status, cmdline, environ, exe·cwd 링크, cgroup, limits 와 thread 마다 stat·status 를 읽고, `status` 는 프로세스의 status 와 thread 의 stat·status 만 읽는다. cmdline·environ 읽기는 대상의 메모리를 복사하므로 대상의 주소 공간 잠금을 잡는다. 어느 쪽이든 읽은 수와 시간이 PERTURBATION `proc-capture` 에 남는다(§10) |
| `--check` | 검증하고 계획을 인쇄한 뒤 멈춘다(아무것도 붙이지 않고 실행하지 않는다) |

`--ring-pages`, `--poll-ms` 는 레퍼런스가 정하지 않은 도구 매개변수이므로 기록에 `unspecified_by_reference` 로 남는다. 기본값은 선택이 아니라 도구의 임시 값이고, S1 에서 정한다 [I].

종료 코드: 0 관측 완료이고 명령도 0 으로 끝남, 10 관측 완료이나 명령이 0 이 아니게 끝남(없는 명령은 127), 1 사용법 오류, 2 거부(basis·프로필·바이너리 불일치), 3 프로브를 붙이지 못함(오류 문구에 시스템 오류와 CPU, 사이트가 있다), 4 컨테이너를 쓰지 못함, 5 관측이 손상됨(해석할 수 없는 stream 등; 컨테이너를 쓰지 않는다). 붙이지 못했을 때 gdb 나 수정한 바이너리로 **조용히 대체하지 않는다**.

### 3.3 `kz hits`

실행 컨테이너의 **개수만** 센다. 사이트별·thread 별 사건 수, 그리고 `--site S --before T` 이면 "시각이 사이트 T 의 첫 사건보다 앞선 S 의 사건 수"를 thread 이름별로, `--group-slot` 이면 S 의 슬롯 값별로, `--group-return-slot` 이면 짝이 된 복귀 사건의 슬롯 값별로 낸다. `--group-slot` 은 thread 이름 × 슬롯 값의 교차 개수도 낸다. `--basis`(실행의 부모 basis)와 `--group-caller` 는 진입 사건의 반환 주소를 함수 이름으로 바꿔 세고, thread 이름 × 호출 함수, (`--group-slot` 이 있으면) 호출 함수 × 슬롯 값의 교차 개수도 낸다(§8). 해석은 하지 않는다. TRUNC 표시가 있는 사건(읽지 못한 슬롯이 0)이 셈에 있으면 그 수를 따로 인쇄한다.

**"앞섬"의 뜻** [D]: 한 시계(`CLOCK_MONOTONIC`)의 병합 순서, 즉 **한 시계의 증거**이다. 서로 다른 thread 사이의 happens-before 가 아니며(SPEC 5.8: 시각에서 HB 간선을 만들지 않는다) 검사기는 이것으로 HB 를 만들지 않는다. 또 사건의 시각은 **프로브가 발화한 시각**이지 그 사이트가 대표하는 프로그램 효과의 시각이 아니다. 호출 직후에 놓인 점 사이트(`after-call`)는 호출이 끝난 뒤에 발화하므로, "T 보다 앞섬"은 "T 의 호출에서 돌아오기보다 앞섬"이고 T 가 대표하는 효과(예: 호출 안에서의 저장)의 시각에 대해서는 **상한**만 준다. `kz hits` 의 출력도 이 두 가지를 적는다.

## 4. 관측 프로필 (`kzp-observe 1`)

관측기가 프레임워크에 대해 아는 것은 이 파일이 전부이고, 항목마다 근거(파일, 줄, 본문 해시)가 있다. 문법은 `probe/plan/profile.hpp` 머리말에 있다.

```
kzp-observe 1
binary <이름>
prov <id> tier=<층> kind=<종류> [side=<면>] file="<경로>" lines=A-B cite="<문구>" [authored="<문구>"] [root=<이름>] [sha256=<64 hex>]
site <라벨> <entry|ret|point> func=<기호> [in=<파일 접미어>] | at=<파일 접미어>:<줄> after-call=<기호>
     [decoder=generic|spawn-create|spawn-return|thread-entry] [prov=<id>] [s0..s3=<원천>:<너비>:<역할>]
```

슬롯의 원천은 레지스터 이름, `stack+<바이트>`, `ip` 이다. 사이트는 최대 네 값(`a[0..3]`)을 낸다. `generic` 사이트는 역할(role)로 읽는다. 엔진이 **위치로** 읽는 곳은 두 군데이고, 계획 검사(`check_site`, `check_plan`; 프로필을 읽을 때와 `make_plan` 에서 둘 다)가 그 위치에 요구한 역할이 선언되지 않은 계획을 **거부**한다(0 으로 해석하지 않는다) [D][M]:

| 읽는 곳 | 요구 |
|---|---|
| `decoder=spawn-create` (진입) | `s1` 역할 `entry_fn`, `s2` 역할 `priority`(4바이트 이하), `s3` 역할 `return_address`(8바이트). `s0`(이름 포인터)은 읽지 않는다 |
| `decoder=spawn-return` (복귀) | `s3` 역할 `return_address` |
| `decoder=thread-entry` (진입) | `s0`(알린 id, 역할 자유, 4바이트 이하) |
| 진입·복귀 짝(같은 `func=` 의 entry 와 ret) | 두 사이트 모두 `s3` 역할 `return_address`(8바이트) — 진입 사이트에서는 스택 꼭대기의 단어, 복귀 사이트에서는 표본의 명령 포인터 |

역할 이름 `entry_fn`, `priority`, `return_address` 는 값이 무엇인지를 말할 뿐 프레임워크의 이름이 아니다(`decode_sites.hpp`). 이 프로필 형식은 준비 상태 명세용 프로필 언어(Φ, `profiles/cfs-v7.0.1/*.kzp` 의 `phi 1;` 파일)가 **아니다**. 같은 사실을 검사기는 Φ 로, 프로브는 이 파일로 따로 갖고 있다. 둘을 하나로 합치는 일은 하지 않았다 [I].

cFS v7.0.1 의 계획은 `profiles/cfs-v7.0.1/observe/sites.kzp` 이다. 사이트 여덟 개: 이벤트 전송 함수의 진입·복귀, SB task 의 AppId 저장 직후, 시작 동기화 함수의 진입·복귀, task 생성의 진입·복귀, 새 task 의 첫 함수(진입). 인용한 줄과 해시는 파일 안에 있고, 이번 실행에서 모두 원문과 일치했다(8개 검증) [M]. SPEC §9.3 S1-R2 에 이 사이트들이 속한다는 진술은 `researcher` 층으로 기록했고, SPEC 이 개정 중이므로 해시하지 않았다.

## 5. 앵커

- **함수 진입**: 함수가 `endbr64` 로 시작하면 그 4바이트 뒤, 아니면 첫 바이트. gcc `-O0` 의 함수는 `endbr64; push %rbp; mov %rsp,%rbp` 로 시작하므로 프로브는 `push %rbp`(진입+4)에 놓인다. 거기서는 인자 레지스터가 그대로이고 스택 꼭대기가 반환 주소이다. 복귀 프로브는 같은 주소를 쓴다(uretprobe).
- **비용 부류**: 진입+4 의 `push %rbp` 는 `emulated`(커널이 명령을 직접 흉내 낸다)로 분류한다. 이 분류는 SPEC 이 인용하는 측정 [R07 §4.7]에 근거한 **표시**이다. 이번 구현은 어떤 비용도 새로 재지 않았다. 나머지는 `unknown`.
- **소스 줄 직후** (`point ... after-call=`): 줄의 기계어를 DWARF 줄 표에서 구한 뒤, 줄의 **마지막 명령이 지정한 함수로의 `call rel32`** 일 때만 그 다음 명령을 앵커로 삼는다. 호출이 마지막 명령이 아니거나, 같은 줄에 조건에 맞는 호출이 둘 이상이면 거부한다. 추측하지 않는다 [D].
- 자체 ELF/DWARF 읽기를 쓴 이유: `tools/kz` 와 `libkz-core` 는 LLVM 을 링크할 수 없고, 필요한 것은 줄 표와 기호 표뿐이다. 읽기 코드는 gcc/clang 이 만든 DWARF 4·5 파일에서 `addr2line` 과 비교했고 [M], 합성 ELF 로 DWARF 2–5, 64비트 형식, 두 번역 단위, 시퀀스 순서, 거부 사례를 시험한다.
- 실제 cFS 바이너리의 앵커 여덟 개: 2026-10-15 의 작업 세션에서 `objdump -d` 와 `addr2line` 으로 보았으나 **그 출력은 저장하지 않았다**. 그래서 이것은 재현할 수 있는 증거가 아니다 [I]. 간접 근거만 남아 있다: basis 에 기록된 프로브 주소의 첫 바이트가 진입 사이트 다섯 곳 모두 `55 48 89 e5`(`push %rbp; mov %rsp,%rbp`)이고 함수 안 거리가 4(=`endbr64` 다음), 줄 사이트의 대상은 `addr2line` 기준 142 줄로 기준 gdb 실행의 `SB_PUB` 중단점(`cfe_sb_task.c:142`)과 같은 줄이며, 반환 주소로 찾은 호출 함수가 기준과 같았다(`observer_smoke_run.md` §5). 이제 `probe/scripts/observe_cfs_smoke.sh` 가 실행 **뒤에** 같은 basis 의 사이트마다 `addr2line -f -i -a` 와 `objdump -d --disassemble=<함수>` 의 해당 줄 앞뒤를 `<tag>_anchor_xcheck.txt` 로 남기고, 같은 단계(`--xcheck`)를 시험 `probe_anchor_xcheck_script` 가 목적 제작 프로그램에서 확인한다 [M]. cFS 바이너리에 대한 저장된 교차 확인은 다음 실행에서 처음 생긴다.

## 6. 커널 인터페이스 [M]

- CPU 마다: 옆 가지(side-band) 이벤트 하나(`PERF_COUNT_SW_DUMMY`; FORK/EXIT, COMM, MMAP2)가 ring 을 갖고, 사이트마다 uprobe 이벤트 하나가 `PERF_EVENT_IOC_SET_OUTPUT` 으로 **같은 ring** 에 쓴다. 그래서 한 CPU 의 기록은 하나의 stream 이고 그 안의 순서가 도착 순서이다.
- uprobe 이벤트: `type` = `/sys/bus/event_source/devices/uprobe/type`, `config` 의 retprobe 비트는 `.../format/retprobe` 에서 읽는다(이 커널에서 type 8). `config1` = 경로 문자열, `config2` = 파일 오프셋. `sample_period = 1`. 표본 필드: `IDENTIFIER, IP, TID, TIME, CPU`, 슬롯이 필요로 하면 `REGS_USER` 와 `STACK_USER`. `sample_id_all` 로 옆 가지 기록에도 같은 꼬리표가 붙는다.
- 이벤트는 **시스템 전체, CPU 별**(`pid = -1`)이다. 대상 파일을 실행하는 **모든 프로세스**의 표본이 들어온다. 명령의 자식 트리로 거르지 않는다. cFS 는 `run_cfs.sh` 의 격리 검사가 다른 인스턴스를 막는다. 해석기는 표본을 낸 pid 와 대상 파일의 MMAP2 로 "추적 대상 프로세스"를 정하고, 다른 프로세스의 옆 가지 기록은 센 뒤 버린다.
- 시계: `use_clockid`, `CLOCK_MONOTONIC`. 모든 CPU 가 한 시계를 쓴다. 시각은 **증거**이고 happens-before 간선이 아니다(SPEC A-CLK-PO). 시계 이름은 기록에 남고, 벽시계 시각은 해시되는 내용에 들어가지 않는다(관찰 시작·끝의 UTC 는 사람이 읽는 기록에만 있다).
- ring: 기본 2 MiB/CPU. `data_head`(acquire)와 `data_tail`(release)로 읽고, 감쌈(wrap)은 해석기가 이어 붙인다. `PERF_FLAG_FD_CLOEXEC` 로 연다 — 대상 명령이 이 기술자를 상속하지 않는다.
- 비움은 별도 thread 가 poll(최대 10 ms)로 한다. 끝날 때 프로브를 끄고, 모든 ring 을 비운 뒤, CPU 마다 **짧은 thread 하나를 그 CPU 에 묶어 이름을 바꾸고 끝내게** 해서 커널이 아직 보고하지 않은 LOST 기록을 쓰게 한다(§7). 그 다음 옆 가지 이벤트를 끈다.
- 프로브를 닫는 데 이벤트마다 약 0.1 s 가 걸린다(커널의 유예 기간 대기; `strace` 로 확인했고, 여러 thread 에서 닫아도 합이 줄지 않았다). 이벤트 수는 사이트 수 × CPU 수이다. 이 시간은 대상 명령이 끝난 뒤에 쓰이고 기록의 `observer.stop_ns` 에 있다 [M].

## 7. 손실 계수 [D][M]

손실은 **커널이 알려 주는 것만** 안다. 이 백엔드에는 다른 손실 탐지기가 없다. 특히 `tseq`(thread 별 순번)는 해석기가 붙이므로 번호의 빈틈은 손실의 증거가 아니다.

- `PERF_RECORD_LOST`: ring 이 가득 차 커널이 기록을 버렸을 때, 다음에 쓸 수 있게 된 기록 앞에 "버린 개수"를 쓴다. **이 개수는 표본만이 아니라, 같은 ring 을 쓰는 모든 기록(옆 가지 기록 포함, 다른 프로세스의 것 포함)을 센다.** 그래서 `LOSS.count` 는 "버려진 프로브 표본의 수의 **상한**"이다 — 기록에서 이름도 `loss.perf_record_lost.dropped_records` 이다. 실제로 1쪽(4 KiB) ring 으로 프로브 40000 번을 돌린 시험에서 `관측된 사건 + 버려진 기록 수`가 40000 이상이고 40000 + 256 이하임을 확인한다(옆 가지 기록이 섞여 정확히 같을 수는 없다) [M].
- 마지막 기록들이 버려졌고 그 뒤에 쓸 일이 없으면 커널은 개수를 보고하지 않는다. 그래서 끝에서 CPU 마다 기록 하나를 일부러 쓰게 한다(§6). 이것이 없으면 마지막 손실이 보이지 않는다 [M].
- `PERF_RECORD_THROTTLE`/`UNTHROTTLE`: 커널이 표본을 세지 않고 버린 구간. 구간의 시작과 길이(ns)를 LOSS 에 남기고, 끝나지 않은 구간은 `tseq_first = 1` 로 표시한다.
- 손실이 하나라도 있으면 RUNMETA 의 `LOSS_PRESENT` 가 켜진다. 해석할 수 없는 stream(잘린 기록 등)도 같다.
- **손실 0 이라는 말의 뜻**: "커널이 손실을 보고하지 않았다". 해석기가 손실이 없음을 증명한 것이 아니다. 이 구분은 컨테이너 안에 설명으로 남아 있다(`kz_raw.h`).
- 호출 짝(`kz hits`, 기록의 `match.*`)이 맞지 않는 경우(진입은 있고 복귀가 없음 등)는 "사건 짝짓기 실패"로 센다. 손실이 있어도 짝이 깨지지 않을 수 있으므로(1쪽 ring 으로 프로브 40000번을 돌린 두 실행 가운데 한 실행은 기록 5902개를 버렸는데도 짝짓기 실패가 0이었고, 다른 실행은 3056개를 버렸고 복귀 없는 진입이 2개였다 [M]) 이 숫자로 손실을 추정하지 않는다.

## 8. 호출 짝짓기와 반환 주소 귀속

- 진입 사건의 `a[3]` 은 반환 주소(스택 꼭대기)이고 복귀 사건의 `a[3]` 은 복귀 시 명령 포인터 — 같은 값이다. thread 마다 호출은 스택처럼 중첩되므로 LIFO 로 반환 주소가 같은 열린 호출과 짝짓는다. 짝이 없는 진입(`entries_without_return`)과 짝이 없는 복귀(`returns_without_entry`)는 기록에 센다. 기호가 같고 진입 하나·복귀 하나이며 **두 사이트가 모두 `s3` 에 `return_address` 를 선언한** 쌍만 짝 후보이다. 선언하지 않은 쌍은 짝짓지 않고 `match.site_pairs_without_key` 로 센다(그런 계획은 관측 전에 `check_plan` 이 거부한다) [D].
- **TRUNC**: 커널이 복사하지 않은 슬롯은 0 이고 사건에 TRUNC 가 켜진다. TRUNC 사건은 그 0 으로 짝짓지 않는다. 혼자 남기고 `match.events_with_unreadable_key` 로 센다(두 실패 수와 따로) [D][M].
- **누가 불렀는지**: 반환 주소에서 적재 편향(bias)을 빼고 basis 의 SYMBOLS 로 함수를 찾는다. 적재 편향은 대상 파일의 MMAP2 와 계획의 사이트 오프셋에서 계산하고, 진입·점 사이트의 표본 명령 포인터(프로브 주소 그대로)와 맞춰 본다(`decode.load_bias ... mismatches 0`). 통합 시험이 `tp_send` 의 호출자 이름(`tp_wrap_a` 등)을 이렇게 되찾는 것을 확인한다 [M].

## 9. 정체성 부트스트랩 (GetAppID 없이)

관측기는 프레임워크의 자체 조회 함수를 대상 프로세스에서 **부르지 않는다**(SPEC 5.7). 세 종류의 증거를 기록하고, 각 사실이 어느 증거로 확립됐는지는 THREADS 의 `flags` 에 남는다. 다만 **어느 생성 호출이 어느 thread 를 만들었는지를 정하는 열쇠는 하나뿐**이다. 나머지는 기록될 뿐 서로 대조되지 않는다 [D]:

1. **생성 관계**: 커널의 FORK 기록이 새 thread 와 만든 thread(`ptid`)를 말한다(`fork-seen`).
2. **생성자 쪽 구간 — 유일한 매칭 열쇠**: task 생성 함수의 진입 사건과 복귀 사건 사이(같은 thread)에 그 FORK 가 시간상 **포함**되면 그 호출이 그 thread 를 만들었다고 본다. 비교하는 시각은 **한 thread 의 것**(생성자의 프로브 사건과, 커널이 그 생성자 thread 에 대해 쓴 FORK 기록)이므로, 이 매칭은 시계 가정 **A-CLK-PO**(SPEC 5.4) 아래에서 성립한다. 그래서 매칭되었거나 모호한 기록에는 `match-by-time` 이 함께 켜진다. thread 사이의 순서는 쓰지 않는다. 한 FORK 를 두 구간이 주장할 수 있거나 한 구간에 후보가 둘이면 **모호**로 보고하고 누구에게도 배정하지 않는다.
3. **자식 쪽**: 새 thread 가 첫 함수에서 알리는 id(`announced_id`), 그리고 thread 가 스스로 정한 커널 이름(COMM). 둘 다 tid 로 붙여 기록할 뿐 생성자 쪽과 **대조하지 않는다**. SPEC 5.7 항목 2 는 생성 함수의 출력 인자(배정된 OSAL id)를 생성자 쪽 증거로 든다. perf-sample 은 포인터가 가리키는 메모리를 읽지 못하므로 이 값을 얻지 못하고, 그래서 알린 id 는 **검증되지 않음**(`id-unverified`)으로 표시된다. 출력 인자와의 대조는 메모리를 읽을 수 있는 백엔드(eBPF; 생성 함수의 복귀에서 `*task_id` 를 읽는다)로 미룬다 [I].

TRUNC 가 켜진 생성·복귀·진입 사건은 열쇠나 id 로 쓰지 않고 `identity.creates_unreadable`, `returns_unreadable`, `entries_unreadable` 로 센다. 복귀를 읽지 못한 생성 호출은 열린 채 남으므로 그 안의 FORK 는 배정되지 않고 모호로 보고된다 [M]. 디코더가 읽는 슬롯을 선언하지 않은 사이트가 들어오면 결합 자체가 오류로 끝난다(§4).

커널 이름은 프레임워크 task 이름을 커널 한계(15자)로 자른 것이다. 접두어가 같은 두 task 는 이름이 아니라 알린 id 로 구분한다. task 이름 **포인터**의 문자열은 읽지 않는다 — perf-sample 은 대상 메모리의 임의 주소를 읽지 못하므로 포인터 값(`name_ptr`)만 보관한다(eBPF 백엔드는 읽을 수 있다 [I]). tid 가 재사용되면 `generation` 을 올린다. 이름이 첫 사건 뒤에 정해지는 경우(thread 가 이름을 붙이기 전에 표본이 나옴)는 `names_after_first_event` 로 센다. 이름이 붙은 시점(`t_name`)은 기록한다.

한계: 이 결합은 실행 한 번의 관측에 대한 것이다. 같은 task 가 다음 실행에서 같은 tid 를 가진다는 뜻이 아니다.

## 10. 관측 효과 기록 (D1)

`kz observe` 는 실행마다 아래를 사람이 읽는 기록(`*.record.txt`)과 컨테이너(BASIS 항목, DEVIATIONS)에 남긴다. 호출자가 선언한 값과 관측기가 직접 잰 값을 구분해 `from-reference` / `deviation` / `unspecified_by_reference` / `declared-assumption` 으로 표시한다.

| 항목 | 기록 이름 |
|---|---|
| 관측기의 사용자·스케줄링·CPU 허용 집합 | `observer.uid`, `observer.scheduling`, `observer.affinity`, `observer.drain_thread_cpus` |
| 관측기의 CPU 시간 | `observer.cpu_ns.*` |
| 도구 매개변수와 시계 | `tool.ring.*`, `tool.drain.*`, `tool.clock` |
| 호스트 | `host.kernel`, `host.online_cpus`, `host.clocksource`, `host.perf_*` |
| 바이너리와 검증 | `binary.*`, `basis.artifact_sha256`, `profile.sha256`, `plan.sites` |
| 명령 | `target.command_line`, `target.start_directory`, `target.exit`, `target.wall_ns`, `target.child_cpu_ms` |
| 추적된 프로세스의 사용자·환경·스케줄링 | `traced.pid*.*`(uid, gid, 그룹, 명령줄, exe, cwd, 능력, cpus_allowed, cgroup, 환경 변수 **이름**과 블록 해시, 한계), `traced.thread.*`(정책, 우선순위, nice, 마지막 CPU, 허용 CPU) |
| 초기 상태 | 호출자가 `--conditions` 로 선언(예: 영속 상태 스냅샷의 이름과 해시) |
| 손실 | `loss.*` |
| 사건 짝짓기 실패 | `match.*` |
| 정체성 결합의 실패·모호, 시간 포함에 기댄 결합 수, 검증되지 않은 id 수 | `identity.*`(`identity.matched_by_time`, `identity.ids_unverified` 포함) |
| 관측기의 `/proc` 읽기(대상이 도는 동안) | `observer.proc_capture.mode`, `.reads`, `.ns`, `.window`(명령 시작 기준), PERTURBATION `proc-capture` |
| 사이트마다 트랩이 세어지는 곳 | `perturbation.site<N>.trap` |
| 시간 | 관측 구간, `trace.window`, `trace.first_exec_mapping_to_last_event_ns`, `observer.stop_ns` |

환경 변수는 **이름과 블록 해시만** 기록하고 값은 기록하지 않는다(비밀 값이 기록에 들어가지 않게). thread 의 `/proc` 기록은 FORK 를 본 직후 비움 thread 가 읽는다. 아주 짧게 사는 thread 는 읽기 전에 사라질 수 있고, 그때는 "읽기 전에 사라짐"으로 기록한다(추측해 채우지 않는다) [M].

**관측기의 `/proc` 읽기도 관측 효과이다** [D]. 이 읽기는 대상이 도는 동안(cFS 에서는 시작 단계, thread 를 만드는 동안) 일어난다. 첫 표본에서 프로세스의 `environ`, `cmdline`(대상 메모리 복사, 주소 공간 잠금), `limits`, `status` 등을, 새 thread 마다 `stat`·`status` 를 읽는다. 읽은 항목 수와 그 읽기에 쓴 시간은 PERTURBATION `kind=proc-capture`(`count`, `ns`)에, 모드와 시간 창은 기록에 남는다. 이 읽기가 cFS 의 시작 순서에 준 영향은 재지 않았다 [I]. `--proc-capture status` 로 메모리 복사 없는 모드를 고를 수 있다.

**PERTURBATION 의 프로브 행** [D]: 진입·점 사이트는 주소마다 트랩 하나, 복귀 사이트는 복귀 트램펄린(`cost_class=ret-trampoline`; 비용은 S1 에서 잴 때까지 모른다). 같은 함수의 진입·복귀 사이트는 **한 중단점 주소를 공유**하므로 진입 트랩은 한 번만 일어난다. 그래서 한 파일 오프셋의 트랩은 그 오프셋의 **첫 진입·점 사이트의 행**에서만 센다. 같은 오프셋의 다른 행은 트랩을 세지 않는다(복귀 행은 트램펄린, 진입 행은 같은 트랩의 반복). 진입·점 사이트가 없는 오프셋의 복귀 사이트는 진입 트랩이 어느 행에도 세어지지 않는다. 사이트마다 이 결과를 기록의 `perturbation.site<N>.trap` 이 문장으로 말한다. 레이아웃은 바뀌지 않았다(버전 1). s1r2_p1 의 컨테이너는 이 규칙 이전의 것이라 복귀 행의 `cost_class` 가 진입 주소의 명령 부류(`emulated`)로 적혀 있다 — 비용을 셀 때 그 세 행(사이트 1, 4, 6)의 트랩을 다시 세면 안 된다.

## 11. `env/run_cfs.sh` 의 명령줄

관측기는 `env/run_cfs.sh` 를 **고치지 않고** 자식으로 실행한다. 스크립트의 인자와 환경은 기준 gdb 실행(`env/run_r2_gdb.sh`)과 같고, `RUN_CFS_WRAP`(gdb)만 없다.

기준 gdb 실행(비교용, `run_r2_gdb.sh` 에서 그대로):

```sh
RUN_CFS_WRAP="gdb -batch -nx -x /home/user/work/procrace/r2_runs/r2_trace.py -ex run --args" \
  CFS_NORMAL_USER=ubuntu \
  /home/user/Shape-Perf/research/cfs_procrace/env/run_cfs.sh operational ci-es-restart-poweron \
  /home/user/Shape-Perf/research/cfs_procrace/env/logs/<tag>_gdb_operational_esrestart.log
```

이 관측기의 실행: `probe/scripts/observe_cfs_smoke.sh <tag> <kz 실행 파일> <run-ordinal>` (root 로, `CFS_NORMAL_USER=ubuntu` 와 함께). `kz` 의 경로는 **인자**로 받는다. 환경 변수로 받으면 그 변수가 cFS 의 환경으로 넘어가기 때문이다(s1r2_p1 에서 `KZ` 가 그렇게 넘어갔다). 스크립트는 호출자가 내보낸 `KZ` 도 지우고, 관측 명령을 `env -u KZ` 로 시작한다. 스크립트가 하는 일(`$KZ_BIN`, `$LOGS`, `$TAG`, `$ORD` 는 그 스크립트의 변수):

```sh
# 0. 도구의 출처: kz 의 원본(include, lib/core, lib/trace, probe, tools/kz, cmake)이 kz 보다 새로우면 거부(낡은 빌드),
#    저장소 commit 과 kairos/ 의 diff($LOGS/${TAG}_source.diff), kz 의 SHA-256, kz 사본(<빌드 루트>/runs/$TAG/kz)
env/tools/persist_snapshot.sh "before $TAG" $CFS_DIR/build-native_std/exe/cpu1 > $LOGS/${TAG}_persist_before.txt
$KZ_BIN anchor $CFS_DIR/build-native_std/exe/cpu1/core-cpu1 profiles/cfs-v7.0.1/observe/sites.kzp \
      -o $LOGS/${TAG}_basis.kz --source-root $CFS_DIR
# 조건 파일: 환경 변수 이름을 기준 실행(r2_g1 의 .meta)과 비교해 다르면 deviation cfs.environment,
#            run.count = 3 (기능 확인; 기준 gdb 실행 수), run.stratum = 확립되지 않음(RUNMETA none)
env -u KZ CFS_NORMAL_USER=ubuntu $KZ_BIN observe $LOGS/${TAG}_basis.kz profiles/cfs-v7.0.1/observe/sites.kzp \
      -o $LOGS/$TAG.kzt --record $LOGS/$TAG.kzt.record.txt --conditions $LOGS/${TAG}_conditions.tsv \
      --stratum none --run-ordinal $ORD \
      -- /home/user/Shape-Perf/research/cfs_procrace/env/run_cfs.sh operational ci-es-restart-poweron \
         $LOGS/${TAG}_perf_operational_esrestart.log
env/tools/persist_snapshot.sh "after $TAG" $CFS_DIR/build-native_std/exe/cpu1 > $LOGS/${TAG}_persist_after.txt
# cFS 가 실제로 받은 환경 이름(.meta)과 선언한 이름의 비교 → driver
$KZ_BIN hits ...   # 사이트·thread 별, --before sb.appid.store 의 app_id·status, 같은 구간과 전체 구간의 호출 함수·event_id → $LOGS/${TAG}_hits.txt
observe_cfs_smoke.sh --xcheck ...   # 실행 뒤: 앵커의 addr2line·objdump 교차 확인 → $LOGS/${TAG}_anchor_xcheck.txt
```

끝에 산출물 전부의 SHA-256 을 driver 에 적는다.

`run_cfs.sh` 가 만드는 cFS 프로세스의 명령은 스크립트의 `.meta` 에 남는다: `launcher=setsid setpriv --reuid=ubuntu --regid=1000 --init-groups --`, `command=./core-cpu1`, 작업 디렉터리 `$CFS_DIR/build-native_std/exe/cpu1`. 한 번에 한 cFS 만 돌릴 수 있다(스크립트의 격리 검사).

## 12. 시험 [M]

빌드 트리는 `/home/user/work/kairos-build/` 아래 (`k2-gcc`, `k2-clang`, `k2-gcc-asan`, `k2-clang-ubsan`).

| 시험 | 확인하는 것 |
|---|---|
| `test_trace_decode_perf` | 합성 ring 바이트: 모든 기록 종류, 경계, 감쌈, 잘림, 알 수 없는 종류 |
| `test_trace_decode_real` | 커널이 실제로 쓴 바이트(고정 표본)로 합성 writer 의 배치를 교차 확인 |
| `test_trace_decode_sites` | 슬롯 적용(레지스터, 스택, ip), 잘림 표시, 디코더가 읽는 슬롯의 검사(`check_site`), 진입·복귀 짝의 열쇠 검사(`check_plan`), 호출 짝짓기(중첩, thread 분리, TRUNC 사건을 0 으로 짝짓지 않음, 열쇠 없는 쌍) |
| `test_trace_decode_merge` | CPU 사이 병합 순서, 적재 편향, 손실·throttle 기록, 추적 대상 선별 |
| `test_trace_identity` | 생성 관계, 구간 포함, 모호, 알린 id, 이름, tid 재사용, `match-by-time`·`id-unverified` 표시, 디코더 슬롯을 선언하지 않은 사이트의 거부, TRUNC 사건을 열쇠·id 로 쓰지 않음 |
| `test_trace_symbols` | 기호 조회와 별칭 선택 |
| `test_probe_elf_dwarf` | ELF/DWARF 읽기(DWARF 2–5, 64비트, 두 번역 단위, 거부 사례)와 앵커 해석기의 거부 사례(모호함, 호출이 줄의 마지막이 아님) |
| `test_probe_anchor_compilers` | gcc/clang, DWARF 4/5 로 만든 같은 프로그램의 앵커. `addr2line` 이나 다른 컴파일러 빌드가 없으면 77 로 끝나 **건너뜀**으로 보고된다 |
| `probe_anchor_xcheck_script` | `observe_cfs_smoke.sh --xcheck` 가 목적 제작 프로그램의 모든 사이트를 `addr2line`·`objdump` 로 찾고, 출력이 두 번 같음 |
| `test_probe_profile` | 프로필 문법과 거부 사례(디코더 슬롯, 진입·복귀 짝의 열쇠 포함) |
| `test_probe_basis` | basis 만들기·읽기, 결정성, 출처 해시, 바이너리 검증의 거부 사례 |
| `test_probe_record` | 조건 파일, `/proc` 읽기(읽은 수, `full`/`status` 모드), CPU 목록, 대상 실행 |
| `test_probe_golden` | basis 와 실행 컨테이너의 정규 dump 와 artifact 해시(합성 입력; `tests/cfs-free/golden/`), PERTURBATION 의 복귀 행·`proc-capture` 행, 트랩 계수 규칙 |
| `test_probe_observe` | **통합**: 목적 제작 C 프로그램에 실제 uprobe 를 붙임 — 인자, 반환 주소 귀속, thread 정체성, 호출 짝, 손실 0; 1쪽 ring 의 손실 계수; 동시 thread; 같은 바이트의 다른 경로; 바뀐 바이트·잘린 파일·다른 프로필·basis 아님의 거부; 종료 코드; `--check`. root 가 아니면 77 로 끝나 **건너뜀**으로 보고된다(통과로 세지 않는다) |

환경: 이 VM(`Linux 6.18.44-fc-v114`, 4 vCPU, root)에서 gcc 13(일반, ASan+UBSan)과 clang 18(일반, UBSan trap) 네 빌드로 전체를 돌렸다. 2026-10-16 의 검토 반영은 `k2rv-gcc`(gcc 13, RelWithDebInfo; 새로 구성)와 `k2-clang`, `k2-gcc-asan`, `k2-clang-ubsan`(증분 빌드)에서 돌렸다. 네 빌드 모두 74개 가운데 73개 통과, 실패 1개는 `lint_case-ids` 로 다른 작업이 고치고 있는 `lib/llvm/ast_fn.cpp` 의 사례 식별자 때문이다(K2 파일이 아니다). 디스크가 가득 차서 `k2-gcc` 는 지웠다. `test_probe_observe` 는 관찰마다 프로브를 닫는 시간(§6) 때문에 약 20 초 걸린다.

## 13. 한계와 하지 않은 일

- **F0 의 뜻**: 디스크의 바이너리와 기호를 바꾸지 않는다. uprobe 는 대상 프로세스의 **메모리 안** 코드 쪽에 중단점 명령을 넣는다(그 프로세스의 페이지가 비공유 사본이 된다). "수정하지 않은 바이너리"는 파일에 대한 말이다.
- **프로브 비용은 재지 않았다.** R-a…R-e(비용), BPF 본문·cookie 백엔드, 앱 `.so` 의 inode 등록 시험, group A 재측정, S1 의 N 결정은 V1b/S1 의 몫이고 하지 않았다. `emulated` 표시는 인용한 측정에 근거한 분류일 뿐이다.
- perf-sample 은 인자 **레지스터**와 스택의 앞쪽 단어만 읽는다. 대상 메모리를 따라가야 하는 값(task 이름 문자열 등)은 읽지 못한다.
- 프로브는 시스템 전체에서 대상 파일을 실행하는 모든 프로세스에 걸린다(§6).
- 서로 다른 thread 의 사건 사이의 시각 순서는 한 시계의 증거이지 happens-before 가 아니다. `kz hits --before` 의 "앞섬"도 그렇고, 점 사이트의 시각은 효과 시각의 상한이다(§3.3).
- 정체성 결합의 생성자 연결은 FORK 시각의 포함 하나에 기대고(A-CLK-PO), 알린 id 는 생성자 쪽과 대조되지 않는다(§9).
- 손실 계수는 상한이고(§7), 손실이 없다는 증명이 아니다.
- 프로필 형식(`kzp-observe 1`)과 Φ 의 관계, `kz anchor` 의 basis 와 앞으로의 LLVM 기반 `kz-static basis` 의 관계는 정하지 않았다. 지금 basis 는 관측용 앵커 표이다.

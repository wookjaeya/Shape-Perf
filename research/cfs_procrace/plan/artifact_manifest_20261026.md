# 외부 검토용 최소 자료 목록 (2026-10-26)

- 근거: IR(`Kairos_OS_Independent_Instrumentation_Review_20261026.md`) §15 단계 A "대표 raw 기록·profile·binary manifest 확보", 앞선 검토(`Kairos_W0W3_Git_Review_20261025.md`) §10.3, 현황 `STATUS_20261026.md` §6 항목 6. 결정: `Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md` §2 [D].
- 지위: **자료 목록**이다. 새 실행이 아니고 결과도 아니다. 모든 기록은 pilot·build-smoke·제어 실행이며 본 실험 결과가 아니다(P8). 수는 `plan/results_pilot_20261025.md` 의 것을 그대로 쓴다.
- 기록은 **저장소에 복사하지 않았다.** 경로는 모두 저장소 밖(`/home/user/work/procrace/…`)이다. sha256 은 2026-10-26 에 이 문서를 쓰며 직접 계산했다 [M]. 저장소 경로는 `research/cfs_procrace/` 기준이다.
- 고른 기록 다섯(IR §15 A, 과제 지정): R2 `v153_pilot3`, C1 smoke3 의 수정 전·후 자연 실행 각 1(`pre_1df262a`, `post_1df262a`, tx), 단계 B 보류 실행 1(`hold1/B_post_1df262a_tx`), confirm2 실행 1(`confirm2/W5_post_1df262a_tx`). 같은 묶음의 다른 인스턴스 중에서 후보 A(`1df262a`)·tx 를 골랐고, confirm2 는 m = 3 재시도를 처음 실현한 W5 를 골랐다(결과표 §5.6.1) — 연구자 선택이며 대표성 주장은 아니다 [D].

## 0. 공통 식별

| 항목 | 값 | 출처 |
|---|---|---|
| 도구 빌드 | `/home/user/work/kairos-build`(kz), `/home/user/work/kairos-build/k1r3-llvm`(kz-static) | `results_pilot_20261025.md` §0 |
| 현재 kz 바이너리 | sha256 `91a75940c80b8dfb79c79d9292a2e6742767c4dced44badd577c95572a3413d2` = 저장소 `e2a86af` 빌드. `e2a86af` 와 `HEAD`(`032ef6c`) 사이 `kairos/` 차이 0 [M: `git diff --stat`] | confirm2 `driver.txt` L2 |
| 컨테이너 헤더 | `basis_sha256` 오프셋 56, `profile_sha256` 88, `tool_sha256` 120(각 32 B), little-endian | `kairos/include/kz/schema/kz_schema.h` L225–L240, `kairos/docs/schema.md` §1 |
| 헤더의 뜻 | 실행 컨테이너(kind 8): profile = **관측 계획 `.kzp` 파일의 sha256**, tool = kz 소스 identity. 재생 컨테이너(kind 12): profile = **Φ 프로필 해시**(`phi::profile_hash`), basis = 실행 컨테이너의 basis 해시 | `schema.md` §1(L146, L172); 아래 값으로 확인 [M] |
| 재생 프로필의 고정 | 재생 프로필(`kairos/profiles/{cfs-v7.0.1,c1-bp-b051eb0}`)은 `71ec15f` 이후 `predict/*.kzm` 세 파일만 바뀌었다(재생 입력 아님) [M: `git diff --stat 71ec15f HEAD -- kairos/profiles`] | — |
| 상수 | `CFE_PLATFORM_ES_MAX_APPLICATIONS=32`, C1 `OS_MAX_API_NAME=20` | `results_pilot_20261025.md` §0 표 "상수" |

**재생 재현 확인 [M]** (2026-10-26, 현재 kz `91a75940…`로 아래 §1–§5 의 재생 명령 `cur` 를 다시 돌림, 출력은 scratch 에만): 다섯 기록 모두 종료 0. confirm2 W5 의 `cur.kz` 는 바이트까지 같다. `71ec15f` 빌드(`d62097a5…`)로 만든 R2·smoke3·hold1 의 `cur.kz` 는 크기가 같고 다른 바이트가 오프셋 120–155(`tool_sha256` 과 `header_crc`)뿐이다. `cur.txt` 는 경로·해시 줄을 빼면 넷 모두 다름 0. `d62097a5…` 바이너리 자체는 보존하지 않았다: 그 판으로 재생하려면 `71ec15f` 를 다시 빌드해야 한다.

## 1. R2 — `v153_pilot3` (natural pilot, cFE v7.0.1, 커널 v153, 손실 0)

| 구분 | 값 |
|---|---|
| 기록 디렉터리 | `/home/user/work/procrace/r2_runs/v153_pilot3/` |
| 기록 kz | 저장소 `0acdda2098f3e294924cb3c939f37fe7eb7f8490`, 미커밋 차이 없음(`v153_pilot3_source.diff` 빈 파일), kz sha256 `40b971622e703437a796a0156fb08193d4de1b96ca579df823f6d1ca373882d5`(`driver.txt` L2–L5) |
| 관측 계획(실행 컨테이너 profile) | `kairos/profiles/cfs-v7.0.1/observe/sites.kzp` sha256 `d514c9531cb1675b3ffd29e18cb04213109e02c544c963d5edecaade843ed9af`(헤더 값과 같음) |
| 실행 컨테이너 헤더 | basis `a4f1f2a398b7550273b9eb8da2cfe199ef9e7e5d7b583d31bca107aafeff303a`, tool `1b39d4dd7f6ea8f32e4a643aa8b30559a96889cfddea6f5b9c633991d292dc97` |
| 재생 kz | 저장소 `71ec15f`, kz `d62097a570c21b98a656ee49e29464a0a7e9ee5eef71bb31916917cf4fc18e2b`(`replay_i1025/sha256.txt`) |
| 재생 프로필 | 디렉터리 `kairos/profiles/cfs-v7.0.1`, Φ 프로필 해시 `ae4d6578b8a88d5bb43436e8ad8fa7a6713bfdf6c6fdfd1146846317b12ba56d`(`replay_i1025/cur.kz` 헤더) |
| 바이너리·빌드 | 기준 빌드 그대로(재계측 없음, F0): `/home/user/work/procrace/cfs_ref/cFS/build-native_std/exe/cpu1/core-cpu1` sha256 `0edb1ae28780160ebee95adf8861fc76fe7119a30434e9cd8698cb505bc97799`. 트리 cFS `088b2fa8`(cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`; `Architecture/spec/SPEC_FINAL.md` L16), CMake 명령 `env/CONDITIONS.md` C3 |

| 파일(디렉터리 기준) | sha256 |
|---|---|
| `v153_pilot3.kzt`(126,416 B, 원 기록) | `242117fb695aa12d8a8fe3be7baab26f06d62b0d15c4fd7f082d022cfcdbad71` |
| `v153_pilot3.kzt.record.txt` | `3447029564d785061d9be9bd67be91ed3cab64180189a68d580e0c25636e76cb` |
| `v153_pilot3_basis.kz` | `a4f1f2a398b7550273b9eb8da2cfe199ef9e7e5d7b583d31bca107aafeff303a` |
| `v153_pilot3_driver.txt`(관측 명령·조건) | `bea88731e7ce011a510c36f8b97dfc0f5fe5ccbf4d7e0fa1c5a8d02c335c75bd` |
| `v153_pilot3_conditions.tsv` | `109cd854c16c9dfd0401d6aaee2f8b9a493bd18885ea2f52cf625d978d3b4080` |
| `replay_i1025/facts.kz` · `static_basis.kz`(정적 닫힘) | `74705f3c95d3222e28f890ae1b1aaa7defe7b33416da429d3dc0975e3cf21568` · `80c2d61b57cca6d31ef36e4eb8cb5cd22eaf352c29a79246f4bb9f5b09484e39` |
| `replay_i1025/cur.kz` · `cur.txt` | `544c7c123078265b9a13d6eb68e1529d2967c00445babb028130f59dd2949fc9` · `75bd70df173649d6d697d6d3ceaca1dce7ed74c83b966beaae37fe255fc2650b` |
| `replay_i1025/p1off.kz` | `2894cecd533126bdfbba68ec75503e21bb0d7bbfa4a81f5da4358a3a352b78c7` |

정적 닫힘 명령: `/home/user/work/procrace/integ_20261025/closures.sh r2`(`kairos/docs/static_facts.md` §2 명령 그대로). 재생 명령(`integ_20261025/replays.sh` 의 `r2run`, `cur` 읽기):

```
R=/home/user/work/procrace/r2_runs/v153_pilot3; O=$R/replay_i1025; SRC=/home/user/Shape-Perf/research/cfs_procrace/kairos
kz replay $R/v153_pilot3.kzt --profile $SRC/profiles/cfs-v7.0.1 --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 \
  --basis $R/v153_pilot3_basis.kz --core-request-caller CFE_ES_CreateObjects \
  --facts $O/facts.kz --static-basis $O/static_basis.kz --compare-narrowing --out cur.kz
```

민감도 읽기는 같은 인자에 `--outcome-narrowing off`, `--owner-identity-targets off`, `--clearing-writes counted|excluded` 를 더한다(같은 스크립트).

## 2–5. C1 공통

| 항목 | 값 |
|---|---|
| 소스 | CF 수정 전 `281a94188cd7d885a5aed01ee041f3ece2b0486f` / 수정 후 `833fdbb27a26f15f2429e79392e7eb73d61abdea`, BP `b051eb0d12bfe76c0230215c2ff851cfaa553ce4`, cFS 후보 A `1df262a64a09f128357482d8f2e06b0fdfe84d37`(cFE `6cccb3f5`, OSAL `f34759d6`, PSP `bd4a2ea2`) — `plan/C1_build_manifest.md` §1 |
| 빌드 | `/home/user/work/procrace/c1_build/<pre|post>/1df262a`, 이탈은 `CFLAGS=-Wno-error=restrict` 하나(P5, `C1_build_manifest.md` §4). compile DB(arch) pre `57d406fd5d05fd2b03570db58644ac93b8490f9eb1cfcc0ab789f69946890b78` / post `2e3879510f06b0ad091eaf10f7331714fdf14a621a9c629d4fc67b2ecfaf2bbf`(같은 문서 §5) |
| 실행한 파일 | 실행마다 `inst/tx/` 에 빌드 산출물을 복사해 실행. 복사본 sha256 = 빌드 산출물(`driver.txt` 의 `file … (build output …)` 줄과 아래 값이 같다 [M]). 보류 실행도 텍스트를 바꾸지 않는다(디버그 레지스터, `kairos/docs/c1_hold_runs.md`) |
| 재생 프로필 | 디렉터리 `kairos/profiles/c1-bp-b051eb0` + `cf-<rev>` + `bundle-1df262a` + `rspec-<rev>`. Φ 프로필 해시: 수정 전 `0c0c6ce00aafc5fee703dec585a2b4a94f6c2a5ff3dcf6c5ef12adaf3d8f1731`, 수정 후 `62d9ef679745cf8b9a52ef0a787d81a2894f80b7caced056c3a38a9b63e46795`(각 `cur.kz` 헤더) |
| 정적 닫힘(`c1prof_i1025`, tx) | `/home/user/work/procrace/c1_build/kz/<cfg>_1df262a/c1prof_i1025/closure.{facts,basis}.kz` — pre `52932a52b4f8c46125a19044e83b4569ce4f62ef024cf5bd2eb327bbdde4cad5` · `684fb5095327a72cdea2faa80fa94df8f5df24ff8db91a779bdbe7ba851994f1`; post `abdeb38d4b5810c7d56f97d2e35a94b947dae70259ba41d9607890d4d62e29de` · `16f2a49a05fda1789d12ca27c025c4f7a0cc5d83a133d213ead5f1894d4d2ba0`. recipe `closure.kzb` pre `56af33d0e39c22940e22f7b54a6fdbecbd6f3abc1a0012f26300f7d81bbaad25`, post `1c43f3535983f9c55efa48d62a79fedc39c358e978f7ea6b7539e91e2e27b28a`. 명령 `integ_20261025/closures.sh c1 <cfg> 1df262a`(kz-static, k1r3-llvm) |

C1 재생 명령(`integ_20261025/replays.sh` 의 `c1run`, confirm2 는 `confirm2/scripts/replays.sh`; 같은 인자, `cur` 읽기). `<cfg>`/`<rev>` = pre/`281a941` 또는 post/`833fdbb`, `<basis>` = smoke3 는 `basis_core-tx.kz`, 보류 실행은 `basis_core.kz`:

```
P=/home/user/Shape-Perf/research/cfs_procrace/kairos/profiles/c1-bp-b051eb0
CL=/home/user/work/procrace/c1_build/kz/<cfg>_1df262a/c1prof_i1025
kz replay $RUN/run.kzt --profile $P --profile $P/cf-<rev> --profile $P/bundle-1df262a --profile $P/rspec-<rev> \
  --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20 \
  --basis $RUN/<basis> --facts $CL/closure.facts.kz --static-basis $CL/closure.basis.kz --compare-narrowing --out cur.kz
```

민감도 읽기는 `--outcome-narrowing off`, `--reads-from-edges off`(± P1 끔), `--owner-identity-targets off`, `--clearing-writes counted|excluded` 이고, confirm2 는 `--emit-trace` 와 `classify.awk`(P11 읽기 A, 저장소 밖)를 더한다.

### 2. C1 smoke3 수정 전 자연 실행 — `smoke3/pre_1df262a` (build-smoke, tx)

- 디렉터리 `/home/user/work/procrace/c1_runs/smoke3/pre_1df262a/`. 기록 kz: 저장소 `5b82047bc778d083a8ddcbb336627fa3c58e65c0`, 미커밋 차이 없음, kz `18cf6bd477d2c363c59c016cc74750bc7d0f0038c1758e5dbaac346ac1d814f9`(`driver.txt` L2–L3). 스크립트 `kairos/probe/scripts/observe_c1_smoke.sh`.
- 관측 계획 `kairos/profiles/c1-bp-b051eb0/observe/sites-cf281a941.kzp` `d375d8535ff920006c41d12e26515df6eaeb694d90d228e72d59bac344245631`. 실행 컨테이너 헤더: basis `7bda4b27…71ecd3`(= `basis_core-tx.kz`), tool `1e87697fea5b5f993416a70a9b74853de7b747665f497791f0475a0e53c1f967`.
- 재생 kz `71ec15f`(`d62097a5…`).

| 파일 | sha256 |
|---|---|
| `run.kzt` | `b23790a3fff0f0e9f0541c3395a0cc1c9ad7d8569498b5b11addbb7e2d9683a5` |
| `run.kzt.record.txt` | `5957ed12c5c319068d60647329d910a98496c70428d0f04b0cbe7f9b84bb8605` |
| `basis_core-tx.kz` · `basis_cf.so.kz` · `basis_bp.so.kz` | `7bda4b27919c8c670c00c5c2956fc9963570312a131f94e10ccc79209171ecd3` · `f25cba235939a8069722f4bcabf9d59655678611db0b4435db2acc7e7feba974` · `5a254ba8995e601213f5950fdd5d888c9116e250c68d87d78ab767de1f17f5a6` |
| `driver.txt` · `conditions.tsv` · `smoke.meta` | `7a56624314520da5d73ca3084904f665899361628a742e5678535757d91bd03a` · `d982b5f3465e1f8cd1a3ed601a4ec991d54c3c0bd8fe27d0f7a8ba1beb2658a1` · `8e382acc93b07e47e3fa13a6bf0f2229d78b427c4e8d9d2c3d8329e2298b4649` |
| `inst/tx/core-tx` · `inst/tx/cf/cf.so` · `inst/tx/cf/bp.so` | `f0b1c0ad7f8076b2d3c1bb4b92a06febba5a11f0be02108d9630e48b3eb32295` · `1d27f27f2554dfed7c823e102dc4f807ffcf22e455d013e6bce6d092f67b0dee` · `334b073b63dee85e1e16ed7d51594aaef7638ba909045a0926c1ae61ef94c75b` |
| `replay_i1025/cur.kz` · `cur.txt` | `70bd384360e5cf3a86a9f558503a17a7c2413f4ac095584e1dc13e53a2287ae0` · `f06f5cadb7a723ab2ec834583dd64e36a8e7286738ea86de89678d55028b48fd` |

### 3. C1 smoke3 수정 후 자연 실행 — `smoke3/post_1df262a` (build-smoke, tx)

- 디렉터리 `/home/user/work/procrace/c1_runs/smoke3/post_1df262a/`. 기록 kz는 §2 와 같다(`5b82047`, `18cf6bd4…`). 관측 계획 `observe/sites-cf833fdbb.kzp` `4f1457903dc761f482981b3da7452d022d8fd33cf046b6fdbef613628b0d1ff9`. 헤더: basis `e7478f40…469b57ac`, tool `1e87697f…1f967`(§2 와 같음). 재생 kz `71ec15f`.

| 파일 | sha256 |
|---|---|
| `run.kzt` | `34567f02a4cc5b0e787ee4b84431a2009b5d666c513d760e8955c71446306920` |
| `run.kzt.record.txt` | `66527a3f47ec2b762e28c1709fd8bffc81b55bef8542b100e92b2354d277a5ee` |
| `basis_core-tx.kz` · `basis_cf.so.kz` · `basis_bp.so.kz` | `e7478f400409bca02999b669d3eac28564636e127a4778d4c5b10365469b57ac` · `f4b7ada1f773919cb1e7577a2e74ea49d3157d0dc7068bc578f5a7fda443f4a2` · `f183a688834ca31f8f2bc3875e5f8d081adbda4369db3c286a638e4ada8762ae` |
| `driver.txt` · `conditions.tsv` · `smoke.meta` | `1b9fbc7adb9bd596a8175667c20d79cd63481d488095b09ecca1e12a97fdf2ff` · `b9be4de94d8defd2ca9dbf0dae03cb3a95412e798e9ed736fa3c6c34c5f38667` · `a02054b7f6829a16a71c0931538306a029ee21ba06325a66a562c94781fe862b` |
| `inst/tx/core-tx` · `cf.so` · `bp.so` | `47436ed429965ff8794e019219095774cbabfa039f1009d7076ad6a5c8bd3364` · `9423ab04489bbdf4871c01cb6b256e6bf2cfb303da6c2df05e1865c77811b4d0` · `334b073b…c94c75b`(§2 와 같음) |
| `replay_i1025/cur.kz` · `cur.txt` | `a329b66b9b75ab5b2af812eb6bbb2a22a02162b1fa672632bbe4f26bb14562f2` · `1cce4ecb97eeeef39df8cfb165d1fe4af3ce0c842a7f3e3663d6128997863464` |

### 4. C1 단계 B 보류 실행 — `hold1/B_post_1df262a_tx` (controlled, P-C1HOLD, E-06)

- 디렉터리 `/home/user/work/procrace/c1_runs/hold1/B_post_1df262a_tx/`. 스크립트 `kairos/probe/scripts/observe_c1_hold.sh`.
- 기록 kz: 저장소 `0ed5c608a235457dcb2c732db2579197a2dee8ed` + 미커밋 차이(`source.diff`), kz `9cba898ad7b145bd009858a9f2665b3070cbadb1e1dfb373dd46a11ffdf35f5c`(`driver.txt` L2–L3). 보류기 소스는 그때 미추적이었고 `1a4867c` 에 커밋됐다. 커밋본과 같은지는 직접 확인하지 않았다 [U](`results_pilot_20261025.md` §0 "관측 당시 도구").
- 관측 계획 `sites-cf833fdbb.kzp` `4f145790…1d0b1ff9`, 보류 계획 `kairos/profiles/c1-bp-b051eb0/hold/hold-cf833fdbb.kzh` `f1215c238dafb2b06cd403742e7821aa267f49ec08da89ddcb5958b20605d20f`. 헤더: basis `6f76575f…7b7cf2d`, tool `2cc5351f487a414d22ba05c419624ec444c867533cdc141e97f11f40ec3b4eae`. 재생 kz `71ec15f`.

| 파일 | sha256 |
|---|---|
| `run.kzt` | `6c6c2fdb89dc0076d19dfbe28fba935d7730f9dfc792e926839e67f1eabafe8e` |
| `run.kzt.record.txt` | `428e59318e079facc35cb4d66c2cb128dbe940213a144c277f080b0fcca70ab0` |
| `hold.kzr` · `hold.meta` · `cf_console.txt` | `a70b5da10f48bfb87423d19fdd72feb6e2c2f23044755b1342d22f238e775d2f` · `a94c232a9575458700552d080f58cb63d8d93fa47b769f8a308126fbc2f84990` · `2aaf42ec0bd5edf371f71d451b51025c95d9f36d79233585b5517323b8c657cc` |
| `basis_core.kz` · `basis_cf.so.kz` · `basis_bp.so.kz` | `6f76575f8aec6c3087ed478c82b9c9d833d7b5c91482d05c7e9e4e8765b7cf2d` · `5b3e154b1ed91badff37d11788c4351cdf4b13e9832c9b2a93251d14d7661f02` · `e7ea6dec6e3c4e28be486a8de6a166229747f15d727a6d3cf3f6ea06f5bc661c` |
| `driver.txt` · `conditions.tsv` · `source.diff` | `e7a2e80f7f6181066866e11288d58b670c8f8eb371fa195804ad50d2d3937ab0` · `ed8ee888843d83d68588b13505cf520d623107f5597318c449eeb87874539bee` · `40da3522500f3a9c167990350017f075b6acba578e8441e179b73a2149fee3e1` |
| `inst/tx/` 의 세 파일 | §3 과 같음(`47436ed4…`, `9423ab04…`, `334b073b…`) |
| `replay_i1025/cur.kz` · `cur.txt` | `0e4ab97cebb5c161c329d00c2cc87233b967b989fcb61652d1f69498b62bff4b` · `70920c8fac02ab126d1fc02becd2c8cd65ef7a19799820e43dd90024f8486841` |

### 5. C1 confirm2 실행 — `confirm2/W5_post_1df262a_tx` (controlled, researcher intervention, outside P-C1HOLD)

- 디렉터리 `/home/user/work/procrace/c1_runs/confirm2/W5_post_1df262a_tx/`. 기록 kz와 재생 kz가 같다: 저장소 `e2a86af8a41ca1176e84175dcfdcaecd0cebb008`, 미커밋 차이 없음, kz `91a75940…13d2`(`driver.txt` L2–L3, `replay_c1025/sha256.txt`). 드라이버 `confirm2/scripts/run_confirm.sh`(저장소 밖).
- 관측 계획 `sites-cf833fdbb.kzp` `4f145790…1d0b1ff9`. 보류 계획(predict2 가 만든 것, 저장소 밖) `/home/user/work/procrace/c1_runs/predict2/post_1df262a/cur/W5.kzh` `e0dfa8747d18f14dff3be92fed2be208def2374c205ba08781f120cc1af0499f`(`kairos/docs/predict.md` §8). 헤더: basis `02d08c4918c1bd9f6384143317bc8bb7e45b36079187b8aded9e6708f0703cb5`(= `basis_core.kz`), tool `f2ab950bc03af116da2c9a37c6509f23fa0f3547d6cbadf1430c5672c7c6ef58`.

| 파일 | sha256 |
|---|---|
| `run.kzt` | `032b07636811b75ef40efb7b385504cd6b433d748db464e89c5e03cb821a378c` |
| `run.kzt.record.txt` | `7b3c41307d6192d9a0081690b7e29e4ba07da91768327a4d6e0c7dca86f72878` |
| `hold.kzr` · `hold.meta` · `cf_console.txt` | `d443ae6e2d3a386d5e7f2d0978e945070c50a679805837b00694c63d5bf9bbbe` · `32cb5eab4dbd28cb9bd69fc423056270d7a89259b397b838cb0236b757911b25` · `d74673bf43a56ccb397cc1b4f88d00d30a9fd5c8f22912678ba59816c029b3c9` |
| `basis_core.kz` · `basis_cf.so.kz` · `basis_bp.so.kz` | `02d08c4918c1bd9f6384143317bc8bb7e45b36079187b8aded9e6708f0703cb5` · `4aaae28a20598aece4e93569ce6554b421a10e960c69c984d4a6cd30b9844dab` · `6fd14e768fbf1365d9b5b1c76689f99d5a8052b03130e17a9a878d76e4fdf69b` |
| `driver.txt` · `conditions.tsv` | `98ece6d79e11fb3424307d50dc7c33051f125bd9a3ca56501c356142a4cb3223` · `c8431d49ed7b9245b923239bf2afe3ec656d7eec9c4c98c11c091914f2239fb2` |
| `inst/tx/` 의 세 파일 | §3 과 같음 |
| `replay_c1025/cur.kz` · `cur.txt` | `62bf050f92f845b6463d96d406e8b5af1854651a2343cb89183f8bae342872cb` · `6c589ce2974b8dc7270da4f12c856b778157dfe7b2f2b3274b32a6c183e03a3b` |
| `replay_c1025/trace.txt` · `class.txt`(P11 읽기 A) | `f78df21b0078b08ffb9d07ef05e66f9b5689b13973acb944b60a24e4c05f5375` · `645138cfaf977eb19ee8759aa4d0c5026de84ad784a52a14bbb333ac309dd816` |

## 6. 공개·비공개 구분

이 문서는 공개 여부를 정하지 않고, 파일이 지금 어디에 있고 무엇을 담는지만 적는다. 실제 공개 범위는 PI 가 정한다(§7).

| 부류 | 해당 | 상태 |
|---|---|---|
| 공개 원천 | NASA cFS·cFE·OSAL·PSP·CF·BP·bplib, intel/tinycbor 의 고정 SHA(§1, §2–5 공통) | 상류 공개 저장소에서 같은 SHA 로 받을 수 있다. 빌드는 이것과 기록된 명령·이탈로 다시 만든다 |
| 저장소 안(커밋으로 고정) | kz·kz-static 소스(기록·재생 커밋), 프로필·관측 계획 `.kzp`·`hold-cf833fdbb.kzh`, 관측 스크립트 `kairos/probe/scripts/`, 이 문서와 결과표 | 저장소 커밋에 있다. 저장소 자체의 공개 여부는 이 문서가 정하지 않는다 |
| 저장소 밖(전달해야 함) | 원 기록 `.kzt`·`.record.txt`, basis `.kz`, `driver.txt`·`conditions.tsv`·`*.meta`, `hold.kzr`, 실행 바이너리 사본 `inst/`, 정적 닫힘, 재생 출력, predict2 계획 `W5.kzh`, 스크립트 `integ_20261025/`·`confirm2/scripts/`(`classify.awk` 포함) | 지금 비공개(이 컨테이너에만 있다). 외부 검토에는 위 sha256 과 함께 따로 전달해야 한다 |
| 넣지 않음 | `/home/user/work/procrace/baselines/` 아래 자료 | 이 목록 밖 |

전달 전 확인한 것 [M] (2026-10-26):

- 기록과 메타에는 **호스트 환경 변수의 이름**이 들어 있다(R2 `.kzt` 의 문자열, `v153_pilot3_driver.txt`, `v153_pilot3_perf_operational_esrestart.log.meta`; C1 `conditions.tsv` 의 `cfs.environment` 항목). 비밀 성격의 이름(토큰·키)에 붙은 값은 `<value not recorded>` 로만 적혀 있다. 값이 기록된 경우는 찾지 못했다(이름 패턴 검색; 모든 파일을 사람이 읽어 확인하지는 않았다).
- 기록에는 호스트 경로, 사용자 이름(`ubuntu`, root), pid, 커널 문자열(`6.18.44-fc-v153`)이 들어 있다.

## 7. 남은 것

- 공개 범위(PI): 환경 변수 이름 목록과 호스트 식별 정보를 그대로 전달할지, 그 항목만 가린 사본을 만들지. 가린 사본은 원 기록과 sha256 이 달라지므로 원본 해시와 가린 항목 목록을 함께 적어야 한다. 레퍼런스(CONDITIONS_POLICY)는 실험 조건을 정하는 규칙이고 자료 공개 범위는 정하지 않는다 [D 필요].
- `71ec15f` 판 kz 바이너리(`d62097a5…`)를 보존하지 않았다. 재생은 현재 빌드로도 같은 판정을 냈지만(위 재현 확인), 기록된 재생 출력의 바이트까지 재현하려면 `71ec15f` 재빌드가 필요하다.
- rx 인스턴스·후보 B 의 기록, confirm1 기록은 이 최소 목록에 넣지 않았다. 필요하면 같은 형식으로 더한다.

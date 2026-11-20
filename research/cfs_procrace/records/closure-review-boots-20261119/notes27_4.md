# 결과 27.4 의 고침에 딸린 주 (2026-11-19, 마지막 부팅 뒤)

결과 문서 `plan/full_stage_results_20261116.md` 27.4 의 `[고침 Gn]` 이 가리키는 묶음(`findings.tsv` 의 group)마다, 본문이 크기(27.3 의 '크기', 10^6 B 아래) 때문에 줄인 근거와 다음 주기로 넘기는 것을 적는다. 수는 모두 `out/review27/`(아래 `rv/`, `scripts/review27.py` 의 한 번 실행, 남긴 파일 7,564 를 keep 목록·평가 머리·빌드 기록과 맞대어 차이 0)와 커밋된 기록에서 온다. 기록, 판정 부분, 평가·기대 스크립트, 기대, 선언, 규칙, `env/tools/chain_check.py` 는 이 단계에서 바꾸지 않는다(평가 절차 12절). 다음 주기의 고침은 모두 lead 가 정하고, 정하면 글을 먼저 커밋한 뒤 코드와 fixture 를 한다.

## G1 기초 1, F10, 64 줄: 연결 점검의 경로 맞춤 (`boot-records-01`, `boot-judgement-01`)

- 덤프: 열 덤프(`obs1/c1cyc_{pre,post}_boot{1..5}/tx/tx.dump.txt`) 모두 `buildid` 줄 일곱(`main`, `module:1,3,4,5,6,7`)이 `note=present` 다. 경로는 `./core-tx`, `./cf/<x>.so` 다(`rv/r1_buildid.tsv`; 부팅마다 `objects 7, present 7`).
- 빌드 트리: `chain_check.py` 의 `build_id()`(readelf -n)로 읽은 빌드 트리(`obs1/c1p2_{pre,post}_derived/ws`)의 id 가 두 빌드 모두 일곱 객체에서 덤프 id 와 같다(`tree_equal 7`). 예: `c1_pre_tx` `exe/tx/cf/cf.so` `c55b464b…`, `exe/tx/core-tx` `619614002e6d…`; `c1_post_tx` `exe/tx/cf/cf.so` `3eb1b8b3…`, `exe/tx/core-tx` `acee9edd…`.
- 맞춤: `chain_check.py` 344–346 행의 `record_path` 는 `p == dpath`, `p.endswith('/' + dpath)`, `dpath.endswith('/' + p)` 만 본다. 덤프의 `./cf/cf.so` 와 빌드 기록의 `exe/tx/cf/cf.so` 는 셋 어느 것도 아니어서 맞는 경로가 0 이다(`rule_match` 는 `--main` 으로 맞춘 `main` 하나뿐). 앞의 `./` 를 떼면 일곱 모두 정확히 하나로 맞는다(`strip_match_one 7`). 규칙 글 `kairos/docs/event_semantics.md` 13.5 (사)('한쪽이 다른 쪽의 `/` 뒤 끝, 정확히 하나')도 같은 틈을 가진다.
- `module:5`: `chain.txt` 에서 `main path=exe/tx/core-tx` 는 confirmed 이고 `module:5 path=./core-tx` 는 같은 dump id(`c1_pre_tx` `619614002e6d…`, `c1_post_tx` `acee9edd…`)인데 `unconfirmed(object not in the build record)` 다. 열 `chain.txt` 모두 `confirmed 1, unconfirmed 6` 이다.
- 영향: 판정 부분은 그 줄을 받아 기초 1 을 `missing(record_insufficient(same build not confirmed))` 로 쓴다(`explain_judge.py` 의 기초 1, 판정 규칙 8.13 의 6). 부팅마다 F1 fail 4(`out/t05b_units.tsv`: 넷 모두 기대 + `basis1_same_build`), F10 fail 46('objects not confirmed: module:1, …, module:7'), 기대 대조 mismatch 64 줄(16 갈래 × 4)이 이 한 입력에서 온다. 판정 부분은 받은 줄에 대해서는 규칙대로다.
- 다음 주기(lead 가 정함): 13.5 (사)의 글과 `record_path` 를 함께 고친다(앞의 `./` 를 떼고 실행 디렉터리 `exe/tx` 에 맞춰 견줌, fixture: `./cf/<x>.so`, `./core-tx`, 같은 끝이 둘인 경우). 이 주기의 연결 점검 출력은 다시 내지 않는다.

## G2 F4 의 79(78): 경로 B 의 찾은 사용에 out id 가 없음 (`boot-records-02`, `boot-judgement-02`)

- 기록: `D1-R0` 의 경로 B 기록(B73, B99)에서 결과가 `found` 인 사용 사건은 부팅마다 79(`c1_pre_tx` 부팅 2 는 78)이고, 모두 `out.read` true, `out.value` null, `field_value` 있음, after-call 값에 `out0` 없음이다(`rv/r2_found_uses.tsv`). 경로 A 의 찾은 사용(A22, A26, `c1_post_tx` A21 의 둘)은 모두 out id 가 있다. 덤프의 그 사건은 after-call 이 status 만 가진다(예: `obs1/c1cyc_pre_boot1/tx/tx.dump.txt` 710 행 `site=28[s0028.use-call] … v=[0x0,-,-,-]`; 계획도 status 만 잡는다).
- 판정 부분: `env/obs1/explain_judge.py`(얼어 있는 `644acc11…`) 1313 행이 status 가 성공 상수와 같으면 out0 이 없어도 `{read: true, value: hx(out0)}` 를 쓰고, 1320 행이 그것으로 `found` 를 준다. 경로 B 는 읽은 값을 `field_value` 에만 둔다(1338–1339 행). 판정 규칙 8.2(경로 A: '객체를 찾아 out id 를 읽었으면')와 8.13 의 14('객체를 찾은 사용')는 `found` 를 out id 를 읽은 사용에 맞춘다. 판정 부분의 결함으로 적고 고치지 않는다.
- 평가 스크립트: `env/obs1/explain_eval.py`(`03b9c95d…`) 452 행이 `fid = out.value`, 454 행이 `fid is not None` 일 때만 후보를 만들고, 457 행이 'no recorded create whose out value is the found id None' 으로 fail 을 낸다. `field_value` 는 이 스크립트에 나오지 않는다. 그래서 이 단위는 늘 fail 이다.
- 셈: `D1-R0` F4 는 `c1_pre_tx` 경로 A pass 2 / fail 0 / not_evaluable 3, 경로 B 0 / 79 / 27(부팅 2: 0 / 78 / 27); `c1_post_tx` A 4 / 0 / 2, B 0 / 79 / 28(`rv/r3_eval_lines.tsv`, 열 부팅). 얼어 있는 수(합 2 / 79 / 30, 4 / 79 / 30)는 낸 그대로 둔다. 평가 절차 14.6 의 첫 항목대로면 이 79 는 셈할 수 없음이고, 6절이 F4 의 fail 에 준 뜻('생성자 집합이나 계획이 완전하지 않다')을 가지지 않는다.
- 합: boot fail 129 = F1 4 + F10 46 + F4 79(`c1_pre_tx` 부팅 2: 4 + 46 + 78 = 128). F1 과 F10 은 G1, F4 는 G2 의 까닭이고, 미리 정한 사용에 대한 판정에서 온 fail 은 없다.
- 다음 주기: out0 이 기록되지 않은 사용에 out read 와 `found` 를 주지 않거나 경로 B 의 성공에 따로 결과를 준다; out id 가 없는 기록의 F4 는 셈할 수 없음으로 둔다(명세 11.5 가 경로 B 의 F3 을 그렇게 두듯이); fixture: out0 이 없는 경로 B 성공 사용.

## G3 쌍마다의 (마)와 기초 6, 쌍 줄의 합집합 대조 (`boot-records-03`, `boot-judgement-03`, `boot-claims-01`)

- `out/t04_pairs.tsv`(두 빌드, 다섯 부팅 같음): 부재 사용의 쌍 18 에서 기초 1·4, (나) 의미, (다)는 18 모두 `missing`, 기초 2·3·5, (가), (나) 한도, (라)는 18 모두 `held` 다. (마) `ma_no_lifetime_end` 는 held 6 / missing 12 이다. held 는 A20 의 use 0 × `BP0_tsem`(aseq 18 생성), use 1 × `BP0_tsem`, use 1 × `BP1_tsem`(aseq 29 생성)이고 두 빌드에 있다. A20 의 `same_actor` 쌍 가운데 missing 은 use 0 × `BP1_tsem` 하나이고 그 근거는 'slot-clear at actor 14 aseq 21 not shown to be no lifetime end of the name' 이다(부팅 1 의 기록 `obs1/c1cyc_pre_explain1/D1-R0/records/c1_pre_tx.A20.bb0951052e63f1bc.actor-14.json` 의 `link.name_binding.pairs`; held 의 근거는 'lifetime-complete yes; 6 lifetime-end sites bound', 'no lifetime end of the name in the interval'). A21 의 쌍과 A20 × `SCH_LAB` 의 쌍은 모두 missing 이다.
- 기초 6 은 held 10 / missing 8 이다. A20 의 `same_actor` 쌍 넷 가운데 use 1 × `BP0_tsem` 하나가 missing('the create is not after the use')이다.
- 대조: `env/obs1/explain_expect.py` 의 `get()`(26–28 행)은 `pair[<생성 자리>].missing_items` 를 그 자리의 모든 쌍의 `missing_items` 의 합집합으로 만든다. 그래서 한 쌍의 차이로는 줄이 어긋나지 않는다. `expect/predictions3.tsv` 33·67 행의 근거는 자리 전체에 (마)의 까닭 하나('slot clears of other allocations inside the pair interval')를 준다. 쌍마다 재면 A20 의 같은 글 쌍 둘(use 0 × `BP0_tsem`, use 1 × `BP1_tsem`)은 (마)와 기초 6 이 held 라 그 근거와 다르다. 14.13 의 7 대로 있는 그대로 적고 기대를 다시 쓰지 않는다.
- 다음 주기: 쌍의 기대를 (사용, 생성 인스턴스, 글의 관계)마다 적는다(평가 절차 5.2 의 '쌍 종류마다 정확한 목록').

## G7 찾은 사용의 연결 사유 (`boot-judgement-04`, `boot-claims-08`)

- 찾은 사용 30(열 부팅의 `D1-R0`: 두 빌드의 A22·A26, `c1_post_tx` A21 의 둘째·셋째)의 `instance_link.reason` 은 모두 'link_unlinked(no create of the id in the record)' 이다. 같은 기록의 `publish_events` 에 raw out 이 찾은 id 와 같은 생성이 있다: A22·A26 은 `soft_timebase_Init -> OS_TimeBaseCreate`(`read=False`, 'the success path line of OS_TimeBaseCreate is missing(an implication between conditions is not decided)'), `c1_post_tx` A21 은 `initialize_throttling -> OS_CountSemCreate`(`read=False`, 'the success path line of OS_CountSemCreate is missing(the stored value is not tied to the success path)') (`rv/r2_found_uses.tsv`).
- 판정 부분은 성공 경로 줄이 `held` 인 생성에만 세대를 연다(`explain_judge.py` 1375 행의 `_out`, 1456 행의 `gens`; 1513 행이 세대가 없을 때 그 글을 쓴다). 그 줄은 빌드마다 0 / 14 다(27.4 (파) 2). F4 는 생성 사건의 out 값(raw 포함)을 읽으므로(평가 절차 14.6) pass 다. 명세 11 의 4 와 26.4 열린 점 3 이 그 글을 다음 개정까지 두었다.
- 다음 주기: '세대 없음: 생성자의 성공 경로 줄이 held 가 아님' 같은 따로 된 사유를 판정 고침 전에 글로 커밋한다.

## G16 시험되지 않은 꼴 넷 (`boot-claims-13`)

26.9 `fix-judge-04` 의 넷은 얼어 있는 fixture(`records/closure-prebootfix-20261119/freeze.tsv`: `test_explain4.py` 206 / 206, `test_explain_eval.py` 116 / 116)에 없다.
- (가) (1)의 요구가 있고 대안 문맥이 둘 이상이며 sidecar 문맥 줄이 없는 대조 행: (1)의 요구는 A20 과 고친 빌드 A21 의 '(1) context 1' 하나씩뿐이다(`out/t06_discriminate.tsv`).
- (나) (2)만 있고 sidecar 문맥 줄이 있는 대조 행: (2)만인 행은 결함 빌드 A21 하나이고 `tier+control` 이라 세지 않는다(t06). 문맥 줄 둘은 그 행의 것이다(`uses/uses_c1_pre_tx.tsv`; 다른 대조 행의 문맥은 '-').
- (다) 경로 A 의 찾은 사용에 `namespace-entry` 줄이 없음: 두 빌드의 계획된 경로 A 행 다섯 가운데 그 줄이 없는 것은 A63(`b881269bea290f8b`) 하나다(`rv/r5_plan_paths.tsv`). A63 은 두 빌드 다섯 부팅 모두 닿지 않았다(`out/t02_uses.tsv`). 이 꼴은 A63 이 닿지 않아서만 피했다. 26.9 의 까닭('경로 A 행 넷은 … `namespace-entry` 줄이 있다')은 다섯 가운데 넷만 다뤘다.
- (라) 찾은 사용에 손실 표지와 묶이지 않은 수명 끝 site 가 함께 있음: 열 부팅 모두 손실이 없었다(`out/t01_boots.tsv`).

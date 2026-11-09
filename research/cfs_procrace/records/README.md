# records/

원 기록 묶음의 보존 위치다(`../CONFIG_MANAGEMENT.md` §5, `../Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md` A2).

- 묶음마다 디렉터리 하나: `MANIFEST.sha256`, `bundle.tar.xz.NN`, `PARTS.sha256`, `INFO.txt`.
- 만들기: `../env/tools/preserve_bundle.sh <id> <dir>... [-- <note>]`
- 확인: `../env/tools/verify_bundle.sh <id> <work dir> [<check command>]`. 원격 브랜치를 새로 clone해서 확인한다.
- 묶음은 고치지 않는다. 고칠 일이 생기면 새 id로 새 묶음을 만들고 INFO의 note에 이유를 적는다.
- 2026-11-09의 로컬 묶음 다섯(`step2a-20261109-stages`, `step2a-20261109-tests`, `step2a-20261109-eb`, `ck-20261109-rv3check`, `c5-20261109-regen-inputs`)은 텍스트 기록만 여기에 있다. 조각은 저장소 밖 `/home/user/work/preserve/bundles-20261109/<id>/`에 있다. push하지 않았다(저장소가 public이고 보존 위치는 PI 답 뒤다). 그래서 CM §5의 "보존됨"이 아니다. 검증은 스트림으로 했다(`VERIFIED_LOCAL.txt`). 방법과 도구 사본 기록은 `local-20261109/README.md`.

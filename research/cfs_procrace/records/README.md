# records/

원 기록 묶음의 보존 위치다(`../CONFIG_MANAGEMENT.md` §5, `../Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md` A2).

- 묶음마다 디렉터리 하나: `MANIFEST.sha256`, `bundle.tar.xz.NN`, `PARTS.sha256`, `INFO.txt`.
- 만들기: `../env/tools/preserve_bundle.sh <id> <dir>... [-- <note>]`
- 확인: `../env/tools/verify_bundle.sh <id> <work dir> [<check command>]`. 원격 브랜치를 새로 clone해서 확인한다.
- 묶음은 고치지 않는다. 고칠 일이 생기면 새 id로 새 묶음을 만들고 INFO의 note에 이유를 적는다.

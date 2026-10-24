# Φ(Phi) 프로필 언어 참조 (언어 버전 1)

Kairos 의 엔진(libkz-core)은 프레임워크 지식을 모른다. 프레임워크에 대한 모든 지식은 **프로필**(`.kzp` 텍스트 파일)에 데이터로 들어 있고, 엔진은 프로필을 읽어 그대로 해석한다. 이 문서는 그 프로필 언어의 어휘·문법·의미, 평가기, 재생 엔진, 검사기 규칙, 정규형과 해시, 출처 규칙, 그리고 SPEC·자료와 맞지 않았던 곳을 적는다.

표기: **[M]** 이번 구현에서 실행해 확인, **[D]** 설계 결정(SPEC·PI 결정), **[I]** 계획·미확인. 코드 주석은 영어이고 이 문서는 한국어이며 식별자는 영어 그대로 쓴다. 용어(T1): 판정 대상은 *readiness specification*(`rspec`), 그 Req 는 *readiness precondition*, 위반은 *readiness violation* 이다. 출처 계층의 이름 `contract-backed` 는 PI 가 정한 값이라 프로필의 값으로만 나온다.

이 문서의 예와 시험 값은 단위 시험 값이고, 연구 조건이나 실험 결과가 아니다(`../../CONDITIONS_POLICY.md`).

## 1. 개요와 위치

```
text (.kzp) ──parse──▶ Profile ──normalize──▶ ──check──▶ 해소된 Profile ──▶ canonical_text ──▶ profile_hash
                                                              │
                                      평가기 Evaluator<D>  ◀──┤  (D = ConcreteDomain | AbstractDomain)
                                                              │
                                      재생 replay(trace)   ◀──┘  (효과, happens-before, readiness 상태)
```

| 구성 | 파일 | 일 |
|---|---|---|
| 어휘 | `lib/core/phi_lexer.{hpp,cpp}` | 글자 → 토큰, 줄·열 |
| 구문 | `lib/core/phi_parser.cpp` | 손으로 쓴 재귀 하강 파서. 첫 구문 오류에서 멈춘다 |
| 구문 트리 | `include/kz/core/phi_ast.hpp` | 선언, 식, 패턴 |
| 정규화·해시 | `lib/core/phi_profile.cpp`, `phi_print.cpp` | 이름순 정렬, 중복 검출, 정규 text, `profile_hash` |
| 검사 | `lib/core/phi_check.cpp` | 이름·sort·어휘·출처. 오류를 **모두** 모은다 |
| 출처 | `lib/core/phi_provenance.cpp` | 인용 본문 해시, 번들 대조, tier–kind 짝 |
| Allowed | `lib/core/phi_allowed.cpp` | 호출자 수준 Allowed 의 엄격 해석 |
| 평가기 | `include/kz/core/eval*.hpp`, `lib/core/eval_{concrete,abstract}.cpp` | 하나의 평가기와 두 domain |
| 재생 | `include/kz/core/eval_trace.hpp`, `lib/core/eval_trace.cpp` | trace 위에서 효과, HB 간선, 상태 |
| 도구 | `tools/kz-phi/` | `check`, `dump`, `hash`, `stats`, `verify-prov --root <번들>`(인용 해시, 식별자, source 의 개정 고정), `replay --trace <파일> [--predicate 규칙=식] [--version 성분=버전]`. 디렉터리 인자는 그 안의 `*.kzp` 를 이름순으로 읽는다. 종료 코드: 0 정상, 1 프로필·인용이 틀림, 2 사용법·입출력 오류 |
| 프로필 | `profiles/cfs-v7.0.1/*.kzp` | V1 부분집합 (SPEC §9.3 S0-R2) |

엔진 코드(`include/`, `lib/`)에는 사례 식별자가 없다(`lint_case-ids`) [M]. cFS 의 이름은 프로필과 시험에만 있다.

**프로필이 하는 말과 하지 않는 말.** 프로필은 *선언*이다: 종류(kind)와 그 생명주기, 연산과 그 효과·준비 조건·결과 코드, happens-before 규칙, readiness specification. 이것이 프레임워크의 실제 동작과 같다는 것은 **인용한 본문**(파일, 줄, 본문 해시)과 사람이 적은 항목(tier `researcher`)이 뒷받침하고, 본문 해시는 `kz-phi verify-prov` 가 고정된 번들에서 다시 읽어 맞춘다 [M]. 프로필이 맞다는 증명은 없다.

## 2. 어휘 규칙

(`lib/core/phi_lexer.hpp` 와 같다.)

- 공백(스페이스, 탭, CR, LF)은 토큰을 가를 뿐이다. 줄 번호는 LF 마다 1 늘고 열은 **바이트**로 센다(1부터). CRLF 와 LF 는 같은 줄 번호를 준다.
- `#` 부터 줄 끝까지는 주석이다(문자열 밖에서). 주석과 문자열 안에는 어떤 바이트든 올 수 있고, 그 밖에서는 ASCII 만 받는다(아니면 `non-ASCII byte 0x.. outside a string or comment`).
- 식별자는 글자나 `_` 로 시작하고 글자, 숫자, `_`, `.`, `-` 로 이어진다. 끝의 `.` 와 `-` 는 식별자에 속하지 않는다(`A->B` 는 `A`, `->`, `B`; `x.y.` 는 `x.y`, `.`). 그래서 `es.app`, `K-ID.send-with-appid`, `arg.AppID` 가 이름 하나다. 예약어는 없다: `state`, `kind` 등은 자리에 따라 키워드로 읽힌다.
- 정수는 10진 또는 `0x` 16진이고 앞에 `-` 가 올 수 있으며 64비트 부호 있는 정수에 들어가야 한다(`integer literal does not fit in 64 bits`). 숫자 뒤에 글자가 바로 오면 오류다(`12ab`). 정규 text 는 정수를 10진으로 쓴다.
- 문자열은 한 줄 안의 `"..."` 이고 이스케이프는 `\\`, `\"`, `\n`, `\t` 뿐이다. 줄 끝을 만나면 `unterminated string`, 모르는 이스케이프와 제어 문자(탭 제외)는 오류다.
- 구두점: `{ } ( ) [ ] , ; : . *` 와 `= == != < <= > >= && || ! => -> :=`. `=` 와 `==` 는 같은 비교다.

## 3. 문법

`{ x }` 는 0번 이상, `[ x ]` 는 0 또는 1번, `|` 는 선택이다. `NAME` 은 식별자, `STR` 은 문자열, `INT` 는 정수다.

```
file     = "phi" INT ";" { decl } ;                 (* INT 는 1 이어야 한다 *)
decl     = profile | source | const | context | scope | kind | loc | bind | site | role
         | op | init | hb | rspec | trusted | prov ;
common   = "prov" NAME { "," NAME } | "guard" NAME cmp STR | "note" STR ;

profile  = "profile" STR [ "revision" STR ] ";" ;
source   = "source" NAME "root" STR "rev" STR { common } ";" ;
const    = "const" NAME "=" ( INT | STR | "true" | "false"
                            | "ref" "eval" STR "in" STR ) { common } ";" ;

context  = "context" "{" { ( "owner_kind" NAME | "prov" names ) ";" } "}" ;
scope    = "scope"   "{" { ( "kind" NAME | "request" NAME | "teardown" NAME | "prov" names ) ";" } "}" ;

kind     = "kind" NAME "(" [ NAME ":" sort { "," NAME ":" sort } ] ")" "{" { kinditem } "}" ;
sort     = "Bool" | "Int" | "Gen" | "Str" | "Actor" | "Loc" | "Id" "<" NAME ">" | "Res" "<" NAME ">" ;
kinditem = ( "states" NAME { "<" NAME } | "initial" NAME | "scope" NAME | "table" NAME
           | "tag" NAME "{" names "}" | "transition" NAME "->" NAME "on" NAME
           | "derived" NAME "when" expr | common ) ";"
         | "ids" "{" { ( "undefined" | "reserved" | "base" | "serial_max" ) NAME ";" } "}" ;

loc      = "loc" NAME "{" { ( "struct" STR | "field" STR | "holds" sort | common ) ";" } "}" ;
bind     = "bind" "field" STR STR "as" expr { common } ";" ;
site     = "site" NAME "=" pattern { common } ";" ;
role     = "role" NAME "{" { ( "op" NAME | "site" NAME | "purpose" NAME | common ) ";" } "}" ;
pattern  = "fn" "(" STR ")" "." ( "call" "(" STR { "," argpred } ")" | "store" "(" NAME ")" | "load" "(" NAME ")"
                              | "memset" "(" NAME ")" | "loop" "(" "poll" ")" | "return" )
           [ "nth" INT ] [ "macro" ( "yes" | "no" | "any" ) ] ;
argpred  = "arg" "[" INT "]" ( ( "==" | "!=" ) ( INT | NAME ) | "is" "addr" "(" NAME ")" | "any" ) ;

op       = "op" NAME "(" [ oparam { "," oparam } ] ")" "{" { opitem } "}" ;
oparam   = [ role ] NAME [ ":" sort ] ;              (* role = out | stamp | key | id | buffer | msg, only before a name *)
opitem   = ( "bind" "symbol" STR | conjunct | effect | outcome | terminal | common ) ";" ;
conjunct = "conjunct" NAME ":" expr "lp" lp ;
lp       = ( "capture" | "at" pattern | "site" NAME ) [ "values" NAME ":" sort { "," NAME ":" sort } ] ;
effect   = "effect" NAME ":" eff [ ( "when" | "filter" ) expr ] "lp" lp
           { "status" NAME | "refutable_by" names | "prov" names | "guard" NAME cmp STR } ;
eff      = "lifecycle" ( "all" NAME | expr ) ( "->" NAME [ "id" expr ] | "raise" expr )
         | "use" expr | "store" [ "*" ] NAME ":=" expr | "settag" NAME "of" expr ":=" NAME
         | "mark" NAME | "sync" NAME STR | "env" STR ;
outcome  = "outcome" NAME ":" ( "ready" | "error" | "not_ready" "(" NAME ")" ) [ "documented" STR [ "prov" names ] ] ;
terminal = "terminal" [ "process_terminal" ] [ "after" pattern ] ;

init     = "init" NAME ":" ( "loc" NAME "=" expr | "state" term "=" NAME ) { common } ";" ;

hb       = "hb" NAME "{" { ( "title" STR | "stratum" ( S | O | C ) | "robust" NAME | common ) ";" | template } "}" ;
template = "template" ( "program_order" "{" [ "looper" ( "yes" | "no" ) ";" ] "}"
                      | "spawn"  "{" "create" NAME "out" NAME ";" "entry" NAME ";" "}"
                      | "phase"  "{" "from" NAME "phase" NAME ";" "}"
                      | "observe" "{" "poll" STR "called_at" pattern ";" "exit_outcome" NAME ";"
                                      "predicate" ( "from_code" | expr ) ";" [ "else_terminal" STR ";" ]
                                      "watch" names ";" "}" ) ;

rspec    = "rspec" NAME "{" { ( "use" "op" NAME | "key" expr | "requires" expr
                              | "oracle" "fns" STR { "," STR } | "appreq" ( "unspecified_by_reference" | NAME )
                              | "allowed" ( "none" | "unspecified_by_reference" ) | "harm" NAME [ "(" NAME ")" ]
                              | "authored" STR | common ) ";"
                            | "allowed" "{" { allowedentry ";" } "}" } "}" ;
allowedentry = ( "documented_drop" STR | "returned_and_tested" "codes" "{" names "}" [ "continuation" STR ]
               | "retry_until_ready" "codes" "{" names "}" "bound" NAME | "fallback" NAME
               | "defer_by_design" STR | "exit" NAME STR ) [ "conflicts_appreq" ] [ "conflicts_caller" ] [ "prov" names ] ;

trusted  = "trusted" NAME "{" { ( "class" NAME | "statement" STR | "refutable_by" names | common ) ";" } "}" ;
prov     = "prov" NAME "{" { ( "tier" NAME | "kind" NAME | "side" NAME | "file" STR | "lines" INT [ INT ]
                             | "sha256" STR | "cite" STR | "authored" STR | "note" STR ) ";" } "}" ;

expr     = imp ;
imp      = or [ "=>" imp ] ;                            (* 오른쪽 결합 *)
or       = and { "||" and } ;   and = not { "&&" not } ;   not = "!" not | cmp ;
cmp      = term [ ( "=" | "==" | "!=" | "<" | "<=" | ">" | ">=" ) term | "in" "{" term { "," term } "}" ] ;
term     = INT | STR | "true" | "false" | NAME | NAME "(" [ expr { "," expr } ] ")" | "(" expr ")"
         | ( "forall" | "exists" ) NAME ":" NAME [ "where" expr ] "{" expr "}" ;
```

문법의 세부 (시험 `test_core_phi_parser` 가 줄·열과 함께 확인한다 [M]):

- `allowed { ... }`, `ids { ... }`, `template ... { ... }` 블록은 `}` 로 끝나고 뒤에 `;` 가 없다. 나머지 항목은 `;` 로 끝난다.
- `And`/`Or` 는 중첩해도 하나의 목록으로 편다: `a || (b || c) || d` 는 피연산자 4개의 `Or` 다. `=>` 는 오른쪽 결합이다.
- 비교는 사슬이 되지 않는다(`a < b < c` 는 `<` 에서 `expected the end of the expression`).
- `init ... state RES = STATE` 의 `RES` 는 *term* 이다(식이 아니다). 그렇지 않으면 `=` 가 비교로 읽힌다 [M: 시험이 이 오류를 찾았다].
- 연산 매개변수의 role(`out`, `stamp`, `key`, `id`, `buffer`, `msg`)은 뒤에 이름이 올 때만 role 로 읽힌다.
- 첫 구문 오류에서 멈추고 `파일:줄:열: error: 메시지` 를 낸다. 의미 오류(§8)는 모두 모은다.

## 4. sort 와 내장 함수

| sort | 뜻 |
|---|---|
| `Bool`, `Int`(=`Gen`), `Str` | 논리값, 64비트 정수, 문자열(상수끼리 비교만) |
| `Actor` | 실행 주체. `exec` 하나만 만들 수 있다 |
| `Loc` | 선언한 위치(`loc`)의 참조 |
| `Id<K>` | 종류 K 의 자원이 가지는 32비트 식별자. K 에 `ids` 절이 있어야 한다 |
| `State<K>`, `Tag<K,T>` | 종류 K 의 상태, 태그 T 의 값(상수는 비교 상대의 sort 로 정해진다) |
| `Res<K>` | 자원 하나: 종류 K 와 키 |

비교 규칙: `Int` 와 `Id` 는 서로 비교된다. 이때 `Id` 쪽만 2³² 로 나눈 나머지(0..0xFFFFFFFF)이고 `Int` 쪽은 줄이지 않은 64비트 값 그대로 비교한다. 그래서 `x = -1`, `x = 0x1FFFFFFFF` 는 어떤 Id `x` 에도 거짓이고 `x < 0x100000000` 은 언제나 참이다. 결과가 sort 만으로 정해지는 이런 비교는 거의 틀림없이 작성 실수이므로, 검사기는 `Id` 와 비교되는 정수 리터럴·정수 상수가 0..0xFFFFFFFF 밖이면 **경고**한다(`=`, `!=`, 순서 비교, `in` 집합) [M: `test_core_phi_check`]. `Id<K>` 끼리는 같은 K 여야 한다(`Id<*>` 는 아무 Id 와 맞는다). `State`, `Res` 는 같은 종류끼리, `Tag` 는 같은 종류·같은 태그끼리. 순서 비교(`< <= > >=`)는 `Int`, `Id`, `State` 에만 있다. 상태 상수·태그 상수의 이름은 비교 상대의 sort 를 보고 풀린다(`state(x) = REG`, `REG = state(x)` 모두 된다).

| 형태 | sort | 뜻 |
|---|---|---|
| `K(k1, ...)` | `Res<K>` | 자원 구성. 인자 수와 sort 가 종류의 매개변수와 같아야 한다. `Res<L>` 매개변수에는 L 의 자원이 오고, 키는 이어 붙는다 |
| `state(R)` | `State<K>` | 자원의 상태 (파생 종류는 §5.3) |
| `key(R)` | 첫 매개변수의 sort | 키의 첫 원소 |
| `gen(R)` | `Int` | 세대. **이 부분집합에서는 항상 0**: 세대(재시작)는 모형에 없다 |
| `id(R)` | `Id<K>` | 자원이 가진 식별자. 정해진 적이 없으면 `ids.undefined` |
| `valid(I)` | `Bool` | `(I − base) mod 2³² ≤ serial_max` |
| `value(L)` | `L` 이 담는 sort | 위치의 값. 정해진 적이 없으면 0 (INIT 사실이 있으면 그 값) |
| `tag(R, T)` | `Tag<K,T>` | 태그 값. 정해진 적이 없으면 첫 값 |
| `owner(A)` | `Res<owner_kind>` | 실행 주체 A 가 속한 자원 (`context { owner_kind K; }` 필요) |
| `forall/exists v: K [where g] { b }` | `Bool` | K 의 모든/어떤 인스턴스 v 에 대해 (`table` 절이 있는 종류만) |

이름: 연산의 비-`out` 매개변수 `arg.NAME`, 점이 정해 둔 값 `lp.NAME`(§5.6), `exec`(실행 주체), `result`(호출 결과; **효과에서만**, 준비 조건에서는 쓸 수 없다), 상수, 위치, 양화 변수. 규칙의 명시적 predicate 는 이름·상수·양화만 쓴다(매개변수, `exec`, `result` 없음).

## 5. 선언의 의미

### 5.1 profile, source, prov

`profile` 은 이름과 개정 표시(자유 텍스트)다. `source` 는 고정된 소스 트리 하나이고(`root` 는 번들 루트 기준 상대 경로, `""` 가 루트 자체, `rev` 는 소문자 16진 7~40자), 출처 기록의 파일은 가장 긴 `root` 접두를 가진 source 에 속한다. `guard C >= "v"` 는 source C 의 버전에 대한 조건이다(`6.5.0a` 처럼 끝에 글자 하나 허용).

### 5.2 const

정수, 문자열, 불, 또는 `ref eval "SYMBOL" in "file"`: 번들의 그 번역 단위에서 추출기가 상수 평가로 푸는 값이다(플랫폼 설정에 의존하므로 프로필은 값을 고정하지 않는다). 결과 코드는 호출이 돌려준 값과 **2³² 로 나눈 나머지**로 비교한다(`0xc2000003` 와 부호 있는 `-1038090237` 이 같은 코드).

### 5.3 kind: typestate

```
kind K(p1: sort, ...) {
  states A < B < C;            # 선언 순서가 종류의 순서다 (순서 비교의 의미)
  initial A;   scope process;  # scope: process | persistent (검사만 한다)
  table N;                     # 인스턴스 수를 주는 정수 상수; 양화와 `lifecycle all` 이 필요로 한다
  ids { undefined U; reserved R; base B; serial_max M; }   # 정수 상수의 이름 (리터럴 값)
  tag T { V0, V1, ... };       # 속성; 첫 값이 기본값
  transition A -> B on action; # 생명주기 간선 (선언과 이름 검사만; §12)
  derived B when expr;         # 파생 상태
}
```

**규약 (평가기가 고정한다, `eval.hpp`)** [M]:

1. 상태가 정해진 적이 없는 자원은 종류의 `initial` 상태다. 그래서 `state(K(key))` 는 전함수다.
2. 태그가 정해진 적이 없으면 첫 값이다. 식별자가 정해진 적이 없으면 `ids.undefined` 다.
3. `Id` 는 32비트 값이다. `valid(x)` 는 `(x − base) mod 2³² ≤ serial_max` 일 때 참이므로 `undefined` 와 `reserved` 표지는 (그 값이 구간 밖이면) 유효하지 않다. 검사기는 `base` 와 `base + serial_max` 가 `0..0xFFFFFFFF` 에 들어갈 것을 요구한다.
4. `owner(a)` 는 실행 주체 `a` 가 속한 자원이다. 소속이 없거나, 소속 자원이 `context` 의 `owner_kind` 의 레코드가 아니면(다른 종류이거나 키 폭이 다르면; 그런 상태는 잘못 만들어진 것이다) *정의되지 않은 자원*이고(두 domain 이 같게 읽는다; 재생은 `owner` 머리말의 키 폭이 틀리면 발견 사항을 남기고 소속을 없앤다), 정의되지 않은 자원의 상태는 종류의 초기 상태, id 는 `undefined`, 태그는 첫 값이다.
5. 파생 종류의 상태는 파생 규칙을 **높은 상태부터** 시험해 처음 참인 규칙의 상태이고, 없으면 초기 상태다.
6. 한 종류의 상태와 태그 값은 각각 최대 64개(추상 domain 이 상태 집합을 한 워드에 둔다). `Res<>` 매개변수가 순환하면 안 된다.

### 5.4 loc, bind, site, init

`loc L { struct "S"; field "f"; holds sort; }` 는 공유 위치다. `bind field "S" "f" as 대상` 은 레코드 필드를 자원의 구성요소(`state(K(v))`, `tag(K(v), T)`, `id(K(v))`, `value(L)`)에 묶는다: 추출기·관측기가 읽는 *데이터*이고, 재생 엔진은 쓰지 않는다. `site NAME = pattern` 은 이름 붙은 관측 지점이다. `init NAME: loc L = expr` 는 프로그램 시작 시 위치의 값(INIT 사실)이고, `init NAME: state K(..) = STATE` 는 시작 시 자원의 상태다. 재생은 시작 전에 이 사실들을 상태에 넣는다 [M].

**패턴 언어.** `fn("F").call("C", arg[i] ...)`, `store(T.f)`, `load(T.f)`, `memset(T)`, `loop(poll)`, `return`; `nth N`(N번째 일치; 없으면 정확히 한 번 일치해야 함), `macro yes|no|any`. 이 단계에서 패턴은 **파싱·정규화·검사만** 되고(상수 이름이 선언되었는지 등) 소스에 대해 평가되지 않는다: 패턴이 소스의 어디에 맞는지 찾는 일은 정적 추출기(K4a)의 몫이다 [I]. 재생 엔진에서 패턴은 지점의 *이름*을 만드는 데 쓰이지 않는다.

### 5.5 op

```
op NAME(params) {
  bind symbol "C 함수 이름";
  conjunct LABEL: expr lp (capture | at pattern | site NAME) [values ...];
  effect LABEL: <효과> [when expr | filter expr] lp (...) [status declared_trusted refutable_by O3, O4] prov ...;
  outcome CODE: (ready | error | not_ready(LABEL)) [documented "..." prov ...];
  terminal [process_terminal] [after pattern];
}
```

- **준비 조건(conjunct)** 은 readiness precondition 이다. `lp capture` 는 호출자가 인자로 넘기는 값의 포획 지점(T-DERIVE)이고, 그 식은 role `stamp`/`key`/`id` 인 매개변수와 상수·리터럴, 함수 `valid(...)` 만 읽을 수 있다. `lp at pattern` 은 함수 안의 지점, `lp site NAME` 은 이름 붙은 지점을 효과들이 공유한다.
- **효과** 종류: `lifecycle R -> S [id e]`(자원을 상태 S 로; id 를 함께 정함), `lifecycle all K -> S`(K 의 모든 인스턴스; memset), `lifecycle R raise e`(단조 상승: 현재보다 높은 수준으로만), `settag T of R := V`, `store *P := e`(out 매개변수가 가리키는 위치에 쓴다), `store L := e`(위치에 쓴다), `use R`(상태를 바꾸지 않음), `mark handler_begin|handler_end`, `sync role "object"`, `env "input"`. `status declared_trusted` 는 사람이 선언한 효과다(반드시 `refutable_by` 로 반박할 의무를 적는다).
- **보호 조건의 두 형태** [D: K1 round 4]: `when expr` 는 *단언*이다. 코드는 보호 조건이 참일 때만 그 선형화 지점에 이르므로, trace 가 그 지점을 보였는데 재생 상태에서 조건이 거짓이면 그것은 발견 사항이다: `declared_trusted` 효과면 그 신뢰한 조건이 이 관측으로 **반박됨**(`refutable_by` 의 의무를 적는다), 아니면 **trace 와 프로필의 불일치**다. 어느 쪽이든 효과는 적용하지 않는다(§7.3). `filter expr` 는 *가지 선택*이다. 여러 가지가 함께 지나는 지점(예: `CFE_ES_GetAppID` 의 실패 가지 저장을 뒤따르는 `UnlockSharedData` 에 묶은 경우, 성공 가지도 이 지점을 지난다)에서 조건이 참인 가지에만 효과를 적용하고, 거짓이면 아무것도 보고하지 않는다. `declared_trusted` 효과는 `filter` 를 쓸 수 없다(반박하는 관측을 숨기게 되므로; 검사기 오류).
- **`values`**: 그 지점의 사건이 매개변수 외에 싣는 값(`lp.NAME`). 기록기와 프로필이 사건의 내용을 맞추는 약속이다.
- **outcome**: 결과 코드(상수)를 `ready`(준비되어 성공), `not_ready(LABEL)`(그 준비 조건이 거짓일 때 이 코드), `error`(준비 여부와 무관한 오류)로 분류한다. `documented` 는 *공급자 쪽* 기록(side supplier)이 뒷받침하는 문서화된 결과다.
- **terminal**: 이 연산의 호출(또는 `after` 패턴의 지점)이 호출자의 후속을 없앤다. `process_terminal` 이면 프로세스 전체가 끝난다(T-TERM).

### 5.6 hb 규칙

규칙은 층(stratum) `S`(구조), `O`(관측된 동기화), `C`(스케줄 제약, HB 간선이 아님)와 강건성(`unconditional`, `obs_fatal`, `obs_soft`, `schedule_chosen`, `assumed`)을 갖고 템플릿 하나로 trace 와 맞춰진다. 짝: S 는 `unconditional`/`obs_fatal`, O 는 `obs_soft`/`schedule_chosen`/`assumed`, C 는 `schedule_chosen`. 템플릿의 *재생 의미*는 §7.4.

### 5.7 rspec

```
rspec NAME {
  use op OP;                     # 사용 연산
  key  R-식;                     # 자원 선택자 (sel); 이 키가 ordinal 을 센다
  requires P1 && P2;             # readiness preconditions: 연산의 conjunct 와 식 text 가 같아야 한다
  oracle fns "F", "G";           # 프레임워크 validity predicate (이름만; 실행하지 않는다)
  appreq PROV | unspecified_by_reference;   # 호출자 쪽(side caller) 기록이어야 한다
  allowed none | unspecified_by_reference | { 항목; ... }
  harm H;  authored "...";
}
```

**호출자 수준 Allowed 의 엄격 해석** (QT-4, Q15; `derive_caller_allowed`) [M: `test_core_phi_allowed`]:

| 입력 | 결과 |
|---|---|
| 항목 없음, `none`, `appreq` 지정 | `none` |
| 항목 없음, `none`, `appreq` 미지정 | `unspecified_by_reference` ("`none` 은 지정된 appreq 를 요구") |
| 항목 없음, `unspecified_by_reference` | `unspecified_by_reference` |
| 항목 중 `caller` 기록이 하나라도 있는 것 | 그 항목이 **판정한다**(`Entries`): horizon monitor 로 읽힌다 |
| 항목이 `rule` 기록만(또는 supplier 와 rule 만) | 그 항목은 받아들이지 않는다("규칙 하나는 결코 수용하지 않는다"). 호출자 수준에 들지 않으므로 결과는 항목이 없을 때와 같다: `appreq` 지정이면 `none`(그래서 VIOLATING), 미지정이면 `unspecified_by_reference` (SPEC App. A.8 litmus LP-ALLOW (b1r): "NOT_READY_UNJUDGED …; VIOLATING with an `appreq` that requires `R`'s state") |
| 항목이 `supplier` 기록만 | 프레임워크 수준 정보일 뿐 호출자 수준에서는 아무것도 판정하지 않는다. `appreq` 지정이면 `none`, 아니면 `unspecified_by_reference` |
| `caller` 항목에 `conflicts_appreq` | 판정을 보류: `unspecified_by_reference`, 충돌 표시 (TOLERATED 가 되지 않는다) |

**caller 쪽 기록이 될 수 있는 tier** (QT-4, QT-9) [M: `test_core_phi_check`, `test_core_phi_allowed`]: `side caller` 는 호출자에게 하는 말을 *인용한 본문*에만 붙는다: `contract-backed`(API 문서), `requirement-derived`(응용·임무 요구; 이 빌드의 어휘에는 아직 없음, §10), `admitted`(사용 코드의 주석이 처리를 인정함, SPEC §3.9 "side caller, code_admission"), `issue-derived`(유지보수자가 이슈·커밋에서 한 말). `code-derived`(코드 패턴은 도구의 고정 규칙, side `rule`), `researcher`, `unreferenced` 기록에 `side caller` 를 붙이면 검사기 오류다. 그래서 손으로 넣은 기록은 `appreq` 가 될 수 없고(QT-9: 연구자가 쓴 appreq 로 발생을 readiness violation 으로 만들지 않는다), 판정하는 Allowed 항목도 만들 수 없다(QT-4: 연구자 규칙 하나는 수용을 확정하지 않는다). `derive_caller_allowed` 도 검사되지 않은 프로필에서 그런 기록을 만나면 `rule` 로 읽고 appreq 를 미지정으로 본다.

**caller 항목끼리의 충돌** (2026-10-24, C1 P3 대안): `conflicts_caller` 는 저자가 기록한 표시이다. 같은 not-ready 결과에 양립하지 않는 계속을 받아들이는 caller 항목이 둘이면(SPEC 1.6 step 3, QT-5) 그 항목들에 붙인다. `derive_caller_allowed` 는 `conflicts_appreq` 와 같이 `unspecified_by_reference` 와 충돌 표시를 낸다. 양립 여부를 계산하지는 않는다(프로필에 그 개념이 없다).

### 5.7a role: 호출 문맥의 역할 (SPEC rev 1.3 보충 2026-10-24, §1.6 "Call contexts")

```
site cf_lookup_call = fn("CF_CFDP_InitEngine").call("OS_CountSemGetIdByName") prov ...;
role cf-dependent-lookup { op OS_CountSemGetIdByName; site cf_lookup_call; purpose dependent_use; prov ...; }
```

역할은 한 연산의 호출 문맥이 무엇을 위한 호출인지를 출처와 함께 선언한다. `site` 는 선언된 호출 패턴(소스 결합)이고, 바이너리 결합은 관측 계획이 한다. `purpose` 어휘는 `dependent_use`(준비 요구가 붙는 의존적 사용)와 `existence_probe`(not-found 결과가 호출자 자신의 생성 조건인 존재 확인)이다. 검사기 [M: `test_core_phi_check` 10건]: 연산·사이트가 있음, 사이트가 그 연산 심볼의 호출 패턴, 사이트당 역할 하나, 그리고 호출을 사양에서 빼는 역할(`dependent_use` 가 아닌 것)은 tier 가 researcher·unreferenced 가 아닌 기록을 하나 이상 인용해야 한다(손으로 넣은 기록만으로 문맥을 빼지 못한다). 앱 이름 어휘는 없다.

재생 의미 [M: golden `replay_c1_*`]: 역할의 사이트 사건은 그 호출이 반환된 바로 뒤, 같은 행위자에서 온다(호출자 안 after-call 지점). 그때 그 호출의 발생에 `context` 가 붙고, `existence_probe` 이면 그 호출이 시작한 발생은 사례가 아니다: 발생 목록에서 빠지고(서수도 세지 않음) `role facts` 절에 `role_fact … not an instance of <rspec>` 로 남는다. 바로 앞 사건이 그 연산의 반환이 아니면 발견 사항이고 아무것도 묶지 않는다(발생은 그대로 사례). 역할 사이트가 관측되지 않은 호출은 사례로 남는다(빼기에는 기록된 역할이 필요하다).

### 5.8 trusted, context, scope

`trusted NAME { class declared_trusted|assumption; statement "..."; refutable_by O9; }` 는 신뢰 원장 (iii)부에 올라갈 항목이다: **반박될 수 있을 뿐 증명된 것이 아니다**. `context { owner_kind K; }` 는 `owner(exec)` 의 종류다. `scope { kind K; request S; teardown T; }` 는 기본 범위(SHUTDOWN_REQ 이전)를 정의한다: 종류 K 의 상태가 `request` 이상이 되는 효과가 SHUTDOWN_REQ 지점이다.

## 6. 평가기

**하나의 평가기**(`Evaluator<D>`, `eval_evaluator.hpp`)가 해소된 식을 걸으며 domain `D` 에 원시 연산(리터럴, 연결어, 비교, 상태, 값, 태그, id, 소유자, 인스턴스 목록 …)을 묻는다. 식의 의미는 이 파일 한 곳에만 있다. 모든 피연산자를 평가하고 단락(short-circuit)하지 않는다. `a => b` 는 `¬a ∨ b`, `x in {e1..en}` 는 `x = e1 ∨ ...`, `forall v: K where g {b}` 는 모든 인스턴스에 대해 `(¬g ∨ b)` 의 합, `exists` 는 `(g ∧ b)` 의 합이다.

| domain | 값 | 용도 |
|---|---|---|
| `ConcreteDomain` | 정확한 값 (`CVal`) | trace 가 말하는 상태 위에서 (§7) |
| `AbstractDomain` | 구간, 상태 집합(비트마스크), 삼치 논리 `{참, 거짓, 모름}`, 정의 안 됨 여부를 가진 자원 상자 | 정적 분석이 아는 것 위에서 (K4a, 후속) |
| (operational) | — | 탐색기용; 같은 템플릿의 `ConcreteDomain` 인스턴스가 될 것이고 새 평가기가 필요 없다 [I] |

**추상 상태의 구체화(concretization)** (`eval.hpp` 의 `AbstractState`): 구체 상태 s 가 추상 상태 a 로 기술되는 것은 (1) a.res 에 나열된 자원의 상태·태그·id 가 나열된 집합 안에 있고, (2) 나열되지 않은 자원은 초기 상태(태그는 첫 값, id 는 undefined)이고 — 나열은 "모름"을 말하는 방법이다 —, (3) 나열된 위치의 값이 구간 안에, 나열되지 않은 위치는 0, (4) 실행 주체·결과가 구간 안에, 소유자가 나열된 소유자 중 하나이거나(`owner_none` 이면) 없음, (5) 유한 종류의 인스턴스가 나열된 키와 정확히 같음. 규칙 (2)·(3) 의 기본값은 모델 trace 와 INIT 의 규약이다: 실제 기록에서는 기록이 고정하지 않은 종류·위치·소유자를 `open_kinds`·`open_locs`·`owner_any` 로 열어 아무 값이나 허용한다(§7.6, SPEC OB-05).

**건전성(soundness)**: s 가 a 에 속하고 인자가 맞으면, 모든 식의 구체 값은 추상 값의 구체화에 속한다. 성질 시험 `tests/property/test_eval_agreement.cpp` 가 생성한 식(고정 시드)과 상태·추상화에서 확인한다 [M]:

- 정확한 추상화: 18,000 평가, 위반 0, 부정확(모름) 0;
- 넓힌 추상화(6개 시드): 72,000 평가, 위반 0, 결과의 약 2/3 가 참/거짓으로 결정됨(비공허);
- 일부러 틀린 추상화: 위반이 나옴(시험이 실패할 수 있음을 확인);
- **변이 확인(손으로, 자동화하지 않음)**: `eval_abstract.cpp` 에 건전성을 깨는 변경 10가지(`<` 의 경계, `valid` 의 상한, `state_of`·`key_of`·`id_of` 의 정의 안 됨 처리, 태그 기본값, 소유자 없음, 파생 상태 선택, `lor` 의 모름 처리, 키 상자 안의 초기 상태)를 하나씩 넣어 보니 성질 시험이 **10가지 모두** 잡았다(하나는 부정확으로 잡혔다). 단위 시험 `test_core_eval` 혼자서는 10가지 중 4가지만 잡았다. 그 뒤 원본으로 되돌렸다(`cmp` 로 확인).

추상 `valid` 는 식별자 구간이 `[0, 2³²)` 를 벗어나면 "모름"을 돌려준다(구체 값은 하위 32비트로 감기므로). 추상 평가기의 정밀도는 *건전하되 일반적으로 근사*다. 정확히 하나의 구체 상태를 나타내는 추상화에서는 위 시험이 정확함을 확인한다.

## 7. 재생 엔진 (`eval_trace.hpp`)

프로필을 trace 에 적용한다: 효과를 되풀이해 상태를 만들고, happens-before 간선을 만들고, readiness specification 의 발생(occurrence)을 분류한다. 엔진은 어떤 연산·종류·규칙도 이름으로 알지 못한다. trace 는 *합성 trace* 여도 된다(이 단계의 시험과 golden 이 그렇다): 기록기가 맞춰야 할 약속은 아래와 같다.

### 7.1 trace

사건 = (행위자, 사이트, 단계, 값 맵, 텍스트 맵). 단계는 `call`, `ret`, `pt`(지점).

- `call`/`ret` 의 사이트는 프로필 연산의 이름. 매개변수 값은 `call` 의 `arg.NAME`, out 매개변수의 값은 `ret` 의 `out.NAME`, 결과는 `ret` 의 `result`.
- `pt` 의 사이트는 선형화 지점의 이름: `OP.LABEL`(연산 안의 지점과 포획 지점), `lp site NAME` 이면 사이트의 이름, 종료 단계(`terminal ... after 패턴`)이면 `OP.terminal`. 선언된 `site` 의 이름도 알려진 지점이다(규칙이 참조할 수 있다).
- 지점은 `lp.NAME`(그 지점의 `values`)과 필요한 `arg.NAME`, 그리고 out 으로 쓰는 효과의 `out.NAME=@위치`(포인터가 가리키는 위치의 이름), 선택적으로 저장한 값 `value` 를 싣는다. 포획 지점은 호출이 받을 `arg.NAME` 을 싣고, 교차 검증용으로 `cell=@위치`, `value` 를 싣는다.
- 머리말: `const NAME 값`(`ref eval` 상수; **유한 종류의 표 크기는 여기서만 온다**), `owner 행위자 키…`(행위자가 속한 자원의 키).

텍스트 형식 (`parse_trace_text`) [M]:

```
# 주석
const CFE_PLATFORM_ES_MAX_APPLICATIONS 4
owner T_SB 2
T_SB CFE_EVS_SendEventWithAppID.id_valid pt arg.AppID=0x110002 cell=@CFE_SB_Global.AppId value=0x110002
T_SB CFE_EVS_SendEventWithAppID call arg.AppID=0x110002
T_SB CFE_EVS_SendEventWithAppID ret result=0
```

정수는 10진·16진·음수, `@NAME` 은 텍스트, `result=` 는 결과. 오류는 `trace line N: ...`. sort `Str` 매개변수는 텍스트로 준다(`arg.sem_name=@BP0_tsem`): 식은 그 문자열의 handle(`str_handle`)을 읽는다(2026-10-24).

### 7.2 알고리즘

한 번의 통과다. 사건마다: (a) 프로그램 순서 간선; (b) 호출이면 열린 호출 목록에 넣고, 그 연산의 *포획된* 준비 조건에 붙은 포획을 가져오고(없으면 호출 자체에서 평가), rspec 의 발생을 만들고; 지점이면 그 지점의 준비 조건을 **먼저**(효과 이전의 상태에서) 평가하고 그 다음 효과를 적용하고; 반환이면 열린 호출을 닫고, 결과 코드를 검사하고, 아직 결정되지 않은 발생을 NOT_EVALUATED 로 결정하고, 관측 규칙을 적용한다. 종료 단계를 만나면 이후의 사건은 재생하지 않는다(발견 사항 하나). 끝난 뒤 값으로 짝을 짓는 간선(생성 → 진입, phase)을 만든다. 같은 내용의 간선은 한 번만, (목표, 출발, 규칙) 순으로 정렬한다.

**발견 사항(finding)** 은 trace 가 프로필과 맞지 않는 곳이다: 모르는 연산/지점, 호출 밖의 지점, 반환되지 않은 호출, 필요한 값이 사건에 없음(식을 평가하지 않는다), 저장한 값이 프로필 계산과 다름, 포획한 값이 재생한 위치 값과 다름, 정의되지 않은 자원에 대한 효과, 범위 밖 상승 수준, 결과 코드가 outcome 목록에 없음, 결과 코드와 계산한 준비 상태가 어긋남(O4(a)), 규칙의 다른 가지가 종료 연산으로 이어지지 않음, 가드 때문에 적용되지 않은 규칙·효과, 단언한 보호 조건(`when`)이 거짓인 지점(§7.3; `Result::guard_failures` 에도 구조로 남는다), `owner` 머리말의 키 폭이 소속 종류와 다름. 발견 사항은 오류가 아니라 보고다: 재생은 계속된다. `Status` 가 `Invalid` 인 것은 검사되지 않은 프로필과, 표 크기를 주지 않은 trace(`const NAME N` 필요), 크기가 0..4096 밖일 때, 유한 종류의 키가 한 원소가 아닐 때뿐이다 [M].

### 7.3 효과 적용

| 효과 | 재생 |
|---|---|
| `lifecycle R -> S [id e]` | R 의 상태를 S 로(없던 항목은 만든다), id 를 정하고, 쓰기를 기록한다 |
| `lifecycle all K -> S` | K 의 모든 인스턴스(표 크기만큼) |
| `lifecycle R raise e` | 현재보다 높을 때만 올린다. 수준은 종류의 상태 번호여야 한다(아니면 발견 사항) |
| `settag T of R := V` | 태그 값 |
| `store *P := e` | `out.P` 가 이름한 위치에 쓴다(없으면 추적하지 않는 위치) |
| `store L := e` | 위치에 쓴다 |
| `mark handler_begin` | 이 사건이 새 핸들러를 시작한다 (프로그램 순서의 looper 규칙) |
| `use`, `sync`, `env`, `mark handler_end` | 상태 변화 없음 |

보호 조건을 평가할 수 없으면(필요한 값이 사건에 없음) 적용하지 않는다. `filter` 조건이 거짓이면 조용히 적용하지 않는다(가지 선택). `when` 조건이 거짓이면 적용하지 않고 **보고한다**: 발견 사항 하나와 `Result::guard_failures` 의 항목 하나(사건, 연산, 효과, `declared_trusted` 여부, 그 `refutable_by` 의무). 결과 text 에는 `guard failures N` 절과 `guard_failure #E effect OP.LABEL refuted refutable_by O3 O4` (신뢰한 조건의 반박) 또는 `... disagreement` (trace 와 프로필의 불일치) 줄로 나온다 [M: `test_core_replay`, golden `replay_effects_external`]. 효과의 `guard` 가 만족되지 않거나 평가할 수 없으면 적용하지 않고 발견 사항을 남긴다. 파생 종류(`derived`)는 저장하지 않고 평가 때 계산한다. 쓴 위치(상태 변경)는 *마지막 쓴 사건*으로 기록해 관측 규칙이 쓴다.

### 7.4 happens-before 템플릿의 재생 의미

| 템플릿 | 간선 |
|---|---|
| `program_order` | 한 행위자의 연속한 사건 쌍. `looper yes` 이면 새 핸들러를 시작하는 사건(`mark handler_begin`)으로 들어가는 간선은 없다 |
| `spawn { create OP out P; entry OP2; }` | 생성 호출 `OP` 의 **호출 사건** → 자식의 `OP2` 호출 사건. 짝은 값으로 맞춘다: `ret` 의 `out.P` 와 진입 호출의 `arg.P`. 호출 시각이 진입 이전이면 되고(자식이 `OP` 가 반환하기 전에 돌 수 있다), 같은 값의 id 가 다시 쓰이면 호출 순서대로 다음 진입과 짝이 된다. 진입이 자기 행위자의 첫 사건이 아니면 발견 사항 |
| `phase { from X phase F; }` | 행위자 A 의 `X`(단계 F: `call`/`return`/`point`) 사건 e 마다, A 가 e **이후에** 만든 자식의 *첫 사건* 으로 가는 간선 |
| `observe { poll W ... exit_outcome C; predicate ...; else_terminal T; watch K1, K2; }` | `W` 의 `ret` 이 결과 `C` 이고 predicate 가 그 시점의 재생 상태에서 참이면, `K1`, `K2` 종류의 자원에 대한 **마지막 쓰기**(상태 변경 효과; 다른 행위자의 것만)에서 이 `ret` 로 간선. 결과가 `C` 가 아니면 같은 행위자의 *다음 호출*이 `T` 여야 한다(아니면 발견 사항). predicate 가 거짓이면 발견 사항을 내고 간선은 없다. predicate 는 `predicate from_code` 이면 호출자가 `Options::predicates` 로 준다(추출기의 몫); 없으면 간선은 없다 |

규칙의 `guard` 는 `Options::versions` 로 평가한다. 버전이 없거나 만족되지 않으면 **규칙을 적용하지 않고** 발견 사항을 남긴다(간선을 빼는 쪽이 보수적이다). 버전 비교는 점으로 나뉜 숫자 비교다(`6.4.10 > 6.4.9`, 모자란 성분은 0).

**관측 규칙의 한계 [D/I]**: "마지막 쓰기" 읽기는 T-OBS 의 *최소* 해석이다. 규칙의 `called_at` 패턴, 쓰기 집합의 완전성, falsifier 는 재생에서 쓰이지 않는다(정적 분석 K4a 의 일).

### 7.5 발생의 상태

(SPEC §1.6 "Instances"; [M] golden `replay_*` 와 `test_core_replay`)

- 발생은 rspec 의 연산 호출에서 시작한다. 값 준비 조건(`lp capture`)의 포획 지점은 **호출 직전의 마지막** 포획 사건이고, 호출에 붙는다(호출 뒤에 올 호출이 없는 포획은 아무것도 시작하지 않는다). 포획 사건이 없으면 호출 자체에서 평가한다(source `call`). 호출 안의 조건은 그 지점에서 평가한다(source `point`). 같은 조건의 첫 통과가 결정한다.
- **결정**: 호출 시점에 붙은 포획 중 거짓이 있으면 호출에서; 그렇지 않으면 처음 거짓이 되는 조건의 지점에서; 모든 조건이 참이면 마지막 조건 지점(모두 호출 전에 결정되면 호출)에서; 그 전에 호출이 반환하면 반환에서.
- **상태**: 반환이 결정하고 거짓인 조건이 없으며 조건 지점을 지나지 못했다면 `NOT_EVALUATED`; 그렇지 않고 모든 조건이 참이면 `READY`; 거짓이 있으면 호출자 수준 Allowed 로: `none` → `VIOLATING`, `unspecified_by_reference` → `NOT_READY_UNJUDGED`, 판정하는 항목 → `PENDING`. 결정되지 않은 채 trace 가 끝나면 `OPEN`. 결정은 최종이다(L-STATUS): 결정 뒤에 다른 조건 지점을 지나도 상태는 바뀌지 않는다.
- **horizon monitor: `retry_until_ready` 만 구현했다** (2026-10-24, C1 판정 가능성; SPEC §1.6 "Horizon monitors") [D][M: `test_core_monitor`, golden `replay_c1_post_retry`]. 판정하는 항목(caller 기록이 있는 항목, `derive_caller_allowed`) 가운데 `retry_until_ready codes {…} bound B` 가 있으면 재생 끝에(역할로 빠진 발생을 지운 뒤) monitor 를 돌린다. 감시 행위자는 사용하는 행위자이다. **시도**는 그 rspec 의 발생이고, **반복 인스턴스**는 한 행위자·한 호출 문맥(`context`)·한 키·키 자원의 한 세대에 대한 연속된 시도이다. 인스턴스는 READY 시도, 결과 코드가 `codes` 밖인 시도, 또는 결과가 `codes` 안인 B 번째 시도(소진 경로: 반복은 최대 B 번 시도한다)에서 끝난다. ACCEPT = 같은 인스턴스의 뒤 시도가 READY(그 앞의 PENDING 시도는 모두 `TOLERATED`; `NOT_EVALUATED` 시도는 ACCEPT 가 아니다), REJECT = 소진 또는 `codes` 밖의 결과(그 인스턴스의 PENDING 시도는 `VIOLATING` — 단 retry 가 판정하는 항목의 유일한 것일 때만; 다른 항목의 monitor 는 돌지 않으므로 그 밖에는 `PENDING` 으로 남기고 REJECT 를 적는다). 기록이 ACCEPT·REJECT 전에 끝나면 `PENDING`(OPEN 은 ACCEPT 가 아니다) → `INCONCLUSIVE(horizon-open)`. **B 는 항목의 상수 값**이다: 프로필의 정수 상수(출처 포함) 또는 trace 가 준 `ref eval` 값. 값이 없으면 monitor 를 돌리지 않고 발견 사항을 남긴다. 도구 자신의 숫자는 없다. **세대**: 키 종류의 자원을 초기 상태로 되돌리는 효과가 세대 경계이다. 프로필에 그런 효과가 없으면 모든 시도는 한 세대(`gen(R)` = 0)이고, 있으면 두 시도 사이에 그 지점이 있거나(실제 기록에서) 그 지점이 관측되지 않으면 한 인스턴스로 세지 않는다. **셀 수 없는 것은 보류한다**: 시도의 키가 고정되지 않음, 보고된 손실이 시도 구간과 겹침(시도가 빠졌을 수 있음), 세대 경계 가능, 뒤 시도의 상태가 READY 를 포함해 미확정 — 그 인스턴스의 PENDING 시도는 `UNDETERMINED`(possible `PENDING`, `TOLERATED`, 유일한 항목이면 `VIOLATING`), 사유 `explanation_ambiguous`(손실이면 `loss` 도). 결과 text 의 발생에 `monitor` 줄이 붙는다(ACCEPT/REJECT 사건, 몇 번째 시도인지, `bound NAME = 값`). `TOLERATED` 의 요약은 caller 기록과 ACCEPT 가 있으므로 `NO_VIOLATION_IN_CHECKED_PREFIX`(D2). 다른 Allowed 항목(`returned_and_tested`, `exit`, `fallback`, `defer_by_design`, `documented_drop`)의 monitor 는 여전히 없다: 그 항목이 판정하는 발생은 `PENDING` 으로 남는다 [I]. 반복의 진입은 사건이 아니므로, 한 행위자가 같은 키·세대에서 두 반복을 사이에 아무것도 없이 돌면 한 인스턴스로 읽힌다 [I].
- **범위**: 결정 사건 앞에 SHUTDOWN_REQ 지점이 있으면 `scope teardown`(K-TEARDOWN 가족), 아니면 `default`.
- **서수**: 같은 행위자가 같은 키로 시작한 k번째 발생(1부터).
- 발생의 조건 목록은 정규화된 순서, 곧 *레이블 이름순* 이다(선언 순서가 아니다; 정규화가 한 연산의 conjunct 를 레이블로 정렬한다). 결정 사건은 사건 번호로 정해지므로 이 순서에 영향받지 않는다.
- **결과 코드 검사(O4(a))**: 결과를 outcome 목록에서 찾는다(2³² 로 나눈 나머지로 비교). `ready` 는 거짓이 평가된 조건이 없을 때만, `not_ready(L)` 은 조건 L 이 평가되어 거짓일 때만 일치한다. `error` 는 준비 여부를 말하지 않으므로 검사하지 않는다(`not_checked`). 목록에 없는 코드는 `unmapped`.

### 7.6 실제 기록 재생 모드 (SPEC rev 1.3 §4.8, ADR-055, 변경표 X-02, 2026-10-24) [D][M]

§7.1–§7.5 와 §6 의 구체화 규칙 (2)(나열되지 않은 자원은 초기 상태, 위치는 0 또는 INIT 값)는 **완전한 모델 trace**(합성 trace, golden, INIT)의 규약이다. 실제 실행 기록은 사건의 일부만 담으므로 이 규약을 쓰지 않는다(OB-01, OB-05). 두 모드는 **명시적으로만** 고른다: trace 의 첫머리 `record real` 줄, `kz-phi replay --real-record`, 또는 `kz replay`(실행 컨테이너 어댑터, `docs/observer.md` §3.4). 아무것도 고르지 않으면 모델 trace 이고 그 결과 text 는 바뀌지 않았다 [M: `phi_golden_*` 17개, `test_core_replay` 그대로 통과].

**평가.** 실제 기록에서는 조건(conjunct), 효과의 자원·값·보호 조건, rspec 의 키, 관측 규칙의 predicate 를 §6 의 **추상 domain** 으로 평가한다. 추상 상태는 "기록과 양립하는 모든 설명"을 나타낸다: 기록이 고정한 종류·위치만 재생 상태 그대로 정확히 나열하고, 나머지 종류는 `open_kinds`(나열되지 않은 자원은 아무 상태·태그·id), 나머지 위치는 `open_locs`(아무 값), 소유자를 기록이 말하지 않은 행위자는 `owner_any`(owner 종류의 아무 기록 또는 정의 안 됨)로 둔다(`eval.hpp` `AbstractState`). 사건에 없는 인자·결과도 아무 값이다. 결과가 참/거짓이면 모든 설명에서 그렇다는 뜻이고, "모름"이면 판정을 보류한다. 이 포함 관계는 의무 L-OBS(SPEC §6.2, `unproved`)이고, 시험은 추상 domain 의 성질 시험을 열린 추상화까지 넓힌 것이다(아래) — 증명이 아니다.

**기록된 쓰기가 상태를 고정하는 조건(ADR-055).** 종류 또는 위치마다 기록이 다음을 모두 세울 때만 기록된 효과가 그것을 고정한다: 시작 상태를 앎(`start=known`), writer 포괄(`writers=covered`), 별칭 포괄(`aliases=covered`), 수명을 앎(`lifetime=known`, 종류만), 그리고 **프로필에서 그것을 쓰는 모든 효과의 지점이 관측됨**(`observed <지점>`; 포인터를 통한 `store *P` 는 모든 위치의 writer 로 센다). 하나라도 없으면 처음부터 끝까지 열려 있다(초기 상태로 두지 않는다). 손실이 보고되면(`loss begin`) 그 위치부터 기록 끝까지 모든 종류·위치가 열린다(커널은 어느 기록이 버려졌는지 말하지 않는다). 기록이 고정한 종류에서도 효과의 대상 자원·값이 고정되지 않거나(예: 소유자 미확정인 행위자의 `owner(exec)`), `filter` 보호 조건이 "모름"이면 그 대상 종류·위치는 그때부터 열린다. 단언한 보호 조건(`when`)이 "모름"이면 그 지점이 실행되었다는 기록이 곧 보호 조건이 참이라는 뜻이므로 적용하고, 모든 설명에서 거짓이면 §7.3 의 보고에 더해 대상이 **모순**(아래)으로 표시된다.

**순서.** 수집기의 병합 순서(시각, CPU, 도착)는 서로 다른 행위자 사이의 실행 순서가 아니다(SPEC §5.8, §4.8 "order"). 그래서 기록이 고정한 종류에서도 **읽는 행위자가 아닌 행위자가 기록 안에서 쓴** 자원·위치는 그 읽기에서 아무 값이나 될 수 있다(사유 `explanation_ambiguous`). 같은 행위자의 쓰기만 있으면 프로그램 순서가 정한다. 기록이 행위자 사이의 순서가 확립되었다고 선언하면(`order established`: HB 간선이나 믿을 수 있는 관측 순서) 이 규칙을 쓰지 않는다. 관측기 어댑터는 이것을 선언하지 않는다.

**지점 통과.** 관측되는(`observed`) 지점의 조건은 그 사건에서 평가하고, 반환까지 그 사건이 없으면 통과하지 않은 것이다(모델과 같음; 단 호출 구간이 손실과 겹치면 통과 여부 `passage-unknown`, 사유 `loss`). 관측되지 않는 지점의 조건은 **호출 구간 전체**(호출에서 반환까지의 모든 상태)에서 읽은 값의 합(join)이고(`interval`), 통과 여부는 모른다(`passage-unknown`, 사유 `effect_point_ambiguous`, EV-04).

**반환 코드(O4).** 반환 코드는 관측된 값이고 프로필의 outcome 목록이 그 뜻을 준다: `not_ready(L)` 은 L 이 도달되어 거짓, `ready` 는 도달한 조건 중 거짓이 없음. 그래서 기록이 열어 둔 조건을 코드가 고정한다(`outcome`, 요약 플래그 `outcome-narrowed`). 이 고정은 outcome 목록의 출처와 A2·A6·O4 에 기댄다. 모든 설명에서 고정된 조건과 코드가 어긋나면(`Qv`, `Qn` 모두 UNSAT) **분석 입력 오류**이다: 발생의 `input_error`, 요약 `observation = none` + `analysis-input-error`, 결과 전체 `partial`. 위반으로도 위반 없음으로도 세지 않는다. 포획 값이 고정된 위치의 재생 값과 다를 때, 단언한 보호 조건이 모든 설명에서 거짓일 때도 그것을 읽는 결과는 같은 입력 오류가 된다.

**상태.** 모든 설명에서 같은 상태만 결정한다(L-STATUS 는 그대로: 고정된 거짓 조건이 나오면 그 지점에서 결정). 반환 때 설명들이 허용하는 상태가 둘 이상이면 `UNDETERMINED` 이고 그 목록(`possible`)과 사유를 남긴다. 이것은 여섯 상태에 더한 상태가 아니라 "상태 미확정"의 표시다(SPEC §1.10: 모호한 관측값은 상태를 미확정으로 둔다; `NOT_READY_UNJUDGED` 를 빈 정보의 공통값으로 쓰지 않는다). 기록 끝까지 결정되지 않으면 `OPEN` 이다.

**요약 뷰.** 모든 발생에 `kz::summarize_observation`(SPEC §1.10, D2; `docs/schema.md` §10)으로 요약을 계산한다. 모델 trace 에서도 계산하지만 결과 text 에는 실제 기록일 때만 쓴다. `READY` → `NO_VIOLATION_IN_CHECKED_PREFIX`; `TOLERATED`(retry monitor 의 ACCEPT, caller 기록) → `NO_VIOLATION_IN_CHECKED_PREFIX`; `VIOLATING` → `OBSERVED_VIOLATION`(모든 설명에서 위반일 때만 결정되므로); `NOT_READY_UNJUDGED` → `INCONCLUSIVE(caller_requirement_absent)`; `PENDING`·`OPEN` → `INCONCLUSIVE(horizon-open)`; `NOT_EVALUATED` → `INCONCLUSIVE(not_evaluated)`; `UNDETERMINED` → `INCONCLUSIVE(사유)`. 발생마다 그 조건이 읽은 것에 대해 기록이 세운 적합성(`adequacy`, KZ_ADEQ)과 정체성(실행 task, 생성 실행자 `creator`, 소유 앱과 근거 수준, 연산의 `stamp` 매개변수로 전달된 식별자)을 함께 쓴다.

**실행 관계(SPEC §1.1, §1.10 항목 10; 2026-10-24 W2 잔여)** [D][M: `test_core_realrecord` `test_execution_relation`]. 발생마다 사용의 실행 task 와 **그 발생의 키 자원을 등록·게시한 행위자** 사이의 관계를 계산한다(`Occurrence::relation`, 요약의 `relation`). 등록·게시는 기록에서 지점이 관측되어 적용되었을 수 있는 효과 중 키의 종류에 대한 자원 하나짜리 효과(`lifecycle`·`raise`·`settag`; 종류 전체를 지우는 `all` 효과는 자원을 지목하지 않으므로 제외)이다. 그 대상은 **쓴 행위자 자신의 정체성으로** 읽는다: `owner(exec)` = 그 행위자의 `app_owner` 레코드 키, `id(owner(exec))` = 기록이 준 소유 앱의 id 값(`owner_id`, 없으면 재생 상태의 id). 이 읽기는 행위자를 지목하는 데만 쓰고 재생 상태를 고정하지 않는다(관측 판정은 더 결정적이 되지 않는다). 판정 규칙:

| 경우 | 관계 |
|---|---|
| 사용 쪽 소유 앱이 `confirmed`·`inferred` 가 아님 | `ownership-undetermined` |
| 키 종류에 대해 대상이 세워지지 않은 쓰기가 있음(쓴 행위자의 소유 앱 미정, 값 미기록, `filter` 가 열린 채) | `ownership-undetermined` |
| 기록 안에 키의 등록·게시가 없음 | `ownership-undetermined`(사유를 적는다. 위반 여부와 따로 보고) |
| 상대 행위자의 소유 앱이 `confirmed`·`inferred` 가 아님 | `ownership-undetermined` |
| 같은 task, 또는 같은 소유 앱의 다른 task | `intra-application`(대조군; "same task" / "same application") |
| 한쪽이 다른 쪽의 생성 실행자 | `creation-framework-new-application` |
| 소유 앱이 다르고 한쪽이 코어 서비스(`owner_core yes`) | `core-application` |
| 소유 앱이 다르고 둘 다 코어 서비스가 아님 | `inter-application` |
| 소유 앱이 다르고 코어 여부가 세워지지 않음 | `inter-application` + 플래그 `core-status-unknown` |

상대가 여럿이면: 하나라도 미정이면 미정, 모두 같은 앱이면 `intra-application`, 아니면 `inter-application` → `core-application` → 생성 관계 순으로 처음 나타나는 것. 쓴 소유 앱 수준 중 가장 약한 것이 `relation_level` 이고, 그것이 `inferred` 이면 요약에 `relation-inferred` 를 붙인다. 관계는 행위자 사이의 것이며 사건 순서를 읽지 않는다(병합 순서는 실행 순서가 아니다, SPEC §5.8). 모델 trace 에서는 계산하지 않는다. 결과 text 에 `relation <값> owners <수준>: <사유>` 줄이 붙는다.

**포획한 위치를 쓴 행위자 (2026-10-24, R2 잔여)** [D][M: `test_core_realrecord` `test_captured_location_relation`, `test_trace_static_closure`]. 포획 사건이 `cell`(값을 읽어 온 프로필의 위치)을 주면 그 조건 결과에 위치가 남고(`ConjunctResult::cell`), 실행 관계의 상대에 **그 위치를 기록 안에서 쓴 행위자**가 더해진다: 그 위치로의 저장 효과(`store L := …`, 위치가 이름으로 기록된 `store *P := …`)와 **게시 사이트**의 사건. 게시 사이트는 호출 패턴이 `arg[i] is addr(T.F)` 로 필드 주소를 넘기는 프로필 `site` 이고, 그 호출 대상 기호를 묶은 연산의 매개변수 i 가 `out` 이며 연산이 그것을 통해 저장하고(`store *P := …`), `bind field T F as value(L)` 일 때이다(`eval::publication_sites`). 그 사이트의 사건(호출자 쪽, 호출 직후)은 행위자가 그 호출로 L 을 썼다는 기록이다. 값(`value`)이 있으면 L 의 재생 값이 되고, 없으면 그때부터 L 은 기록으로 고정되지 않는다(`effect_unsupported`). 위치 이름이 없는 포인터 저장(아무 위치나 썼을 수 있음)이나 `filter` 가 열린 저장은 상대를 열어 둔다(`ownership-undetermined`; 이미 지목된 행위자의 것은 행위자 집합을 바꾸지 않으므로 무시). 키의 등록·게시와 위치의 writer 는 합쳐서 판정하고(표의 규칙 그대로), 키에 등록이 없어도 위치의 writer 가 있으면 관계를 계산하며 사유에 둘 다 적는다("no registration or publication of K[k] in the record; the passed value was read from L, written by A"). 관계는 여전히 행위자 사이의 것이고 사건 순서를 읽지 않는다: 쓰기가 사용보다 뒤였어도(병합 순서) 그 행위자가 상대이다. 위치의 writer 중 기록이 관측하지 않은 것은 상대에 들어가지 않는다(종류의 등록과 같은 규칙); 무엇이 관측되지 않았는지는 발생의 `unobserved` 가 적는다.

**관측 목록과 정적 닫힘 (2026-10-24, R2 잔여)** [D][M: `test_observation_lists`]. 발생마다 `probes`(기록이 관측한 것: 연산의 `call`·`return`, 조건 지점, 쓰인 포획, 조건이 읽는 종류·위치(정적으로: state/tag/id, 정량화 종류, owner 종류, value, 파생 종류는 그 술어가 읽는 것; 포획한 위치 포함)의 writer 효과 지점과 위치의 게시 사이트)와 `unobserved`(관측되지 않은 조건 지점, 지점이 관측되지 않은 writer 효과 `OP.label`, 정적 닫힘이 준 관측되지 않은 writer·별칭 위치 `<종류|위치>: …`, 닫힘이 없으면 `<종류|위치>: static closure not given`)를 낸다. 결과 text 에 발생마다 `probes`·`unobserved` 줄, `real record knowledge` 의 종류·위치마다 `writer effects not observed:` 와 `static closure:` 줄이 붙는다. 정적 닫힘(`StaticClosure`: 묶인 필드의 상태, 정적 writer 수, 관측 지점에 있는 수, 관측되지 않은 위치)은 어댑터가 kz-static facts 에서 세우며(`docs/observer.md` §3.4), 재생에는 `adequacy` 의 `writers=`·`aliases=` 로만 들어간다. 다른 ADR-055 조건은 바뀌지 않는다. trace 머리말에 `closure kind|loc <이름> writers=<n> covered=<m> <상태…>` 와 `unobserved kind|loc <이름> <위치…>` 줄을 더했다(`trace_text` 가 쓰고 `parse_trace_text` 가 읽는다).

**P1 대안(반환 코드 고정 끄기)** [D]. `Options::outcome_narrowing`(기본 `true`, 현행)을 끄면 위 O4 의 고정을 하지 않는다: 기록이 열어 둔 조건은 열린 채 남고, 모든 설명에서 고정된 조건과 코드가 어긋나는 경우의 분석 입력 오류 검사는 그대로이다. PI 결정 P1(`STATUS_W0W3_20261024.md` §2) 전까지 현행을 유지하며, `kz replay --outcome-narrowing off` 로 대안을, `--compare-narrowing` 으로 두 읽기의 개수를 나란히 본다 [M: `test_outcome_narrowing_alternative`].

**trace 머리말(실제 기록만).**

```
record real
start_evidence none|init|state-capture|declared
adequacy kind <종류> [start=known|unknown] [writers=covered|uncovered] [aliases=covered|uncovered] [lifetime=known|unknown]
adequacy loc <위치> [start=...] [writers=...] [aliases=...]
observed <지점>                     기록 전체에서 붙어 있고 활성인 지점(lp_name, 사이트, 종료 지점)
owner <행위자> <키...>              (모델과 같은 줄) 소유 앱의 키
owner_level <행위자> confirmed|inferred|undetermined   없으면 undetermined: 소유자는 모름(owner_any)
owner_id <행위자> <값>              소유 앱의 id 값(생성 요청이 넘긴 값): 등록·게시 행위자 지목에만 씀
owner_core <행위자> yes|no          소유 앱이 코어 서비스인지(기록이 세운 경우만)
creator <행위자> <생성 행위자>
order established|merged            기본 merged: 서로 다른 행위자의 사건 순서는 확립되지 않음
loss begin / loss end               다음 사건부터 / 다음 사건 앞까지 사건이 빠졌을 수 있음
```

선언하지 않은 것은 확립되지 않은 것이다. `inferred`(예: `match-by-time`) 소유자도 키로 쓰되 근거 수준을 발생에 남긴다. `trace_text()` 가 이 형식을 쓰고 `parse_trace_text()` 가 읽는다.

**결과 text 의 추가(실제 기록만).** 머리의 `record real`; 발생마다 `possible {…}`, 조건 줄의 `unknown`, `passage-unknown`, `contradicted`, `reasons {…}`, 그리고 `summary`, `adequacy`, `identity` 줄; 끝에 `real record knowledge`(종류·위치마다 `fixed` 또는 `open {사유}`, 세운 적합성, 모순 여부, 손실 수, `partial`). `final state` 는 기록된 효과의 재생이며 `fixed` 인 것에 대해서만 뜻이 있다.

**한계 [I].** 손실은 기록 전체에 대한 것으로만 다룬다(어느 task 의 것인지 모름: 커널의 손실 보고는 CPU ring 만 말하고, 사건에는 CPU 가 없으며 백엔드는 문맥 전환을 기록하지 않는다). 일관된 상태 포획(state capture)으로 열린 위치를 다시 고정하는 일, 효과 지점의 시각 구간(EV-04)을 사건 단위보다 세밀하게 다루는 일은 하지 않는다. 실행 관계의 상대는 키 자원의 등록·게시 행위자와, 포획이 `cell` 을 준 경우 그 위치를 쓴 행위자뿐이다(조건이 읽은 다른 자원·위치의 writer 는 지목하지 않는다). 위치의 writer 는 기록이 관측한 것만이다: 관측되지 않은 writer(예: 시작 thread 의 EarlyInit 지우기)가 있으면 그것이 관측되었을 때 관계가 달라질 수 있다 [I]. 소유자가 `inferred` 인 결과의 판정은 A-CLK-PO 에 기댄다.

## 8. 검사기 규칙

`check_profile` 은 이름·sort·어휘·출처를 확인하고 **모든 오류를 모은다**. 아래는 규칙의 분류이고, 메시지는 안정되어 있으며 시험(`test_core_phi_check`, 사례 126개와 별도 블록 [M])이 각각을 한 번의 편집으로 일으켜 확인한다. 정상 프로필은 오류가 없고 경고·메모만 있다.

| 영역 | 규칙 |
|---|---|
| 머리 | `profile` 이 있다 (`CheckOptions::require_header`) |
| 출처 기록 | tier, kind, side 가 K0 어휘에 있다; tier 와 kind 가 짝이 맞다(§10); `side caller` 는 tier `contract-backed`/`requirement-derived`/`admitted`/`issue-derived` 에만(§5.7); `api_doc`/`requirement`/`code_admission`/`code_pattern` 은 file, lines, sha256 을 가진다; sha256 은 소문자 16진 64자; file 이 있으면 lines 가 있고 그 file 은 선언한 source 에 속한다 (`require_sources`); `issue` 는 `cite`; `researcher` 는 `authored` |
| source | rev 는 소문자 16진 7~40자; 같은 root 가 둘이면 안 된다; root 는 상대 경로(앞뒤 `/` 없음); guard 의 성분은 선언한 source, 버전 형식 |
| 주장의 출처 | 모든 주장(상수, 종류, 위치, 바인딩, 사이트, 연산, 효과, 규칙, rspec, trusted …)은 알려진 출처 기록을 하나 이상 인용한다 (`require_provenance`) |
| const | `ref eval` 은 기호와 번역 단위(source 에 속함)를 가진다 |
| kind | 상태 1~64개, 중복 없음; `initial` 이 상태 중 하나; scope 는 process/persistent; `table` 은 정수 상수; `ids` 는 네 항목 모두, 정수 상수, `base`·`base+serial_max`·표지가 32비트 안, `undefined ≠ reserved`; 태그 값 1~64개 중복 없음; transition·derived 의 상태가 존재; `Res<>` 순환 금지; `Id<K>`/`Res<K>` 의 K 가 존재하고 `Id<K>` 는 K 가 `ids` 를 가질 때만 |
| context, scope | `owner_kind`, scope 의 kind 가 존재; request/teardown 은 그 종류의 상태이고 request 가 teardown 앞 |
| loc, bind, site, init | loc 은 struct·field·holds; 바인딩 대상은 `state/tag/id/value` 형태이고 위치는 같은 struct.field; 패턴은 함수 이름·callee 가 있고 비교 상수가 선언됨; INIT 의 위치·상태가 존재하고 값 sort 가 맞음 |
| 식 | 이름이 풀림(`exec`/`result` 는 쓸 수 있는 곳에서만); 인자 수; 자원이 필요한 곳에 자원; `valid` 는 Id; `value` 는 위치; `tag` 의 둘째 인자는 그 종류의 태그; `owner` 는 Actor 와 `context`; 키 인자 sort; 비교 sort 호환; 순서 비교는 Int/Id/State; `in` 집합의 sort; 양화는 `table` 이 있는 종류 |
| op | 매개변수 role, 중복 없음; `lp capture` 조건은 stamp/key/id 매개변수만 읽고 상태를 읽지 않음; 효과에 `lp capture` 불가; 효과마다 lp 가 있음; `declared_trusted` 효과는 `refutable_by`(O1~O10)를 가짐; `declared_trusted` 효과의 보호 조건은 `filter` 가 아니라 `when`; `lifecycle` 의 상태가 존재, `id` 절은 `ids` 가 있는 종류에만, `all` 은 `table` 이 있는 종류에만 id 없이; `raise` 수준은 State/Int; `store *P` 의 P 는 `out` 매개변수; `store L` 의 위치와 값 sort; `settag` 의 태그와 값; `mark` 는 handler_begin/handler_end; `sync` 역할; outcome 의 코드는 정수 상수, `not_ready(L)` 의 L 은 conjunct, 중복 outcome 없음, `documented` 는 **공급자 쪽** 기록을 인용; `terminal` 은 `bind symbol` 필요 |
| hb | 층–강건성 짝(§5.6); spawn 의 create 연산·`out` 매개변수·entry 가 존재; phase 의 `from` 이 연산 또는 사이트이고 단계가 call/return/point; observe 의 poll·called_at·exit_outcome(정수 상수)·else_terminal(종료 연산)·watch(존재하는 종류); `obs_fatal` observe 는 `else_terminal` 필요 |
| rspec | 사용 연산 존재; `key` 는 자원; `requires` 의 각 conjunct 는 연산의 conjunct 중 **정확히 하나**와 text 가 같고(그래서 각각 선형화 지점을 가진다) 중복 없음; `appreq` 는 **side caller** 기록이고 그 tier 가 caller 쪽이 될 수 있는 것(§5.7; 손으로 넣은 기록·code-derived 기록은 안 됨); `allowed` 항목은 출처(side 가 있는)를 인용하고 코드·상수·연산이 존재; `harm` 은 K0 어휘 |
| trusted | class 는 declared_trusted/assumption; statement 가 있음; `refutable_by` 가 있고 O1~O10 |
| 중복 | 같은 종류의 선언 이름은 한 번만; 한 연산 안에서 conjunct/effect 레이블, outcome 코드, 한 종류 안에서 태그 이름은 한 번만 |

경고·메모(오류가 아님): 연산에 `bind symbol` 이 없음, `Id` 와 비교되는 정수 리터럴·상수가 0..0xFFFFFFFF 밖(§4), 패턴이 연산의 심볼과 다른 함수를 찾음, rspec 에 oracle·harm·authored 가 없음, `allowed none` 인데 appreq 가 미지정(→ 호출자 수준 Allowed 는 `unspecified_by_reference`라는 메모).

## 9. 정규형과 프로필 해시

`canonical_text` 는 프로필을 **하나의 정규 text** 로 쓴다: 선언을 종류별로, 이름순으로(출처 기록도 이름순), 연산의 conjunct/effect 는 레이블순, `prov`·`refutable_by`·`watch` 목록은 정렬하고 중복을 없애고, 정수는 10진, 공백은 정해진 모양, 주석 없음. 같은 프로필을 읽어 같은 text 가 나오며, 정규 text 를 다시 읽어 쓰면 **같은 text**(고정점)와 같은 해시가 나온다 [M: `test_core_phi_canonical`, `test_phi_cli`].

`profile_hash = SHA-256("kz-phi-profile\n" + canonical_text)`. 이것이 산출물 헤더의 `profile_sha256` 이다.

해시가 **변하지 않는** 것 [M]: 주석, 공백, 선언의 순서, 파일 분할(파일 이름순으로 읽지만 결과에 무관), 정수 철자(`0xc0000001` = `3221225473`), 인용 목록의 순서와 중복. 해시가 **변하는** 것 [M]: 상수 값, 인용한 줄·본문 해시, `researcher` 기록의 `authored`, 초기 상태, 상태·태그 값의 추가, outcome 클래스, Allowed, harm, 강건성, 버전 가드, 반박 의무. 파일 이름·줄 번호는 해시에 들어가지 않는다(진단용).

## 10. 출처 규칙

**tier–kind 짝** (K1 의 결정; SPEC rev 1.2 에서 확정 필요; `phi_provenance.cpp` 의 `tier_kind_compatible`): `contract-backed` ← `api_doc`; `requirement-derived` ← `requirement`(이 빌드의 K0 어휘 `KZ_TIER` 에 `requirement-derived` 가 아직 없어 `requirement` 기록은 검사기가 그 사실을 말하며 거절한다); `admitted` ← `code_admission`; `code-derived` ← `code_pattern`; `issue-derived` ← `issue`; `researcher` ← `researcher`; `unreferenced` ← `none`. `kz-phi stats` 는 항목마다 인용한 기록 중 **가장 강한** tier 와 **가장 약한** tier(SPEC 1.6: 섞인 항목은 부분 중 가장 약한 tier 를 갖는다) 두 가지로 센다(순서는 앞의 열거 순서; `unreferenced` 가 가장 약함).

**인용 본문의 해시**: `file` 의 `lines a b` 의 각 줄 뒤에 LF 를 붙인 바이트열의 SHA-256 이다. `sed -n 'a,bp' file | sha256sum` 과 같다(마지막 줄에 줄 바꿈이 없으면 있는 것으로 본다; CR 은 줄의 일부로 남는다). 이 프로필의 인용은 `sed`+`sha256sum` 으로 따로 계산해 적었고, 저장소의 코드가 만든 값이 아니다. `kz-phi verify-prov --root <번들>` 와 ctest `phi_profile_provenance` 가 번들에서 다시 읽어 맞춘다: V1 프로필의 98개 인용을 모두 대조했다 [M]. 번들이 없는 머신에서는 시험이 SKIP(77)으로 보고한다.

**개정 고정(revision pin)의 확인** [M: `test_core_phi_provenance`, 번들에서 4개 source 모두 일치]: `verify-prov` 는 `source NAME root R rev X` 마다 그 트리에서 체크아웃된 커밋을 git 메타데이터 **파일**에서 읽어(`.git` 디렉터리 또는 `gitdir:` 파일, `HEAD`, loose ref 또는 `packed-refs`; git 명령은 돌리지 않으므로 읽기 전용 번들을 건드리지 않는다) `rev` 로 시작하는지 본다. 다르면 `MISMATCH` 이고 종료 코드 1, 메타데이터가 없으면 `unverifiable` 로 이유와 함께 출력한다(종료 코드에는 영향 없음). **확인하지 않는 것**: 작업 트리에 커밋되지 않은 수정이 있는지(index 와 객체가 필요하다). 출력의 해당 줄이 그렇게 말한다. 인용한 줄 자체는 위의 해시 대조가 덮는다.

**side** (기록과 rspec 의 짝에 대해 저자가 판단): `supplier`(공급자가 반환·문서화하는 것), `caller`(호출 코드가 달성/처리해야 할 것을 말하는 텍스트), `rule`(텍스트 없이 도구의 고정 규칙). `appreq` 는 caller 기록만, `documented` outcome 은 supplier 기록만 인용할 수 있다.

**손으로 넣은 항목**: tier `researcher`(또는 `unreferenced`)가 가장 강한 항목과 `declared_trusted` 항목을 보고에서 센다(`kz-phi stats`). 호출자 수준의 인용도 항목으로 센다: `appreq RSPEC` 하나, Allowed 항목마다 `allowed RSPEC.KIND#N` 하나, 그리고 그 기록들의 tier·side 를 `caller level` 절에 줄마다 적고, 손으로 넣은 기록(또는 기록 없음)을 인용하는 appreq·항목의 수를 센다 [M: `test_core_phi_allowed`]. V1 프로필(출처 기록 109개): 항목 91개(가장 강한 tier 기준 contract-backed 3, code-derived 85, researcher 3; 가장 약한 tier 기준 contract-backed 1, code-derived 78, researcher 12), 손으로 넣은 것 3개(hb R1, kind cell, op abort), `researcher` 기록을 *하나라도* 인용하는 항목 12개, `declared_trusted` 3개(효과 `CFE_ES_CleanUpApp.retire`, trusted A-FRESH, A-OWNER), 호출자 수준: 지정된 appreq 1개(`evs_h_appid_note`, contract-backed, side caller), Allowed 항목 0개(`allowed none`), 손으로 넣은 인용 0개 [M]. 프로필 해시 `bc8a3401e6b53b23642e258363dd27eabd7187eea2790f463a45171ae5ec90a4`. 정확한 목록은 `tests/phi/data/profile_summary.golden`.

## 11. SPEC·자료와의 불일치

고정된 소스(`cfe c5fb2b4d` 등)를 다시 읽어 확인한 것만 적는다.

1. **`CFE_ES_GetAppID` 실패 가지의 저장**: SPEC 은 `cfe_es_api.c:758–760` 이라 쓴다. 저장 문장 `*AppIdPtr = CFE_ES_APPID_UNDEFINED;` 는 **:759** 이고 758 은 `{`, 760 은 `Result = ...` 이다. 의미는 같고 범위가 저장 한 줄보다 넓다. 프로필은 :757–761 을 인용한다(`es_getappid_fail`). 성공 가지의 저장은 :754.
2. **`CFE_ES_CleanUpApp` 호출 보호**: SPEC 은 `cfe_es_apps.c:1110–1113` 이라 쓴다. 해제 조건은 **:1112**(`IsUsed && Type == EXTERNAL`)와 **:1114**(`AppState > RUNNING`)에 있다. 1110·1113 은 `for`·`{` 이고 `AppState` 시험은 1113 밖이다. 프로필은 :1109–1114 를 인용한다.
3. **코어 순서의 출처**: `cases/mapping_v701.md` 는 코어 객체 순서가 `cfe_es_objtab.c` 로 고정된다고 쓴다. 실제 순서는 생성된 `cfe_static_module_list.c`(빌드 구성)에서 오고, SPEC §3.6 의 서술이 맞다. 프로필은 순서를 *쓰지 않고* 루프 구조(R18, R19)만 쓴다.
4. **R19 의 버전 가드 `cfe >= 6.4.2`** 는 SPEC 의 값이다. 이 프로필을 쓰면서 6.4.x 소스를 읽지 않았으므로 가드의 하한은 **재검증하지 않았다**(`r_guard_core_serial` 에 적음).
5. **SPEC rev 1.2**: §2.3 의 DSL 발췌는 아직 `contract` 를 쓴다. 이 구현은 `rspec`/`requires` 를 쓴다. tier–kind 짝(§10)도 SPEC 에 명시가 없어 K1 이 정했다.
6. **ES AppId 의 base**(`CFE_ES_APPID_BASE = 0x110000`)는 SIMPLE 모드의 값이다. STRICT 빌드에서는 `0x02110000` 이다. 번들의 설정으로 정해지는 *basis* 의 성질이므로 basis 레코드와 일치해야 한다(K4a 가 대조).
7. **`CFE_ES_CleanUpApp` 의 생명주기** (K1 round 4 에서 고침): 이전 프로필은 정리를 `LIVE -> FREE` 한 걸음(`:1632–1635` 의 해제)으로 두었다. 고정된 소스에서는 먼저 **:1498** 에서 레코드를 `RESERVED` 로 두고(`CFE_ES_AppRecordSetUsed(AppRecPtr, CFE_RESOURCEID_RESERVED)`, 주석 "Set the record to RESERVED", :1493–1498), 나머지 자원을 잠금 밖에서 푼 뒤, 끝에서 아직 `RESERVED` 일 때만 해제한다(:1632–1635). 그 사이에는 레코드가 `LIVE` 가 아니므로 `CFE_ES_GetAppRecordByContext` 가 그 앱의 task 에 대해 실패한다. 지금 프로필은 효과 `retire`(`LIVE -> RESERVED`, `es_apps_cleanup_retire`, 신뢰한 보호 조건 `EXTERNAL && AppState > RUNNING` 은 이 효과로 옮김)와 `free_record`(`RESERVED -> FREE`, 보호 조건은 코드의 시험 :1632)를 두고, 종류 `es.app` 의 간선도 `LIVE -> RESERVED on retire`, `RESERVED -> FREE on free` 로 바꿨다. 그 창은 golden `replay_cleanup_window` 가 보인다.

모형의 근사(`02_researcher.kzp` 에 `researcher` 기록으로 남김): EarlyInit 이 함수 포인터로 불려 패턴 언어가 이름을 붙일 수 없어 R18 의 기준점을 둘째 루프의 첫 `LockSharedData` 로 잡았다; 실패 가지의 out 포인터 저장에 패턴이 없어 그 선형화 지점을 뒤따르는 `UnlockSharedData` 에 묶었다(두 가지가 모두 그 지점을 지나므로 보호 조건은 `filter`); `evs.reg` 는 정확한 AppId 로 키를 잡고 세대는 모형에 없다; `es.self` 는 task 레코드와 App 레코드의 조회를 App 레코드 하나로 줄였다.

## 12. 증명 상태와 한계 (D20, PS2)

- 이 도구와 프로필은 **무엇도 증명하지 않는다**. 목록에 의무로 적은 것(O3, O4, O9)은 반박 가능한 항목이다. `declared_trusted` 와 `assumption` 은 "증명됨"이 아니다. 출력 어디에도 의무를 적어 두었다는 이유로 증명·검증·인증이라는 말을 쓰지 않는다.
- 규칙 하나로는 수용하지 않는다(QT-4/Q15). `PENDING` 은 `TOLERATED` 로 세지 않는다.
- 손으로 넣은 기록(`researcher`, `unreferenced`)과 code-derived 기록은 `side caller` 가 될 수 없다(검사기 오류). 그래서 연구자가 쓴 appreq 도, 연구자 규칙으로 판정하는 Allowed 항목도 없다(QT-4, QT-9).
- 신뢰한 보호 조건(`declared_trusted` 효과의 `when`)은 재생에서 **반박만** 될 수 있다: 그 지점이 관측되었는데 조건이 거짓이면 `guard_failure ... refuted` 로 보고한다. 조건이 참이었다는 관측은 그 조건을 증명하지 않는다.
- 구현하지 않은 것 [I]: `retry_until_ready` 밖의 Allowed 항목의 horizon monitor(→ `PENDING` 으로 남음); 생명주기 간선(`transition`)의 합법성 검사(재생은 효과가 가리키는 상태를 그대로 쓴다); 세대와 재시작; 패턴을 소스에 맞춰 찾기(K4a); 관측 규칙의 `called_at`·쓰기 집합 완전성·falsifier; operational domain; 패턴 언어의 "포인터를 통한 저장"과 "간접 호출 이름 붙이기".
- 시험의 trace, 값, 식별자는 모두 합성이다. cFS 를 실행하지 않았다.

## 13. 시험 [M]

| 시험 | 확인하는 것 |
|---|---|
| `test_core_phi_parser` | 어휘·구문 오류 71개를 줄:열과 함께; 식의 우선순위·평탄화·출력의 고정점; 받아들이는 형태 전부 |
| `test_core_phi_check` | §8 의 규칙마다 한 번의 편집으로 오류를 일으킴(126개); 손으로 넣은 기록의 `side caller`·그 기록을 인용한 appreq·`declared_trusted` 효과의 `filter`·Id 와 범위 밖 정수의 비교 경고; 정상 프로필은 오류 없음 |
| `test_core_phi_canonical` | 정규 text 의 고정점(`when`/`filter` 보호 조건 포함); 해시의 불변성(주석·공백·순서·파일 분할·정수 철자·인용 순서)과 변화(14가지 의미 편집이 모두 다른 해시) |
| `test_core_phi_allowed` | `derive_caller_allowed` 의 모든 가지; 손으로 넣은·code-derived 기록의 `side caller` 는 판정하지 않고 appreq 를 지정하지 않음; `stats` 의 호출자 수준 인용 |
| `test_core_phi_provenance` | 인용 해시(경계·CR·마지막 줄), tier–kind, 번들 검증(임시 트리), 개정 고정(임시 트리의 git 메타데이터: 일치·불일치·확인 불가), source 해석 |
| `test_core_eval` | 모든 구성의 구체 의미; 추상 domain 의 삼치 논리·구간·상태 집합 |
| `test_property_eval_agreement` | 추상 결과가 구체 결과를 포함함(§6); 잘못 만들어진 소유자(다른 종류·다른 키 폭)를 섞은 상태 포함 — round 4 이전의 `owner_of` 로 돌리면 넓힌 추상화에서 위반 92개로 실패한다. 2026-10-24: 실제 기록처럼 연 추상화(`open_kinds`, `open_locs`, `owner_any`, §7.6) 48,000 평가에서 위반 0 |
| `test_core_replay` | 효과, 단언한 보호 조건의 반박·불일치 보고(`guard_failures`)와 `filter` 의 가지 선택, HB 템플릿 4개, 상태 기계, 범위·종료, 가드·버전, trace text, 무작위 trace 400개에서 재생이 죽지 않고 두 번 돌려도 같은 결과(총함수) |
| `test_core_realrecord` | §7.6: 모든 적합성을 선언한 실제 기록은 모델 trace 와 같은 결과; 시작 상태를 모르면 초기 상태로 두지 않음(모델은 VIOLATING, 실제 기록은 UNDETERMINED → `INCONCLUSIVE(start_state_unknown)`); writer 미포괄·writer 지점 미관측·수명 미상·손실·(순서 미확립 시) 다른 행위자의 쓰기이면 기록된 쓰기가 고정하지 않음; 결정 뒤의 손실은 결정을 바꾸지 않음; `PENDING`·`OPEN` → `INCONCLUSIVE(horizon-open)`, `NOT_READY_UNJUDGED` → `INCONCLUSIVE(caller_requirement_absent)`, `NOT_EVALUATED` → `INCONCLUSIVE(not_evaluated)`; 반환 코드에 의한 고정과 분석 입력 오류(`Qv`·`Qn` 모두 UNSAT); 관측되지 않는 지점(호출 구간), 소유자 미확정; 머리말 text 의 왕복 |
| `test_core_monitor` | `retry_until_ready` monitor(§7.5): 재시도 뒤 성공(TOLERATED + READY, 마지막 허용 시도의 READY 포함, 다른 키·다른 행위자는 다른 인스턴스), 소진(B 번 모두 미준비 → 모두 VIOLATING, 그 뒤 시도는 새 인스턴스), `codes` 밖 결과의 REJECT, 기록이 재시도 중에 끝남(PENDING, 열린 호출), 키 미고정·손실·세대 경계(관측됨/관측 안 됨)의 보류, `ref eval` bound(값이 없으면 monitor 안 돎), 판정 항목이 둘이면 REJECT 를 VIOLATING 으로 만들지 않음, P1 대안(고정 끔)에서는 아무것도 결정하지 않음, 모델 trace 도 같은 monitor |
| `phi_golden_*` (17개) | 실제 프로필의 정규 text, 해시·통계, 합성 trace 12개로 만든 재생 결과 15개(predicate 유무·버전을 바꾼 변형 포함; `kid_outcomes`, `effects_core`, `effects_external`(보호 조건의 반박·불일치 5개), `cleanup_window`(정리 중 RESERVED 창) 포함) |
| `phi_profile_check`, `phi_profile_provenance`, `test_phi_cli` | `kz-phi` 가 라이브러리와 같은 출력을 냄, 종료 코드 |

golden 을 의도적으로 바꿀 때: `KZ_GOLDEN_UPDATE=1 ctest -R phi_golden` 후 `tests/phi/data/` 의 diff 를 읽는다.

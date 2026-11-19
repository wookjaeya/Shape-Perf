# The generated observation plan (`kz-static plan`, plan rules revision 2)

`kz-static plan` turns the rows of `kz-static relations` into an observation plan by rule: which points of the program a
derived build records, and with which values.  It takes no field, callee or case name; the plan comes from the facts,
the basis and the framework-layer data only, and it states what to record, never what to expect.  Code:
`lib/llvm/plan_gen.hpp`, `plan_gen.cpp`, `tools/kz-static/cmd_plan.cpp`.  Tests: `tests/static/test_static_plan.cpp` on
the fixture `tests/static/fixtures/plangen.c`, and `plankey.c` for the key argument of revision 2.

Revision 2 (2026-11-16): the key argument of a `use-call` or `creator-call` site that is a character array of a known
extent, read at a point that dominates the call (ARG_TYPES, facts of `kz-static extract --object-access`,
`docs/static_facts.md` 5s), is recorded at the call as `string=validated` with `bound` = min(extent, `KZ_RT_STR_MAX`);
the sidecar then has a line `bound string-extent source=ARG_TYPES`.  Facts without ARG_TYPES give the plan of revision 1
except the revision text.

## 1. Command and inputs

```
kz-static plan <facts> [--basis B] --framework F --select observation-set --order method-priority
               --budget tier:D07|tier:directed|all|rows:<n> --budget-reason TEXT [--budget-disclosure TEXT]
               -o MANIFEST --sidecar SIDECAR --modules-out TABLE --exes-out LIST
```

The command opens the files named on its command line and no other.  The facts container; the basis container the facts
were read under (required when the facts name one, refused when it is another one or not a basis container); the
framework-layer data file (section 4; a line that is not an entry is refused, the file names no other file).  Each input
is listed in the sidecar with the sha256 of the bytes read.  The rows are computed in-process by `relations_rows` over
the facts loader the relations use (`RelFacts`), so the plan and the relations text cannot disagree on a row.  Exit
status: 0 ok, 1 an input does not read or verify, 2 a usage error or a refusal (no output is written).

## 2. Selection, priority order and budget

- Selection `observation-set`: every row (the method document's observation set, revised rule: no label and no proof of a
  row removes it).
- Order `method-priority`: a port of the method's first-step rule over the row text, row by row: the label (`R`, `J`,
  `H`, `0`, `T?`, `F`, `E`), the order class (`ENF`, `FIXED`, `COND`, `RELOC`, `NOPAIR`, `UNK`), the reach mark and one
  disposition (`D01`-`D13`), as written in `plan/ovm_rev1_20261116/S1_RETALLY.md` sections 2.1-2.4.  Tier 1 = disposition
  `D07` (label `R` or `J`, a counterpart, order class `RELOC`); tier 2 = the other rows of order class `RELOC` (the other
  directed-run candidates); tier 3 = the rest.  Within a tier the order of the relations text.  The method text governs;
  the agreement of the port with the probe table of each build is a measured table of the step that built it.
- Budget: `tier:D07` plans tier 1 whole, `tier:directed` tiers 1 and 2, `all` every row, `rows:<n>` the first n rows of
  the order.  The sidecar records the value, the reason and the disclosure given, the number planned, the rows cut (each
  listed by its use-site key with `need row <key> observation not-planned reason=budget_not_run(row outside the budget)`),
  what is unresolved and the sensitivity (not measured).

## 3. Observation rules

Each rule derives sites from a planned row (or from the actors and the framework-layer data) and names itself in the
site's label.  Sites are deduplicated by (unit, function, phase, place, callee, line); the values of one site are
merged by their source (`from=`).  At most four values per site (the slots kzgcc 0.3 binds); a site that would need
more names the dropped value as a need.

| Rule | Sites |
|---|---|
| `use-call` | path A: at-call of the row's use (the key argument as a pointer, the other arguments) and after-call (`ret`, and `out<k>` for every parameter the callee stores through: PARAM_FLOW out-store rows, or the P0 out parameter); path B: at-call of every site of the row (the shared field's argument) and after-call (`ret`) |
| `key-text` | for a key parameter that reaches a byte-string comparison of P0: the first call (by position) the parameter reaches at depth 0 in the key-parameter function whose PARAM_FLOW guard has a conjunct saying the parameter is not null (after the negations are moved inside); the argument there is recorded as `string=validated` with `bound` = the constant of a bounded NUL search of the same parameter in the same function whose result is tested (`memchr(p, 0, N)`, ISO C 7.24.5.1; `strnlen(p, N)`, POSIX.1-2017), else `KZ_RT_STR_MAX` (64, `runtime/include/kz_rt.h:82`; the runtime copies up to the NUL or the bound and flags a cut).  Premise recorded: the string is NUL-terminated within the bound or the cut is flagged.  Without such a test: the key stays a pointer and a gap `record_insufficient(key text)` is named |
| `creator-call` | path A: the same two rules at the call of every `creator:` line of the row (its key parameter: the parameter of the creator that reaches a byte-string comparison) |
| `handling-chain` | path A: after-call of every call a `handling: ... returns the result of ...` line names; for the `terminal` line (the part before the F8 explanation of the callee): at-call of the terminal call, after-call of the calls and store sites of the positions it names (`place=store`, the function whose body holds the position), after-call of the calls whose results the line's guards test (`call-result X`) and of the call whose result a guard's local holds (`the result of X is used in F`) |
| `writer` | path B: the writes the row lists: `assign`, `inc-dec`, `aggregate-assign` as store sites; `memset`, `memcpy`, `out-pointer` as at-call and after-call of the call (an out-pointer write records `out<k>` of the argument that is the address of the field) |
| `spawn-call`, `writer-after-spawn` | path C: at-call and after-call of the spawn call; store sites of the creator's writes after the spawn |
| `actor-root`, `actor-create`, `actor-start`, `actor-owner` | for every actor root of a planned row (the actor names of its lines) and every startup-script entry: the entry of the root; the P0 thread-creation calls the root's SPAWN_WRAP chain reaches (from the rows whose entry is the root, or the spawn calls whose resolved entries include it, through the next function of each row) with at-call (start routine, creation argument) and after-call (`ret`, the thread handle through the P0 out parameter); the entry of each start routine at that call (a function argument, the fixed entries the chain passes to the primitive's wrapper, else the root) with its parameters; the store sites of a parameter after a spawn of the chain in the spawn's function (LOCK_PATHS after-spawn write rows that a depth-0 PARAM_FLOW store row of that function names).  A creation argument that is one constant for several roots carries no tie: the start routine's entry is planned without its parameter and a gap `link_unlinked(no creation-argument channel)` is named.  No owner store found: `need ... owner-request not-planned` |
| `stop-event` | the entry of each framework-layer data function that applies to the build (section 4), with its parameters |

Not planned in this revision, named per planned row: `wait-result`, `lock-result` (rule added in the manifest revision
4), path A `event-send` (the effects of a result are not in the facts), path C `read-value` (a load is no site kind of
kzs-sites 3).  A call whose function calls the same callee again on the same line is not planned (the binding is by
line): `record_insufficient(two calls of one callee on one line)`.

Every reason a need or gap line can carry is in the table `plan_reasons()` (`plan_gen.cpp`), each a reason of the judgement
rules' vocabulary (`plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` 3.5) with a sub-reason; `test_static_reasons` checks the
table against `tests/static/data/reason_vocabulary.txt`.

## 4. Framework-layer data

`semantics/framework_api_roles.txt`: one entry per role per framework version, for the roles of the method's stop event
(run loop, app exit):

```
role <run-loop|app-exit> function=<name> def=<file>:<line> def_sha256=<hex> decl=<header>:<line> decl_sha256=<hex> version="<text>"
```

An entry applies to a build when the build's facts hold the definition file (a path with that tail) with that sha256 and
define the function on that line.  Provenance: every entry is in the output of the mechanical scan of the version-pinned
public headers and passes the check (research `env/tools/framework_scan.py scan|check`); the lint `case-ids-data` keeps
case identifiers out of `semantics/`.

## 5. Outputs

- **Manifest** (`kzs-sites 3`, read by kzgcc 0.3 and the runtime 1.4): a comment header with the sidecar's sha256; one
  `source build` statement (rev = the basis artifact hash: every source file read is hashed in its FILE_HASHES); one
  `module` statement per module of the module table; `site` and `value` statements.  No `identity`, `control`, `bind` or
  `equiv` statement.  Site ids and labels from the sorted site keys: `s<id>.<rule>[+<rule>...]`; every site has `ref=`
  (the rows and facts it came from; the full list is in the sidecar).  `tu=` is the unit's path relative to the bundle
  root as the basis records it, or, for a unit outside the bundle root, relative to the common directory of the basis
  units outside it (a site whose `tu=` names two basis units is not planned).  Values whose types the facts do not give
  are `u64` (the plugin zero-extends from the operand's own type), results `s64`, keys `ptr`.
- **Sidecar** (`kz-plan-sidecar 1`): the generator identity; `input` lines (role, sha256, path); the facts and basis
  artifact hashes; the rules, selection and priority revisions; the class sizes; the budget; the bounds (string bound,
  slots, site ids); a `row` line per row (use-site key, number, tier, position, label, order class, reach, disposition,
  planned or cut); a `site` line per site (key, refs); `map <row key> <site ids>`; `need` and `gap` lines; `creation`
  lines (the creation calls with their roots, start routines and tie); `stop-event` lines; the module table and the
  executables.
- **Module table** (for the derive script, `instrument/gcc/scripts/derive_args.sh`): first line
  `# kz module table; plan sidecar <path> sha256=<hex>`; `<id>\t<name>\t<prefix>` lines and `default\t<id>`.  Modules:
  the executable of the basis (`binary.path`) and every module whose units the basis TUS names (`member` "module <name>
  (...)"); ids by sorted name; the prefix of a module is the common directory of its units (per unit directory, or per
  unit, where that directory holds a unit of another module), as an absolute path of the reference build.  A derived
  build in another directory maps these paths to its copies.
- **Executables**: the executable of the basis and the loaded file of every module, as absolute reference paths.

## 6. Regeneration check

`env/tools/plan_regen.sh <generator or its xz copy> <args file>` runs the generator again with the recorded arguments and
compares: manifest and sidecar byte for byte, the module table and the executable list after their first line and in it
except the sidecar path.

## 7. Path A control selection (control selection rule revision 1, 2026-11-16)

Written and committed before any plan is generated with it.  It is read by the generator of plan rules revision 3
(manifest revision 4, `--manifest 4`); plans of revision 2 (`kzs-sites 3`) do not use it.

Purpose.  The judgement rules count a boot as the prior test of the name-binding rule only when its plan holds path A
rows outside the relation the rule was fitted on and the same-actor control row, under a recorded budget
(`plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` 8.12 (ㄷ)).  The plan of revision 2 holds, of the path A rows, only
what the tier order reaches (one row on c1_pre_tx, none on the other four builds: results document
`plan/full_stage_results_20261116.md` 6.7 `rules-04`).  The control rows are therefore chosen by a rule over the row
lines, never by a row id, a use-site key or a case value (lead decision of that document, 6.10 item 2).

The rule.
- Candidates: every path A row of the relations the generator computes in-process (`relations_rows`), whatever its tier
  and whether the tier order plans it.
- (a) `probe-create`: a row that has a `handling: probe-create` line (the same-actor control of 8.6 (a)).
- (b) `creator-site`: a row that has at least one creator site.  The creator sites of a row are the create calls named
  by its `creator:` lines and by its `not a pair: key constants differ` lines: that exclusion is a filter for candidates
  only, it is not read as a refutation and it does not reduce the row's creator sites (8.12 (ㅁ)).  For a row of (a) the
  create calls named by its `not a pair: probe-create` lines are creator sites as well (8.12 (ㄴ)).  A
  `not a pair: same actor only (program order)` line names no creator site of this rule.
- The control set is (a) then (b): the rows of (a) in the order of the relations text, then the rows of (b) that are not
  in (a), in the same order.
- Budget: `--control-budget all|rows:<n>` with `--control-budget-reason TEXT` (both required with `--manifest 4`).  The
  sidecar writes the line `control-budget value=… reason=… rows=… planned=… cut=… complete=yes|no` before its first row
  line; the value is given on the command line before the generation (the generator reads no other input for it).  The
  rows of the control set past the budget are cut and listed by their use-site key.  `complete=no` means the plan does
  not hold the whole control set: a boot with that plan is not a prior test in the sense of 8.12 (ㄷ), and its link
  stays `unlinked` on that ground.
- A control row is planned apart from the tier order and by the same observation rules as a tier row, with every one of
  its creator sites as a `creator-call` site.  Its sidecar row line carries `control=<a|b|a+b>` and
  `control-state=planned|cut`; its tier state is unchanged (a row may be planned by both).
- The rule reads the row lines only.  Expectations per control row are [I] until a run, and are committed before the
  boot whose plan holds them, not here.

## 8. Plan rules revision 3 (manifest revision 4, `--manifest 4`, 2026-11-16)

```
kz-static plan <facts> ... --manifest 4 --control-budget all|rows:<n> --control-budget-reason TEXT --requests-out REQUESTS
               [--namespaces F]...
```

Without `--manifest 4` (or with `--manifest 3`) the output is that of revision 2, byte for byte apart from the generator
identity; the revision-3 options are refused there.  With it the same selection, priority, budget and observation rules
of sections 2-3 apply, and:

- **Manifest `kzs-sites 4`** (`runtime/tools/manifest/kz_rt_manifest.hpp`; versions 1-3 parse as before): a `plan`
  statement (id = the first 16 hex digits of the sha256 of the facts artifact hash, the rule revisions, selection,
  order, budgets and the facts' semantics join; `facts=` the facts artifact hash); per site `skey=` (the first 16 hex
  digits of the sha256 of the site key), `row=` (the use-site keys of the rows the site serves), and for a call site
  `col=` (the call's column) and `nth=` (its ordinal among the calls of the callee in the function, in source order),
  so that two calls of one callee on one line are two sites, not a gap; `place=load field=Type.field` for a read;
  `need` and `gap` statements (the sidecar's need and gap lines; a planned need names its sites); at the at-call of each
  thread-creation call the actor rule plans, a `spawn` statement (`wrapper=kz_rt_posix_create`, the start routine,
  creation argument and thread handle positions of the P0 entry) and `identity … decoder=spawn-token`.  The wrapper is
  the runtime's creation wrapper, which the runtime step after this one provides [I]; kzgcc 0.3 does not read
  `kzs-sites 4` (the plugin of the step after it does).
- **Control selection** (section 7) with its `control-budget` line, `control=`/`control-state=` on the row lines and
  `need row <key> control not-planned reason=budget_not_run(control row outside the control budget)`.
- **Creator sites** of every planned path A row: the create calls of its `creator:` lines and of its
  `not a pair: key constants differ` lines, and in a probe-create row of its `not a pair: probe-create` lines.
- **Wait rule** (`wait-cond`, `wait-exit`): for every poll or count-bounded loop (FACT_LOOPS) at or before a site of the
  row in the site's function: when FACT_LOOPS has a predicate form or LOOP_DEFS recognises one, the after-call of the
  calls whose results the condition reads (FACT_LOOPS exit tree, the in-body and in-condition definitions of LOOP_DEFS)
  and the loads of the fields it reads (the exit tree outside call arguments, AWAIT_L1's relevant loads), and the
  `line-start` of each break, return or goto exit; a condition exit is no statement
  (`gap … record_insufficient(the exit is no statement)`).  Otherwise
  `need … wait-result <loop> not-planned reason=plan_missing(wait predicate not recognised)`.
- **Object rules**: path C: `read-load` (the load of the read field at the read), `object-index` (the after-call of each
  call whose result is a subscript of the read's access path: the OBJECT_ACCESS index root of the read, else its READS
  index root), `lock-call` (the after-call of each acquisition or wait and the at-call of each release of a lock
  operation of LOCK_OPS called in the creator's or the reader's function, with its first argument).  Path A:
  `object-delete` and `object-retire` from the namespace data given (`--namespaces`, `semantics/framework_namespaces_*.txt`):
  an entry (a `kind` or `table`) whose lookup, create, copy or create-check function is the callee of the row's use or
  of one of its creator calls, and whose operation applies to the build (the facts hold the file of its `def=` with the
  data's sha256 and define the function on that line); for each `delete` of the entry, the entry of the function with
  its parameters and the after-call of the calls on the line of its `at=` (the retire call); for each `clear`, the
  store at its `at=` line.  No entry: `need … object-delete not-planned reason=plan_missing(no namespace data entry)`.
  The sidecar's `lifetime` lines list each operation and whether it applies.  No framework API or kind name of the
  object rule is in the generator: they come from the data file, which the lint keeps free of case identifiers.
  (2026-11-18: the sentence read `No API or kind name is in the generator`; the generator does hold three names of its
  own, each with its ground: the two C library length checks of the key-text rule, section 3, and the name of this
  repository's runtime wrapper in the spawn binding, section 8.)
- **Effect calls** (`effect-call`): for the use of a planned path A or B row, the at-call of each event send, system log,
  application exit or terminal effect of RESULT_EFFECT that has a call, with the argument a SEMANTICS arg-role row
  names for its callee (the role's first word).  A path A row's `event-send` need names those sites.
- **Value requests** (`--requests-out`, `kz-plan-requests 1`; the sidecar line `requests sha256=… file=… rows=…
  requests=… planned=…` names it): for a planned path A or B row whose handling label is T?, J? or F, or whose `absence:`
  line has an open effect: the result and out values at the use's after-call, each effect call with its role argument
  (complete or open with the reason), and the branch taken (`not-planned`: a branch is no site kind).
- The sidecar header is `kz-plan-sidecar 2`, with `manifest-revision 4`, `plan-id`, the `control`, `control-budget`,
  `requests` and `namespace-data` lines, and `spawn` lines.  It names no output path (only the requests file's name).

Not planned in revision 3, each named: the lock calls of path A and B rows
(`plan_missing(no rule of this revision)`), the branch taken, an effect with no call.

## 9. Plan rules revision 4 (2026-11-18; source only, the MC kz-static is not rebuilt yet)

Revision 4 is revision 3 with three changes of `--manifest 4`; the default path is revision 2 as before.

- **Both handling labels on the row line.** The tier order (sections 2 and 8) uses the label the generator reads from the
  row's text (`handling: terminal` gives R, and so on).  The relations text of facts with RESULT_FLOW and RESULT_EFFECT
  also carries a fact-derived label (`handling_label:`), which the value-request rule reads.  The two can disagree on a
  row.  The sidecar row line now writes both: `label=<text label> fact-label=<fact label or ->`.  The tier order is
  unchanged: it still uses the text label (the earlier budget stays regenerable); a selection rule that reads no
  handling label is a later step.
- **Key-clearing calls.** A row's `key clearing:` lines (relations rules revision 12: a call every write of the key it
  reaches stores a constant) are no creator sites.  Each is an at-call site of the rule `object-delete`, and the row's
  `object-delete` need is planned with those sites (and with the sites of a matching name-space entry, when one applies).
  A row whose only `creator:` lines named such calls is no longer in control set (b).
- **Event sends of the other outcomes.** The row's `event-send` need lists the event sends and system logs of
  RESULT_EFFECT on a failure or absence outcome of the row's use.  Those on another outcome (often open effects of the
  success value, reached on a later pass of a loop) are a need of their own, `event-send-other-outcome`.

## 10. Plan rules revision 5 (2026-11-18; text only, committed before the code that implements it)

Revision 5 is revision 4 (`--manifest 4`) with the sites and sidecar lines that the name-binding items and the
use-condition elements of the judgement rules read (`plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` 8.13 items 9-16).
It is written before the generator is changed; the build that implements it declares its differences against the kept
revision-4 plans.  The manifest grammar is `kzs-sites 4` as it is: every new site is an entry, an at-call, an after-call
or a store, so no parser changes.  The default path (no `--manifest`) is revision 2 as before.

**What does not change.**  The selection (`observation-set`), the priority port, the budgets and the control selection
of section 7 are those of revision 4.  **The tier order keeps the text label** (section 9): the row line carries both
labels, `label=<text label> fact-label=<fact label or ->`, and no rule of revision 5 selects or orders rows by either.
A selection rule that reads no handling label is a later text of its own and is not built with this revision.  A plan
made from the revision-1 name-space files has the output of revision 4 apart from the generator identity and the
`rules` line; the lines below need the files of coverage rule revision 2 (`docs/semantics_coverage.md` section 8).  A plan
names one set of name-space files only, and its sidecar `input namespace-data` lines say which.

**Wording of the control budget reason.**  Section 7 calls a boot with every control row in its plan "the prior test of
the name-binding rule".  The rows that sentence was written for were read when the rules were written: their runs are
control runs of design rows, not a prior test (judgement rules 8.13 item 1).  `--control-budget-reason` is a
command-line text; the plans of the next cycle give it in these words.  The earlier wording stays in the kept plans
only.

### 10.1 Sidecar lines of a name-space entry

For every planned path A row (tier or control) whose use or creator callee is a `lookup` or `create` of an entry that
applies to the build (section 8, object rules), one line; fields in this order, `-` for a field with no value:

```
namespace-entry row=<use-site key> data=<path> sha256=<hex> line=<n> kind=<constant> value=<integer> kind-at=<file>:<line>
  lookup=<function>:<applies|not-applicable> create=<function>:<applies|not-applicable>[,...]
  compare=<function>:<applies|not-applicable> compare-call=<library function>@<file>:<line>
  stored-name=<Type.field> decl=<file>:<line> extent=<constant> extent-value=<integer> extent-source=FIELD_TYPES
  copy-call=<library function>@<file>:<line>[,...] length-call=<library function>@<file>:<line>[,...]
  success-path=<creator callee>:<held|missing>
```

(one physical line; broken here for reading).

- `kind`, `value`, `kind-at` and the operations come from the data file; `applies` is the rule of section 4 (the facts
  hold the file of the operation's `def=` with the data's sha256 and define the function on that line).
- `extent-value` is the element count of the stored-name field's array type in FIELD_TYPES of the facts
  (`docs/static_facts.md` 5s); `extent` is the constant the data names.  Both are carried so that the judgement reads the
  bound of the name space, not the runtime's copy bound (judgement rules 8.11 item 13, 8.13 item 10).  Facts without
  FIELD_TYPES give `extent-value=-`.
- `compare-call`, `copy-call` and `length-call` are the P0 data-function and comparison calls the facts hold (CALLS) in
  the comparison function, on the line of the `stored-name` statement, and at the bounded NUL search of the key
  parameter in the lookup and the create (the search the `key-text` rule of section 3 already finds).  They are read from
  the facts, where macros are expanded; the data file does not name them.  They are what the candidate list of
  unverified entries is generated from (judgement rules 8.13 item 3).
- `success-path` repeats whether the row's `callee-check (success path)` line of the relations is held
  (`docs/static_facts.md` 5x).

One line per name-space file whose component applies:

```
namespace-layer data=<path> sha256=<hex> clear-table=<function>@<file>:<line>:<applies|not-applicable>
  clear-slot=<function>@<file>:<line>:<applies|not-applicable> rollback=<function>:<applies|not-applicable> status-param=<i> out-param=<j>
  delete-finalize=<function>:<applies|not-applicable> fill-call=<library function>@<file>:<line>
```

### 10.2 The kind value on the create side (`kind-value`)

Condition (라) needs the kind value of both calls in the record.  For every planned path A row with a `namespace-entry`
line: an at-call site of the layer call on the `at=` line of the entry's `lookup` operation and of each `create`
operation, in that operation's function, with the argument at the `kind-param` of the layer function's `layer-fn` line.
The lookup side is often planned already by the `key-text` rule (the same call); the create side is new.  A site that
does not bind (for example a call inlined away) is listed by the binding report with its reason, and the item is then
missing for the pairs of that row; nothing is assumed from the static partition alone.

### 10.3 Context frames (`context-frame`)

A use that the relations reach through more than one caller chain has one `alternatives [context …]:` line per chain
(`docs/static_facts.md` 5x).  For every planned row with more than one context: for each context, the at-call and the
after-call of **the lowest call site of its chain that lies on no other chain of that row**.  A use event that lies, in
its actor, between the at-call and the after-call of exactly one such site belongs to that context (judgement rules
8.13 item 16).  Sidecar: `context row=<key> id=<n> frame=<site id>|- chain="<call sites from the root to the use>"`.  A
context with no such call site: `need row <key> context-frame <n> not-planned reason=plan_missing(no call site tells the
context apart)`.  A row with one context needs no frame.

### 10.4 Lifetime ends: the site form of each, and how its event names the id

Per kind of a `namespace-entry` line, from the data's `delete` and `rollback` lines and the component's `layer-op` lines.
The sidecar's `lifetime` lines list every operation with `applies` or not and its sites, and close with
`lifetime-complete kind=<constant> yes|no(<what is not planned>)`.

| Lifetime end | Sites | How the event names the id |
|---|---|---|
| delete (`object-delete`, as revision 3) | the entry of each `delete` function with its parameters; the after-call (`ret`) of the `delete-finalize` call on the line of its `at=` | the id is the delete function's id parameter at its entry; the end is successful when `ret` of the finalize call in the same execution of the function is the success value |
| failed-create rollback (`object-rollback`) | in each `create` function of the kind: the at-call of the `create-finalize` call on the line of the `rollback` statement's `at=`, with the argument `status-arg`; its after-call with `out<out-arg>` | it names **no id**: no id was handed out.  The event belongs to the execution of the create function it lies in, and the name is the key text of that execution (the `key-text` site of the same function that precedes it in the same actor).  It is a rollback when the status argument is not the success value |
| table initialisation (`object-clear`) | the at-call of the fill call on the line of `layer-op clear-table`, once per component | it names **no id**: it ends every open generation of that table |
| slot clear (`object-clear`) | a store site (`place=store`) on the line of `layer-op clear-slot`, **only when** the relations' `lifetime end:` line of the name-space block does not show that the store runs only on a record whose id field holds the cleared value | no id; the event belongs to the execution of the create it lies in.  When the line shows the condition, the store is no lifetime end, no site is planned, and the `lifetime` line says `clear-slot=not-a-lifetime-end(<the relations line>)` |

- Pairing a callee-side event with the key text of "the same execution" by actor order is valid only when the create
  function is not re-entered by the same actor in between.  The `lifetime` line prints `reentry=none` when the create
  function reaches itself through no call of the facts (CALLS), else `reentry=possible`; with `possible` a rollback
  event has no name and counts against every name of its kind in the interval.
- A kind with no `rollback` or `delete` line in the data, or a component with no `layer-op` lines (the files of
  revision 1): `need row <key> object-delete|object-rollback|object-clear not-planned reason=plan_missing(no lifetime-end
  entry in the namespace data)`, and `lifetime-complete … no(…)`.
- The key-clearing calls of revision 4 (section 9) stay `object-delete` sites of the row.

### 10.5 Key text bound from the extent

Where the `namespace-entry` line has an `extent-value` smaller than the bound the `key-text` rule would use for the use's
or the creator's key (the bound of a tested NUL search, else `KZ_RT_STR_MAX`), the key text is recorded with
`bound=extent-value`; the sidecar `bound` line names the source `namespace-entry extent`.  The validation of the lookup
side's key text against the bound of the name space is done by the judgement from the recorded length and the
`extent-value`; the plan only makes sure the copy is not cut below it.

### 10.6 Needs and reasons

New need names: `kind-value`, `context-frame`, `object-rollback`, `object-clear`.  New sub-reasons, each under a reason
of the judgement rules' vocabulary and each added to `plan_reasons()` with the code: `plan_missing(no call site tells the
context apart)`, `plan_missing(no lifetime-end entry in the namespace data)`.  Not planned in revision 5, each named per
planned row: the branch taken (no site kind), an effect with no call, the lock calls of path A and B rows.

### 10.7 What the build that implements revision 5 measures

Per build of the five: the `namespace-entry` and `namespace-layer` lines; sites added per rule; contexts per planned row
and frames found; `lifetime-complete` per kind; the sites of the new rules that the binding report lists as unbound
(after a derived build).  Every count from a kept output of that step.

### 10.8 Precisions and one correction (2026-11-19, committed before the code that implements revision 5)

Written when the generator was changed, after reading the revision-2 data files (`semantics/framework_namespaces2_*.txt`)
and the relations lines of relations rules revision 14 that sections 10.1-10.4 read (`docs/static_facts.md` 5x).  Where
10.1-10.6 leave a choice open, the choice is this; the item marked *correction* changes the text above.

- **When the lines are written.**  The lines and sites of 10.1-10.5 are written when every `--namespaces` file is a file
  of coverage rule revision 2 (its `semantics-data 2` line).  A plan given files of both revisions is refused (a plan
  names one set).  With revision-1 files the output is that of revision 4 (above).  The `rules` line and the plan id are
  renamed by the step that renames the rule revisions; until then they keep the revision-4 text.
- **The reader.**  In a `semantics-data 2` file the reader also accepts `layer-fn`, `layer-op` and `kind-unresolved`
  statements and the `rollback` and `not-covered` sub-statements, and it reads `kind … value= at=`, `stored-name`,
  `extent` and both `compare` forms for the fields of 10.1; `kind-unresolved`, `not-covered`, `other-store` and
  `chain-status` are read and not used.  In a revision-1 file these statements are refused as before.  Where a field of
  a `namespace-entry` line has more than one value (two lookups, two creates, two calls), they are comma-separated.
- **10.1, an entry of the table-copy generation** (`table`): `kind=` is the table's name, `value=-`, `kind-at=-`;
  `create=` lists its `copy` and `create-check` functions; `compare=` is the function of its `compare` sub-statement;
  the stored name is the field declared at its `extent` sub-statement's `decl=`; `copy-call` is read on the line of
  the `copy` statement's `at=`.  **An entry of the object-id generation** (`kind`): `compare=` is the component's
  top-level `compare` statement; the stored names are those of its `stored-name` sub-statements (more than one:
  comma-separated in each field).
- **10.1 fields from the facts.**  `stored-name=<Type.field>` is the FIELD_TYPES row declared at the `decl=` position
  (file by its tail, line); none: `-`.  `extent-value` is that row's extent when FIELD_TYPES gives a constant one.
  `compare-call` are the calls of the P0 byte-string comparison family in the comparison function; `copy-call` the
  calls of P0 data functions on that line of the function; `length-call` the bounded NUL searches of section 3 (memchr
  with the byte 0 and a constant length, strnlen with a constant length) that the key parameter of the lookup and of
  each create reaches through PARAM_FLOW **at any depth** (the bound of section 3 reads depth 0 only and stays as it
  is).  Each call is written `<callee>@<file>:<line>`; none: `-`.  `success-path=` has one item per creator callee that
  the row's `callee-check (success path)` lines name, comma-separated; `-` when the row has none.
- **10.1, the component of a file applies** when a `layer-fn`, `layer-op` or entry operation of the file applies to
  the build (section 8).  A field of a `namespace-layer` line whose statement the file does not have is `-` (the files
  of the table-copy generation have no `layer-op` lines); two statements of one kind are comma-separated.
- **Placement in the sidecar.**  `namespace-layer` lines follow the `namespace-data` lines.  After the `lifetime` lines:
  the `lifetime-complete` lines, the `namespace-entry` lines, the `context` lines.
- **10.2.**  The layer call is a call on the `at=` line in the operation's function whose callee a `layer-fn` line with a
  `kind-param` names; its value has the role `kind` (from `arg<kind-param>`).  No such call in the facts:
  `need row <key> kind-value not-planned reason=record_insufficient(site position not in the facts)`.  An entry of the
  table-copy generation has no kind value: `need row <key> kind-value not-planned reason=plan_missing(no rule of this
  revision)`.
- **10.3, correction.**  The frame is two sites, the at-call and the after-call of one call; the sidecar writes
  `frame=<at-call site id>,<after-call site id>` (or `-`).  The id `<n>` counts the distinct chains of the row's
  `alternatives [context …]` lines in the order of the relations text (a chain written for two absence values is one
  context).  A context with a frame also gets `need row <key> context-frame <n> planned … sites=…`.
- **10.4.**  The rollback site's at-call is the call on the `rollback` statement's `at=` line whose callee the component's
  `create-finalize` `layer-fn` (or its `layer-op rollback`) names; its status value has the role `status`.  `reentry`
  is computed over the defined callees of CALLS.  The table initialisation's fill call is a P0 data-function call on the
  `layer-op clear-table` line.  The slot clear is matched with the relations' `lifetime end […]: clear-slot <position>`
  line by file tail and line.  The `lifetime` lines of revision 5 end with `sites=<ids>` (none: no field).
  The `layer-op` lines belong to the object-id generation: a `table` entry has none.  `lifetime-complete kind=<k> yes`
  when the kind has at least one `delete` and one `rollback` statement, its component has `layer-op clear-table` or
  `clear-slot` lines, and each of these applies and has its sites or is `not-a-lifetime-end`; else `no(<what is not,
  `; `-separated>)`.
- **10.4, needs.**  `object-rollback` and `object-clear` are written per planned row with a `namespace-entry` line as
  `object-delete` is (planned with the sites, or not planned with `plan_missing(no lifetime-end entry in the namespace
  data)`).  An `object-delete` need of a row whose matching entries have no applying `delete` statement takes the same
  sub-reason under revision 5 (revision 3 writes `no namespace data entry` there).
- **10.5.**  The sidecar line is `bound string-namespace-extent source=namespace-entry extent reason=… sites=<n>`, written
  when a key text was bounded so; the value's `ref` names the field and its extent.

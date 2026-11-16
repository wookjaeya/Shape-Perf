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

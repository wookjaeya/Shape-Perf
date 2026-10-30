// model.hpp -- the actor-program model of `kz predict` (format "kzp-predict 1"; SPEC 4.9, rev 1.3
// "Engine": explicit-state search on a small finite cone).
//
// A predict model states, for the actors of one bounded cone, the program each actor runs on the
// resources of the cone: its calls of the profile's operations, the branches its code takes on the
// returned codes, its loops and their bounds, and its exit.  Every step names the cited text it rests
// on (a provenance record of the Phi profile, or a `cite` of the model file, which records file, lines
// and the SHA-256 of the cited lines like a profile record).  Framework and case knowledge lives only
// in the model files (profiles/), never here.
//
// What a step does to the state comes from the profile, not from the model: a `use` returns the code of
// the operation's ready outcome when its conjunct's resource is in the stated state at the step, else the
// code of its not-ready outcome, and only where the outcome mappings' applicability conditions (`narrow`,
// P1) hold for the step's input; a `create` applies the operation's lifecycle effect; `clear`/`set` must
// name the linearization point of a profile effect with the same target and state.  A model whose step
// the profile does not support is refused (resolve_model).
//
// Grammar (line oriented; '#' starts a comment outside quotes; a quoted token may contain blanks):
//
//   kzp-predict 1
//   cite NAME file "PATH" lines A B sha256 "HEX" [note "TEXT"]
//   resource NAME kind KIND [key CONST]        a resource of the cone; CONST is a Str constant of the profile
//   actor NAME [start] [input] [record "NAME"] prov N, ...
//   program ACTOR
//     <instruction> [prov N, ...]
//     ...
//   end
//
// Instructions.  Visible ones are one transition of the search (an atomic step at the linearization point);
// local ones run inside the step of the actor and touch no shared state.
//   visible  clear SITE KIND STATE              every resource of KIND -> STATE (the profile effect at SITE)
//            set SITE RES STATE                  RES -> STATE (the profile effect at SITE)
//            spawn OP ACTOR                      the creation call of the profile's spawn rule; enables ACTOR
//            use OP RES role-site SITE           a call with the resource's key; result from the state
//            create OP RES effect LABEL role-site SITE taken CONST
//                                                ABSENT-like (not the effect's target): the effect, a new
//                                                generation and id, owner = the actor; else result CONST
//            delete-each OP KIND effect LABEL    one call per live resource of KIND, in creation order
//            exit OP CONST                       a terminal call with the constant as its first argument
//   guard    await RES STATE                     the next visible instruction is enabled only in that state
//   local    event SITE call|ret|pt [NAME=VALUE ...]   VALUE: integer, @text or an integer constant
//            mark "TEXT"                         a note of the code the model does not trace
//            label L | goto L
//            if-result CONST L | unless-result CONST L   on the actor's last use/create result
//            counter C reset | counter C inc | if-below C CONST L
//            finish                              the actor leaves the cone (continues outside it)
//   hit SITE before|after [caller MODULE] [name ROLE]
//            attached to the preceding visible instruction, finish or event (an event's hit is `after`: a site the
//            code reaches where the event is emitted): a site of the observation profile the
//            step reaches before (an entry) or after (an after-call point) its linearization point, with the
//            attributes a hold condition can test (kzp-hold 1: caller-in, name).  Hold plans are built from
//            these declarations only.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "kz/core/phi_ast.hpp"
#include "kz/core/status.hpp"

namespace kz::predict {

struct Cite {
  std::string name, file, sha256, note;
  uint32_t from = 0, to = 0;
  uint32_t line = 0;  // line in the model file
};

struct Resource {
  std::string name, kind, key_const;
  std::string key_text;     // resolved: the text of key_const
  int32_t kind_index = -1;  // resolved
  uint32_t line = 0;
};

enum class Ins : uint8_t {
  Event, Mark, Label, Goto, IfResult, UnlessResult, CounterReset, CounterInc, IfBelow, Finish,  // local
  Await,                                                                                       // guard
  Clear, Set, Spawn, Use, Create, DeleteEach, Exit                                             // visible
};
bool ins_visible(Ins i);
const char* ins_text(Ins i);

struct SiteHit {
  std::string site;
  bool before = true;
  std::string caller;     // module for caller-in; empty: not testable
  std::string name_role;  // slot role that carries the step's key text; empty: none
};

struct Instr {
  Ins op = Ins::Event;
  uint32_t line = 0;
  std::string site;    // Event, Clear, Set, role-site of Use/Create
  std::string phase;   // Event: call|ret|pt
  std::vector<std::pair<std::string, std::string>> values;  // Event: NAME=VALUE
  std::string text;    // Mark
  std::string label;   // Label, Goto, If*, IfBelow target
  std::string opname;  // Spawn, Use, Create, DeleteEach, Exit
  std::string res;     // Set, Use, Create, Await
  std::string kind;    // Clear, DeleteEach
  std::string state;   // Clear, Set, Await
  std::string actor;   // Spawn
  std::string effect;  // Create, DeleteEach
  std::string konst;   // If*, IfBelow, Create taken, Exit code
  std::string counter; // Counter*, IfBelow
  std::vector<SiteHit> hits;
  std::vector<std::string> prov;
  // resolved
  int32_t res_index = -1, kind_index = -1, state_index = -1, op_index = -1, actor_index = -1, target = -1, counter_index = -1;
  int64_t value = 0;           // the constant's value (If*, IfBelow bound, Create taken, Exit code)
  int32_t role_index = -1;     // Use, Create: the profile role whose site is `site`
  bool instance = false;       // Use: the role is a dependent use (an instance of the readiness specification)
  int64_t ready_code = 0, not_ready_code = 0;  // Use: the ready / not-ready outcome codes; Create, DeleteEach: ready
  std::string key_param, out_param, lp_point;  // Use: conjunct point; Create/DeleteEach: effect point
  std::string live_state;      // Use: the state the conjunct requires; Create: the effect's target state
  std::vector<std::pair<std::string, bool>> lp_values;  // the effect's lp values (name, is a text)
  std::string entry_op, entry_param;  // Spawn: the child's first event (the spawn rule's entry operation)
  std::string exit_param;             // Exit: the first parameter
};

struct Actor {
  std::string name, record;  // record: the actor's name in the record (default: name)
  bool start = false, input = false;
  std::vector<std::string> prov;
  std::vector<Instr> prog;
  std::map<std::string, int32_t> labels;  // resolved
  std::vector<std::string> counters;      // resolved
  bool has_instance_use = false;          // resolved
  uint32_t line = 0;
};

struct Model {
  std::vector<std::string> files;
  std::vector<Cite> cites;
  std::vector<Resource> resources;
  std::vector<Actor> actors;
  // resolved from the profile
  int32_t scope_kind = -1;            // the profile's scope kind (the shutdown) or -1
  std::string scope_request, scope_teardown;
  std::map<std::string, int64_t> consts;  // the integer constants the model used (name -> value), for the report
  bool resolved = false;

  int32_t find_actor(const std::string& n) const;
  int32_t find_resource(const std::string& n) const;
  int32_t find_cite(const std::string& n) const;
};

// Appends the declarations of one file.  Errors are "file:line: message".
Status parse_model(const std::string& name, const std::string& text, Model* m, std::vector<std::string>* errors);

// Resolves every name against the checked profile and the given values of `ref eval` constants (the replay's
// --const values).  Refuses steps the profile does not support (see the file comment) and a `use` whose outcome
// mappings do not apply to its key (P1 conditional: the key is a text shorter than the bound constant).
Status resolve_model(const phi::Profile& p, const std::map<std::string, int64_t>& ref_consts, Model* m, std::vector<std::string>* errors);

// Re-reads every `cite` of the model under `root` (the pinned bundle) and compares the SHA-256 of the cited lines.
Status verify_cites(const Model& m, const std::string& root, std::vector<std::string>* problems);

// "file LA-B" of a provenance name, from the profile or the model's cites ("?" if unknown).
std::string cite_text(const phi::Profile& p, const Model& m, const std::string& prov);

}  // namespace kz::predict

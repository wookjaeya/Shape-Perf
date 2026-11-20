// model.cpp -- parsing and resolution of predict models (kz/predict/model.hpp).
#include "kz/predict/model.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/phi_provenance.hpp"

namespace kz::predict {

bool ins_visible(Ins i) { return i >= Ins::Clear; }

const char* ins_text(Ins i) {
  switch (i) {
    case Ins::Event: return "event";
    case Ins::Mark: return "mark";
    case Ins::Label: return "label";
    case Ins::Goto: return "goto";
    case Ins::IfResult: return "if-result";
    case Ins::UnlessResult: return "unless-result";
    case Ins::CounterReset: return "counter-reset";
    case Ins::CounterInc: return "counter-inc";
    case Ins::IfBelow: return "if-below";
    case Ins::Finish: return "finish";
    case Ins::Await: return "await";
    case Ins::Clear: return "clear";
    case Ins::Set: return "set";
    case Ins::Spawn: return "spawn";
    case Ins::Use: return "use";
    case Ins::Create: return "create";
    case Ins::DeleteEach: return "delete-each";
    case Ins::Exit: return "exit";
  }
  return "?";
}

int32_t Model::find_actor(const std::string& n) const {
  for (size_t i = 0; i < actors.size(); ++i)
    if (actors[i].name == n) return static_cast<int32_t>(i);
  return -1;
}
int32_t Model::find_resource(const std::string& n) const {
  for (size_t i = 0; i < resources.size(); ++i)
    if (resources[i].name == n) return static_cast<int32_t>(i);
  return -1;
}
int32_t Model::find_cite(const std::string& n) const {
  for (size_t i = 0; i < cites.size(); ++i)
    if (cites[i].name == n) return static_cast<int32_t>(i);
  return -1;
}

namespace {

// Splits a line into tokens: blanks separate, double quotes group (the quotes are kept as the first character
// so that a quoted token is told apart), '#' outside quotes ends the line.  Commas are separators too.
bool tokenize(const std::string& line, std::vector<std::string>* out, std::string* err) {
  std::string cur;
  bool in_tok = false;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '"') {
      std::string q = "\"";
      size_t j = i + 1;
      for (; j < line.size() && line[j] != '"'; ++j) q += line[j];
      if (j >= line.size()) {
        *err = "unterminated quote";
        return false;
      }
      if (in_tok) {
        out->push_back(cur);
        cur.clear();
        in_tok = false;
      }
      out->push_back(q);
      i = j;
      continue;
    }
    if (c == '#') break;
    if (c == ' ' || c == '\t' || c == ',' || c == '\r') {
      if (in_tok) {
        out->push_back(cur);
        cur.clear();
        in_tok = false;
      }
      continue;
    }
    cur += c;
    in_tok = true;
  }
  if (in_tok) out->push_back(cur);
  return true;
}

bool quoted(const std::string& t) { return !t.empty() && t[0] == '"'; }
std::string unq(const std::string& t) { return quoted(t) ? t.substr(1) : t; }

bool to_u32(const std::string& s, uint32_t* v) {
  char* end = nullptr;
  errno = 0;
  const unsigned long x = std::strtoul(s.c_str(), &end, 10);
  if (s.empty() || end == nullptr || *end != '\0' || errno != 0 || x > 0xffffffffUL) return false;
  *v = static_cast<uint32_t>(x);
  return true;
}

// Takes a trailing "prov N, ..." off the tokens.
std::vector<std::string> take_prov(std::vector<std::string>* toks) {
  std::vector<std::string> prov;
  for (size_t i = 0; i < toks->size(); ++i)
    if ((*toks)[i] == "prov") {
      prov.assign(toks->begin() + static_cast<long>(i) + 1, toks->end());
      toks->resize(i);
      break;
    }
  return prov;
}

}  // namespace

Status parse_model(const std::string& name, const std::string& text, Model* m, std::vector<std::string>* errors) {
  m->files.push_back(name);
  size_t pos = 0;
  uint32_t lineno = 0;
  bool header = false;
  int32_t cur_actor = -1;
  const size_t errors_before = errors->size();
  auto err = [&](const std::string& msg) { errors->push_back(name + ":" + std::to_string(lineno) + ": " + msg); };
  while (pos <= text.size()) {
    size_t nl = text.find('\n', pos);
    if (nl == std::string::npos) nl = text.size();
    const std::string line = text.substr(pos, nl - pos);
    pos = nl + 1;
    ++lineno;
    std::vector<std::string> t;
    std::string terr;
    if (!tokenize(line, &t, &terr)) {
      err(terr);
      continue;
    }
    if (t.empty()) {
      if (nl >= text.size()) break;
      continue;
    }
    if (!header) {
      if (t.size() != 2 || t[0] != "kzp-predict" || t[1] != "1") {
        err("the first statement must be 'kzp-predict 1'");
        return Status::error(Err::Invalid, "not a kzp-predict 1 model");
      }
      header = true;
      continue;
    }
    const std::string& k = t[0];
    if (cur_actor < 0) {
      if (k == "cite") {
        Cite c;
        c.line = lineno;
        if (t.size() < 2) {
          err("cite: a name is needed");
          continue;
        }
        c.name = t[1];
        for (size_t i = 2; i < t.size(); ++i) {
          if (t[i] == "file" && i + 1 < t.size()) {
            c.file = unq(t[++i]);
          } else if (t[i] == "lines" && i + 2 < t.size()) {
            if (!to_u32(t[i + 1], &c.from) || !to_u32(t[i + 2], &c.to) || c.from == 0 || c.to < c.from) err("cite " + c.name + ": bad line range");
            i += 2;
          } else if (t[i] == "sha256" && i + 1 < t.size()) {
            c.sha256 = unq(t[++i]);
          } else if (t[i] == "note" && i + 1 < t.size()) {
            c.note = unq(t[++i]);
          } else {
            err("cite " + c.name + ": unexpected '" + unq(t[i]) + "'");
          }
        }
        if (c.file.empty() || c.from == 0 || c.sha256.size() != 64) err("cite " + c.name + ": file, lines and a 64-digit sha256 are required");
        if (m->find_cite(c.name) >= 0) err("cite " + c.name + ": defined twice");
        m->cites.push_back(c);
      } else if (k == "resource") {
        Resource r;
        r.line = lineno;
        if (t.size() < 4 || t[2] != "kind") {
          err("resource NAME kind KIND [key CONST]");
          continue;
        }
        r.name = t[1];
        r.kind = t[3];
        if (t.size() == 6 && t[4] == "key") {
          r.key_const = t[5];
        } else if (t.size() != 4) {
          err("resource NAME kind KIND [key CONST]");
        }
        if (m->find_resource(r.name) >= 0) err("resource " + r.name + ": defined twice");
        m->resources.push_back(r);
      } else if (k == "actor") {
        Actor a;
        a.line = lineno;
        a.prov = take_prov(&t);
        if (t.size() < 2) {
          err("actor NAME ...");
          continue;
        }
        a.name = t[1];
        a.record = a.name;
        for (size_t i = 2; i < t.size(); ++i) {
          if (t[i] == "start") {
            a.start = true;
          } else if (t[i] == "input") {
            a.input = true;
          } else if (t[i] == "record" && i + 1 < t.size()) {
            a.record = unq(t[++i]);
          } else {
            err("actor " + a.name + ": unexpected '" + unq(t[i]) + "'");
          }
        }
        if (m->find_actor(a.name) >= 0) err("actor " + a.name + ": defined twice");
        m->actors.push_back(a);
      } else if (k == "program") {
        if (t.size() != 2 || m->find_actor(t[1]) < 0) {
          err("program ACTOR: the actor must be declared before");
          continue;
        }
        cur_actor = m->find_actor(t[1]);
        if (!m->actors[static_cast<size_t>(cur_actor)].prog.empty()) err("program " + t[1] + ": given twice");
      } else {
        err("unknown statement '" + k + "'");
      }
      continue;
    }
    // inside a program
    Actor& a = m->actors[static_cast<size_t>(cur_actor)];
    if (k == "end" && t.size() == 1) {
      cur_actor = -1;
      continue;
    }
    if (k == "hit") {
      if (a.prog.empty() || !(ins_visible(a.prog.back().op) || a.prog.back().op == Ins::Finish || a.prog.back().op == Ins::Event)) {
        err("hit: must follow a visible instruction, an event or finish");
        continue;
      }
      SiteHit h;
      if (t.size() < 3 || (t[2] != "before" && t[2] != "after")) {
        err("hit SITE before|after [caller MODULE] [name ROLE]");
        continue;
      }
      h.site = t[1];
      h.before = t[2] == "before";
      if (a.prog.back().op == Ins::Event && h.before) err("hit: the hit of an event is reached after it (after)");
      for (size_t i = 3; i + 1 < t.size(); i += 2) {
        if (t[i] == "caller") {
          h.caller = t[i + 1];
        } else if (t[i] == "name") {
          h.name_role = t[i + 1];
        } else {
          err("hit: unexpected '" + t[i] + "'");
        }
      }
      if ((t.size() - 3) % 2 != 0) err("hit: an attribute without a value");
      a.prog.back().hits.push_back(h);
      continue;
    }
    Instr in;
    in.line = lineno;
    in.prov = take_prov(&t);
    const size_t n = t.size();
    auto need = [&](size_t want, const char* form) {
      if (n != want) {
        err(std::string("expected: ") + form);
        return false;
      }
      return true;
    };
    if (k == "event") {
      if (n < 3) {
        err("expected: event SITE call|ret|pt [NAME=VALUE ...]");
        continue;
      }
      in.op = Ins::Event;
      in.site = t[1];
      in.phase = t[2];
      if (in.phase != "call" && in.phase != "ret" && in.phase != "pt") err("event: phase must be call, ret or pt");
      for (size_t i = 3; i < n; ++i) {
        const size_t eq = t[i].find('=');
        if (eq == std::string::npos || eq == 0) {
          err("event: expected NAME=VALUE, got '" + t[i] + "'");
          continue;
        }
        in.values.push_back({t[i].substr(0, eq), t[i].substr(eq + 1)});
      }
    } else if (k == "mark") {
      if (!need(2, "mark \"TEXT\"")) continue;
      in.op = Ins::Mark;
      in.text = unq(t[1]);
    } else if (k == "label") {
      if (!need(2, "label L")) continue;
      in.op = Ins::Label;
      in.label = t[1];
    } else if (k == "goto") {
      if (!need(2, "goto L")) continue;
      in.op = Ins::Goto;
      in.label = t[1];
    } else if (k == "if-result" || k == "unless-result") {
      if (!need(3, "if-result|unless-result CONST L")) continue;
      in.op = k == "if-result" ? Ins::IfResult : Ins::UnlessResult;
      in.konst = t[1];
      in.label = t[2];
    } else if (k == "counter") {
      if (!need(3, "counter C reset|inc")) continue;
      in.counter = t[1];
      if (t[2] == "reset") {
        in.op = Ins::CounterReset;
      } else if (t[2] == "inc") {
        in.op = Ins::CounterInc;
      } else {
        err("counter C reset|inc");
        continue;
      }
    } else if (k == "if-below") {
      if (!need(4, "if-below C CONST L")) continue;
      in.op = Ins::IfBelow;
      in.counter = t[1];
      in.konst = t[2];
      in.label = t[3];
    } else if (k == "finish") {
      if (!need(1, "finish")) continue;
      in.op = Ins::Finish;
    } else if (k == "await") {
      if (!need(3, "await RES STATE")) continue;
      in.op = Ins::Await;
      in.res = t[1];
      in.state = t[2];
    } else if (k == "clear") {
      if (!need(4, "clear SITE KIND STATE")) continue;
      in.op = Ins::Clear;
      in.site = t[1];
      in.kind = t[2];
      in.state = t[3];
    } else if (k == "set") {
      if (!need(4, "set SITE RES STATE")) continue;
      in.op = Ins::Set;
      in.site = t[1];
      in.res = t[2];
      in.state = t[3];
    } else if (k == "spawn") {
      if (!need(3, "spawn OP ACTOR")) continue;
      in.op = Ins::Spawn;
      in.opname = t[1];
      in.actor = t[2];
    } else if (k == "use") {
      if (!need(5, "use OP RES role-site SITE") || t[3] != "role-site") continue;
      in.op = Ins::Use;
      in.opname = t[1];
      in.res = t[2];
      in.site = t[4];
    } else if (k == "create") {
      if (!need(9, "create OP RES effect LABEL role-site SITE taken CONST") || t[3] != "effect" || t[5] != "role-site" || t[7] != "taken") continue;
      in.op = Ins::Create;
      in.opname = t[1];
      in.res = t[2];
      in.effect = t[4];
      in.site = t[6];
      in.konst = t[8];
    } else if (k == "delete-each") {
      if (!need(7, "delete-each OP KIND effect LABEL status CONST") || t[3] != "effect" || t[5] != "status") continue;
      in.op = Ins::DeleteEach;
      in.opname = t[1];
      in.kind = t[2];
      in.effect = t[4];
      in.konst = t[6];
    } else if (k == "exit") {
      if (!need(3, "exit OP CONST")) continue;
      in.op = Ins::Exit;
      in.opname = t[1];
      in.konst = t[2];
    } else {
      err("unknown instruction '" + k + "'");
      continue;
    }
    a.prog.push_back(in);
  }
  if (!header) {
    errors->push_back(name + ": empty model (no 'kzp-predict 1')");
  } else if (cur_actor >= 0) {
    errors->push_back(name + ": program " + m->actors[static_cast<size_t>(cur_actor)].name + " has no 'end'");
  }
  return errors->size() == errors_before ? Status::success() : Status::error(Err::Invalid, "the model has errors");
}

namespace {

struct Resolver {
  const phi::Profile& p;
  const std::map<std::string, int64_t>& refc;
  Model* m;
  std::vector<std::string>* errors;
  std::string where;

  void err(const std::string& msg) { errors->push_back(where + ": " + msg); }

  bool int_const(const std::string& name, int64_t* v) {
    const int32_t ci = p.find_const(name);
    if (ci < 0) {
      err("no constant " + name + " in the profile");
      return false;
    }
    const phi::ConstDecl& c = p.consts[static_cast<size_t>(ci)];
    if (c.value == phi::ConstDecl::Value::Int) {
      *v = c.ival;
    } else if (c.value == phi::ConstDecl::Value::RefEval) {
      auto it = refc.find(name);
      if (it == refc.end()) {
        err("the `ref eval` constant " + name + " has no value: give it (--const " + name + "=N, from its reference)");
        return false;
      }
      *v = it->second;
    } else {
      err("constant " + name + " is not an integer");
      return false;
    }
    m->consts[name] = *v;
    return true;
  }

  bool provs(const Instr& in, bool required) {
    if (required && in.prov.empty()) {
      err(std::string(ins_text(in.op)) + ": a step of the code needs its citation (prov)");
      return false;
    }
    for (const std::string& n : in.prov)
      if (p.find_prov(n) < 0 && m->find_cite(n) < 0) {
        err("prov " + n + ": neither a provenance record of the profile nor a cite of the model");
        return false;
      }
    return true;
  }

  int32_t op(const std::string& n) {
    const int32_t i = p.find_op(n);
    if (i < 0) err("no operation " + n + " in the profile");
    return i;
  }

  int64_t ready_code(const phi::OpDecl& o, bool* ok) {
    for (const phi::OutcomeDecl& oc : o.outcomes)
      if (oc.cls == phi::OutcomeClass::Ready) {
        int64_t v = 0;
        *ok = int_const(oc.code, &v);
        return v;
      }
    *ok = false;
    err("operation " + o.name + " has no ready outcome");
    return 0;
  }

  // "K(arg.P)" or "K(lp.P)" or "K()" -> (K, P)
  static bool res_parts(const std::string& text, std::string* kind, std::string* param) {
    const size_t lp = text.find('(');
    if (lp == std::string::npos || text.back() != ')') return false;
    *kind = text.substr(0, lp);
    std::string inner = text.substr(lp + 1, text.size() - lp - 2);
    const size_t dot = inner.find('.');
    *param = dot == std::string::npos ? inner : inner.substr(dot + 1);
    return true;
  }

  // The profile effect at `site` that sets `kind` (every instance when all) to `state`.
  bool effect_at(const std::string& site, const std::string& kind, const std::string& state, bool all) {
    for (const phi::OpDecl& o : p.ops)
      for (const phi::EffectDecl& e : o.effects) {
        if (e.kind != phi::EffectKind::Set || eval::lp_name(o, e.label, e.lp) != site || e.to_state != state) continue;
        if (all && e.all_kind == kind) return true;
        if (!all && e.res) {
          std::string k, prm;
          if (res_parts(phi::expr_text(*e.res), &k, &prm) && k == kind) return true;
        }
      }
    return false;
  }

  void instr(Actor& a, Instr& in) {
    where = "line " + std::to_string(in.line) + " (" + a.name + ")";
    switch (in.op) {
      case Ins::Event: {
        for (auto& kv : in.values) {
          const std::string& v = kv.second;
          if (v.empty()) err("event: empty value");
          else if (v[0] == '@') continue;
          else if (v[0] == '-' || (v[0] >= '0' && v[0] <= '9')) continue;
          else {
            int64_t x = 0;
            if (!int_const(v, &x)) return;
          }
        }
        provs(in, false);
        return;
      }
      case Ins::Mark:
      case Ins::Label:
      case Ins::Finish:
        provs(in, false);
        return;
      case Ins::Goto:
        provs(in, true);
        return;
      case Ins::IfResult:
      case Ins::UnlessResult:
        provs(in, true);
        int_const(in.konst, &in.value);
        return;
      case Ins::CounterReset:
      case Ins::CounterInc:
        provs(in, false);
        return;
      case Ins::IfBelow:
        provs(in, true);
        int_const(in.konst, &in.value);
        return;
      case Ins::Await: {
        provs(in, true);
        in.res_index = m->find_resource(in.res);
        if (in.res_index < 0) {
          err("await: no resource " + in.res);
          return;
        }
        const phi::KindDecl& kd = p.kinds[static_cast<size_t>(m->resources[static_cast<size_t>(in.res_index)].kind_index)];
        if (std::find(kd.states.begin(), kd.states.end(), in.state) == kd.states.end()) err("await: " + in.state + " is not a state of " + kd.name);
        return;
      }
      case Ins::Clear:
        provs(in, true);
        in.kind_index = p.find_kind(in.kind);
        if (in.kind_index < 0) err("clear: no kind " + in.kind);
        else if (!effect_at(in.site, in.kind, in.state, true)) err("clear: no effect of the profile at " + in.site + " sets every " + in.kind + " to " + in.state);
        return;
      case Ins::Set: {
        provs(in, true);
        in.res_index = m->find_resource(in.res);
        if (in.res_index < 0) {
          err("set: no resource " + in.res);
          return;
        }
        const Resource& r = m->resources[static_cast<size_t>(in.res_index)];
        if (!effect_at(in.site, r.kind, in.state, false)) err("set: no effect of the profile at " + in.site + " sets " + r.kind + " to " + in.state);
        return;
      }
      case Ins::Spawn: {
        provs(in, true);
        in.actor_index = m->find_actor(in.actor);
        if (in.actor_index < 0 || m->actors[static_cast<size_t>(in.actor_index)].start) {
          err("spawn: " + in.actor + " is not a declared actor that is spawned");
          return;
        }
        in.op_index = op(in.opname);
        for (const phi::HbRuleDecl& r : p.rules)
          if (r.tmpl == phi::RuleTemplate::Spawn && r.create_op == in.opname) {
            in.entry_op = r.entry_op;
            in.out_param = r.create_out;
          }
        if (in.entry_op.empty()) {
          err("spawn: no spawn rule of the profile creates with " + in.opname);
          return;
        }
        const int32_t eo = op(in.entry_op);
        if (eo >= 0 && !p.ops[static_cast<size_t>(eo)].params.empty()) in.entry_param = p.ops[static_cast<size_t>(eo)].params[0].name;
        return;
      }
      case Ins::Use: {
        provs(in, true);
        in.res_index = m->find_resource(in.res);
        in.op_index = op(in.opname);
        if (in.res_index < 0 || in.op_index < 0) {
          if (in.res_index < 0) err("use: no resource " + in.res);
          return;
        }
        const Resource& r = m->resources[static_cast<size_t>(in.res_index)];
        const phi::OpDecl& o = p.ops[static_cast<size_t>(in.op_index)];
        if (o.conjuncts.size() != 1) {
          err("use: " + o.name + " must have exactly one conjunct");
          return;
        }
        const phi::ConjunctDecl& cj = o.conjuncts[0];
        // state(K(arg.P)) = S
        const std::string et = cj.expr ? phi::expr_text(*cj.expr) : std::string();
        const std::string pre = "state(" + r.kind + "(arg.";
        const size_t close = et.find("))");
        if (et.compare(0, pre.size(), pre) != 0 || close == std::string::npos || et.compare(close, 5, ")) = ") != 0) {
          err("use: the conjunct of " + o.name + " is not of the form state(" + r.kind + "(arg.P)) = S: " + et);
          return;
        }
        in.key_param = et.substr(pre.size(), close - pre.size());
        in.live_state = et.substr(close + 5);
        in.lp_point = eval::lp_name(o, cj.label, cj.lp);
        for (const phi::OpParam& prm : o.params)
          if (prm.role == "out" && in.out_param.empty()) in.out_param = prm.name;
        // the role of the call context
        for (size_t ri = 0; ri < p.roles.size(); ++ri)
          if (p.roles[ri].site == in.site && p.roles[ri].op == o.name) {
            in.role_index = static_cast<int32_t>(ri);
            in.instance = p.roles[ri].purpose == "dependent_use";
          }
        if (in.role_index < 0) {
          err("use: " + in.site + " is not the site of a role of " + o.name + " (the call context must be stated)");
          return;
        }
        // the two outcome mappings and their applicability conditions (P1 conditional)
        bool have_ready = false, have_nr = false;
        for (const phi::OutcomeDecl& oc : o.outcomes) {
          const bool is_ready = oc.cls == phi::OutcomeClass::Ready;
          const bool is_nr = oc.cls == phi::OutcomeClass::NotReady && oc.conjunct == cj.label;
          if (!is_ready && !is_nr) continue;
          int64_t code = 0;
          if (!int_const(oc.code, &code)) return;
          if (!oc.narrow.present) {
            err("use: the outcome " + oc.code + " of " + o.name + " has no applicability conditions (narrow): the code cannot be computed from the state");
            return;
          }
          bool ctx = oc.narrow.context_any;
          for (const std::string& c : oc.narrow.contexts) ctx = ctx || c == p.roles[static_cast<size_t>(in.role_index)].name;
          if (!ctx) {
            err("use: the mapping of " + oc.code + " does not apply in the call context " + p.roles[static_cast<size_t>(in.role_index)].name);
            return;
          }
          for (const phi::NarrowCaptured& nc : oc.narrow.captured) {
            if (nc.param != in.key_param || !nc.text) {
              err("use: the mapping of " + oc.code + " needs the argument " + nc.param + ", which the model does not give as a text");
              return;
            }
            if (!nc.length_below.empty()) {
              int64_t bound = 0;
              if (!int_const(nc.length_below, &bound)) return;
              if (static_cast<int64_t>(r.key_text.size()) >= bound) {
                err("use: the key \"" + r.key_text + "\" is not shorter than " + nc.length_below + " = " + std::to_string(bound) + ": the mapping of " + oc.code + " does not apply");
                return;
              }
            }
          }
          if (is_ready) {
            in.ready_code = code;
            have_ready = true;
          } else {
            in.not_ready_code = code;
            have_nr = true;
          }
        }
        if (!have_ready || !have_nr) err("use: " + o.name + " needs a ready and a not-ready outcome of its conjunct");
        return;
      }
      case Ins::Create: {
        provs(in, true);
        in.res_index = m->find_resource(in.res);
        in.op_index = op(in.opname);
        if (in.res_index < 0 || in.op_index < 0) {
          if (in.res_index < 0) err("create: no resource " + in.res);
          return;
        }
        const Resource& r = m->resources[static_cast<size_t>(in.res_index)];
        const phi::OpDecl& o = p.ops[static_cast<size_t>(in.op_index)];
        const phi::EffectDecl* eff = nullptr;
        for (const phi::EffectDecl& e : o.effects)
          if (e.label == in.effect) eff = &e;
        if (eff == nullptr || eff->kind != phi::EffectKind::Set || !eff->res) {
          err("create: " + o.name + " has no lifecycle effect " + in.effect + " on one resource");
          return;
        }
        std::string k, prm;
        if (!res_parts(phi::expr_text(*eff->res), &k, &prm) || k != r.kind) {
          err("create: the effect " + in.effect + " is not on " + r.kind);
          return;
        }
        in.key_param = prm;
        in.live_state = eff->to_state;
        in.lp_point = eval::lp_name(o, eff->label, eff->lp);
        for (const phi::OpParam& v : eff->lp.values) in.lp_values.push_back({v.name, v.sort.kind == phi::SortKind::Str});
        for (const phi::OpParam& pp : o.params)
          if (pp.role == "out" && in.out_param.empty()) in.out_param = pp.name;
        bool ok = false;
        in.ready_code = ready_code(o, &ok);
        bool documented = false;
        for (const phi::OutcomeDecl& oc : o.outcomes) documented = documented || oc.code == in.konst;
        if (!documented) err("create: " + in.konst + " is not an outcome of " + o.name);
        int_const(in.konst, &in.value);
        if (p.find_site(in.site) < 0) err("create: no site " + in.site + " in the profile");
        return;
      }
      case Ins::DeleteEach: {
        provs(in, true);
        in.kind_index = p.find_kind(in.kind);
        in.op_index = op(in.opname);
        if (in.kind_index < 0 || in.op_index < 0) {
          if (in.kind_index < 0) err("delete-each: no kind " + in.kind);
          return;
        }
        const phi::OpDecl& o = p.ops[static_cast<size_t>(in.op_index)];
        const phi::EffectDecl* eff = nullptr;
        for (const phi::EffectDecl& e : o.effects)
          if (e.label == in.effect) eff = &e;
        std::string k, prm;
        if (eff == nullptr || eff->kind != phi::EffectKind::Set || !eff->res || !res_parts(phi::expr_text(*eff->res), &k, &prm) || k != in.kind) {
          err("delete-each: " + o.name + " has no lifecycle effect " + in.effect + " on " + in.kind);
          return;
        }
        in.live_state = eff->to_state;
        in.lp_point = eval::lp_name(o, eff->label, eff->lp);
        for (const phi::OpParam& v : eff->lp.values) in.lp_values.push_back({v.name, v.sort.kind == phi::SortKind::Str});
        if (!o.params.empty()) in.key_param = o.params[0].name;
        int_const(in.konst, &in.ready_code);
        return;
      }
      case Ins::Exit: {
        provs(in, true);
        in.op_index = op(in.opname);
        if (in.op_index < 0) return;
        const phi::OpDecl& o = p.ops[static_cast<size_t>(in.op_index)];
        if (!o.terminal.present) err("exit: " + o.name + " is not a terminal operation of the profile");
        if (!o.params.empty()) in.exit_param = o.params[0].name;
        int_const(in.konst, &in.value);
        return;
      }
    }
  }
};

}  // namespace

Status resolve_model(const phi::Profile& p, const std::map<std::string, int64_t>& ref_consts, Model* m, std::vector<std::string>* errors) {
  const size_t before = errors->size();
  if (!p.checked) return Status::error(Err::Invalid, "the profile is not checked");
  Resolver rv{p, ref_consts, m, errors, "model"};
  for (Resource& r : m->resources) {
    rv.where = "resource " + r.name;
    r.kind_index = p.find_kind(r.kind);
    if (r.kind_index < 0) {
      rv.err("no kind " + r.kind + " in the profile");
      continue;
    }
    const phi::KindDecl& kd = p.kinds[static_cast<size_t>(r.kind_index)];
    if (r.key_const.empty()) {
      if (!kd.params.empty()) rv.err("the kind " + r.kind + " has a key: give `key CONST`");
      continue;
    }
    if (kd.params.size() != 1 || kd.params[0].sort.kind != phi::SortKind::Str) {
      rv.err("the kind " + r.kind + " is not keyed by one string");
      continue;
    }
    const int32_t ci = p.find_const(r.key_const);
    if (ci < 0 || p.consts[static_cast<size_t>(ci)].value != phi::ConstDecl::Value::Str) {
      rv.err("no string constant " + r.key_const + " in the profile");
      continue;
    }
    r.key_text = p.consts[static_cast<size_t>(ci)].sval;
  }
  for (Actor& a : m->actors) {
    rv.where = "actor " + a.name;
    for (const std::string& n : a.prov)
      if (p.find_prov(n) < 0 && m->find_cite(n) < 0) rv.err("prov " + n + ": unknown");
    if (a.prog.empty()) rv.err("no program");
    for (size_t i = 0; i < a.prog.size(); ++i) {
      Instr& in = a.prog[i];
      if (in.op == Ins::Label) {
        if (a.labels.count(in.label) != 0) rv.err("label " + in.label + " twice");
        a.labels[in.label] = static_cast<int32_t>(i);
      }
      if (in.op == Ins::CounterReset || in.op == Ins::CounterInc || in.op == Ins::IfBelow)
        if (std::find(a.counters.begin(), a.counters.end(), in.counter) == a.counters.end()) a.counters.push_back(in.counter);
    }
    for (Instr& in : a.prog) {
      rv.instr(a, in);
      if (in.op == Ins::Goto || in.op == Ins::IfResult || in.op == Ins::UnlessResult || in.op == Ins::IfBelow) {
        auto it = a.labels.find(in.label);
        if (it == a.labels.end()) rv.err("no label " + in.label);
        else in.target = it->second;
      }
      if (in.op == Ins::CounterReset || in.op == Ins::CounterInc || in.op == Ins::IfBelow)
        in.counter_index = static_cast<int32_t>(std::find(a.counters.begin(), a.counters.end(), in.counter) - a.counters.begin());
      if (in.op == Ins::Use && in.instance) a.has_instance_use = true;
    }
    const Ins last = a.prog.empty() ? Ins::Finish : a.prog.back().op;
    if (last != Ins::Finish && last != Ins::Exit && last != Ins::Goto) rv.err("the program must end with finish, exit or goto");
  }
  if (p.scope.present && p.scope.kind_index >= 0) {
    m->scope_kind = p.scope.kind_index;
    m->scope_request = p.scope.request;
    m->scope_teardown = p.scope.teardown;
  }
  m->resolved = errors->size() == before;
  return m->resolved ? Status::success() : Status::error(Err::Invalid, "the model does not resolve against the profile");
}

Status verify_cites(const Model& m, const std::string& root, std::vector<std::string>* problems) {
  for (const Cite& c : m.cites) {
    Bytes b;
    const Status s = read_whole_file(root + "/" + c.file, &b);
    if (!s.ok()) {
      problems->push_back("cite " + c.name + ": " + s.describe());
      continue;
    }
    Digest d{};
    if (!phi::cited_text_hash(std::string(b.begin(), b.end()), c.from, c.to, &d)) {
      problems->push_back("cite " + c.name + ": lines " + std::to_string(c.from) + "-" + std::to_string(c.to) + " are not in " + c.file);
      continue;
    }
    if (to_hex(d) != c.sha256) problems->push_back("cite " + c.name + ": the text of " + c.file + " L" + std::to_string(c.from) + "-" + std::to_string(c.to) + " changed (sha256 " + to_hex(d) + ")");
  }
  return Status::success();
}

std::string cite_text(const phi::Profile& p, const Model& m, const std::string& prov) {
  const int32_t pi = p.find_prov(prov);
  if (pi >= 0) {
    const phi::ProvDecl& d = p.provs[static_cast<size_t>(pi)];
    if (d.file.empty()) return prov + " (" + d.tier + ")";
    return prov + " = " + d.file + " L" + std::to_string(d.line_from) + (d.line_to != d.line_from ? "-" + std::to_string(d.line_to) : std::string());
  }
  const int32_t ci = m.find_cite(prov);
  if (ci >= 0) {
    const Cite& c = m.cites[static_cast<size_t>(ci)];
    return prov + " = " + c.file + " L" + std::to_string(c.from) + (c.to != c.from ? "-" + std::to_string(c.to) : std::string());
  }
  return prov + " = ?";
}

}  // namespace kz::predict

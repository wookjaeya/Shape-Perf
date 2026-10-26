// kz_rt_manifest.cpp -- see kz_rt_manifest.hpp ("kzs-sites 1" and "kzs-sites 2").
#include "kz_rt_manifest.hpp"

#include <cerrno>
#include <cstdlib>
#include <set>

#include "kz_rt.h"

namespace kzrt {

namespace {

// Splits one line into tokens: white space separates, '"' toggles quoting (quotes removed), '#' outside
// quotes ends the line.  Returns false on an unterminated quote.
bool tokenize(std::string_view line, std::vector<std::string>* out) {
  out->clear();
  std::string cur;
  bool in_tok = false, quoted = false;
  for (char ch : line) {
    if (quoted) {
      if (ch == '"')
        quoted = false;
      else
        cur.push_back(ch);
      continue;
    }
    if (ch == '#') break;
    if (ch == '"') {
      quoted = true;
      in_tok = true;
      continue;
    }
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      if (in_tok) out->push_back(cur), cur.clear(), in_tok = false;
      continue;
    }
    cur.push_back(ch);
    in_tok = true;
  }
  if (quoted) return false;
  if (in_tok) out->push_back(cur);
  return true;
}

bool parse_u32(const std::string& s, uint32_t lo, uint32_t hi, uint32_t* v) {
  if (s.empty() || s.size() > 10) return false;
  uint64_t x = 0;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
    x = x * 10u + static_cast<uint64_t>(c - '0');
  }
  if (x < lo || x > hi) return false;
  *v = static_cast<uint32_t>(x);
  return true;
}

bool parse_mask(const std::string& s, uint32_t* v) {
  if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
    char* end = nullptr;
    const unsigned long x = std::strtoul(s.c_str() + 2, &end, 16);
    if (end == nullptr || *end != 0 || x > 0xFFu) return false;
    *v = static_cast<uint32_t>(x);
    return true;
  }
  return parse_u32(s, 0, 0xFFu, v);
}

// key=value attributes after the positional tokens.
bool attrs(const std::vector<std::string>& t, size_t from, std::map<std::string, std::string>* kv, std::string* bad) {
  for (size_t i = from; i < t.size(); ++i) {
    const size_t eq = t[i].find('=');
    if (eq == std::string::npos || eq == 0) {
      *bad = "expected key=value, got '" + t[i] + "'";
      return false;
    }
    const std::string key = t[i].substr(0, eq);
    if (kv->count(key) != 0) {
      *bad = "attribute '" + key + "' given twice";
      return false;
    }
    (*kv)[key] = t[i].substr(eq + 1);
  }
  return true;
}

bool only(const std::map<std::string, std::string>& kv, const std::set<std::string>& allowed, std::string* bad) {
  for (const auto& e : kv)
    if (allowed.count(e.first) == 0) {
      *bad = "unknown attribute '" + e.first + "'";
      return false;
    }
  return true;
}

std::string get(const std::map<std::string, std::string>& kv, const std::string& k) {
  auto it = kv.find(k);
  return it == kv.end() ? std::string() : it->second;
}

bool one_of(const std::string& v, const std::set<std::string>& s) { return s.count(v) != 0; }

}  // namespace

const Value* Site::value(uint32_t slot) const {
  for (const Value& v : values)
    if (v.slot == slot) return &v;
  return nullptr;
}

uint32_t Site::declared_mask() const {
  uint32_t m = 0;
  for (const Value& v : values) m |= 1u << v.slot;
  return m;
}

const Site* Manifest::site(uint32_t id) const {
  auto it = site_index.find(id);
  return it == site_index.end() ? nullptr : &sites[it->second];
}
const Site* Manifest::site_by_label(const std::string& label) const {
  auto it = label_index.find(label);
  return it == label_index.end() ? nullptr : &sites[it->second];
}
const Bind* Manifest::bind(uint32_t id) const {
  auto it = bind_index.find(id);
  return it == bind_index.end() ? nullptr : &binds[it->second];
}
const Module* Manifest::module(uint32_t id) const {
  auto it = module_index.find(id);
  return it == module_index.end() ? nullptr : &modules[it->second];
}

bool identity_decoder_known(const std::string& name) {
  return one_of(name, {"spawn-create", "spawn-return", "spawn-id", "thread-entry", "owner-request", "child-request", "request-return"});
}

uint32_t ctx_code(const std::string& w) {
  if (w == "task") return KZ_RT_CTX_TASK;
  if (w == "lock-held") return KZ_RT_CTX_LOCK_HELD;
  if (w == "isr") return KZ_RT_CTX_ISR;
  if (w == "osal-internal") return KZ_RT_CTX_OSAL_INTERNAL;
  if (w == "terminating") return KZ_RT_CTX_TERMINATING;
  if (w == "unsupported") return KZ_RT_CTX_UNSUPPORTED;
  return 0;
}

const char* ctx_name(uint32_t c) {
  switch (c) {
    case KZ_RT_CTX_TASK: return "task";
    case KZ_RT_CTX_LOCK_HELD: return "lock-held";
    case KZ_RT_CTX_ISR: return "isr";
    case KZ_RT_CTX_OSAL_INTERNAL: return "osal-internal";
    case KZ_RT_CTX_TERMINATING: return "terminating";
    case KZ_RT_CTX_UNSUPPORTED: return "unsupported";
    default: return "undeclared";
  }
}

bool parse_manifest(std::string_view text, const std::string& name, Manifest* m, std::string* error) {
  static const uint32_t kMaxId = 2147483647u;
  int ln = 0;
  bool header = false;
  uint32_t version = 0;
  std::vector<std::string> t;
  auto fail = [&](const std::string& msg) {
    *error = name + ":" + std::to_string(ln) + ": " + msg;
    return false;
  };
  size_t pos = 0;
  // Bindings may name sites and modules of an earlier file, so references are checked as read.
  while (pos <= text.size()) {
    size_t nl = text.find('\n', pos);
    if (nl == std::string_view::npos) nl = text.size();
    const std::string_view line = text.substr(pos, nl - pos);
    pos = nl + 1;
    ++ln;
    if (!tokenize(line, &t)) return fail("unterminated quote");
    if (t.empty()) {
      if (nl == text.size()) break;
      continue;
    }
    if (!header) {
      if (t.size() != 2 || t[0] != "kzs-sites" || (t[1] != "1" && t[1] != "2")) return fail("the first statement must be 'kzs-sites 1' or 'kzs-sites 2'");
      header = true;
      version = t[1] == "2" ? 2u : 1u;
      if (version > m->version) m->version = version;
      continue;
    }
    const std::string& kw = t[0];
    std::map<std::string, std::string> kv;
    std::string bad;
    if (kw == "source") {
      if (t.size() < 2 || !attrs(t, 2, &kv, &bad) || !only(kv, {"rev", "ref"}, &bad)) return fail(bad.empty() ? "source <name> rev=..." : bad);
      if (get(kv, "rev").empty()) return fail("source needs rev=");
      for (const Source& s : m->sources)
        if (s.name == t[1]) return fail("source '" + t[1] + "' declared twice");
      Source s;
      s.name = t[1];
      s.rev = get(kv, "rev");
      s.ref = get(kv, "ref");
      s.line = ln;
      m->sources.push_back(s);
    } else if (kw == "module") {
      Module md;
      if (t.size() < 2 || !parse_u32(t[1], 1, kMaxId, &md.id)) return fail("module <id 1..2147483647>");
      if (!attrs(t, 2, &kv, &bad) || !only(kv, {"name", "source"}, &bad)) return fail(bad);
      md.name = get(kv, "name");
      md.source = get(kv, "source");
      if (md.name.empty() || md.source.empty()) return fail("module needs name= and source=");
      bool found = false;
      for (const Source& s : m->sources) found = found || s.name == md.source;
      if (!found) return fail("module source '" + md.source + "' is not declared");
      if (m->module_index.count(md.id) != 0) return fail("module id " + t[1] + " declared twice");
      md.line = ln;
      m->module_index[md.id] = m->modules.size();
      m->modules.push_back(md);
    } else if (kw == "site") {
      Site s;
      if (t.size() < 2 || !parse_u32(t[1], 1, kMaxId, &s.id)) return fail("site <id 1..2147483647>");
      if (!attrs(t, 2, &kv, &bad) || !only(kv, {"label", "op", "phase", "tu", "func", "callee", "line", "place", "ref"}, &bad)) return fail(bad);
      s.label = get(kv, "label");
      s.op = get(kv, "op");
      s.phase = get(kv, "phase");
      s.tu = get(kv, "tu");
      s.func = get(kv, "func");
      s.callee = get(kv, "callee");
      s.ref = get(kv, "ref");
      if (s.label.empty() || s.op.empty() || s.tu.empty() || s.func.empty()) return fail("site needs label=, op=, phase=, tu= and func=");
      if (!one_of(s.phase, {"before", "effect", "after", "exit", "point"})) return fail("phase is before|effect|after|exit|point");
      if (kv.count("line") != 0 && !parse_u32(get(kv, "line"), 1, kMaxId, &s.src_line)) return fail("line=<n>");
      if (kv.count("place") != 0) {
        // version 2: the statement a point or effect site is bound to (event_semantics.md section 3)
        s.place = get(kv, "place");
        if (version < 2) return fail("place= needs 'kzs-sites 2'");
        if (!one_of(s.place, {"at-call", "after-call", "line-start", "store"})) return fail("place is at-call|after-call|line-start|store");
        if (!one_of(s.phase, {"point", "effect", "exit"})) return fail("place= needs phase=point|effect|exit");
        const bool at_call = s.place == "at-call" || s.place == "after-call";
        if (at_call && s.callee.empty()) return fail("place=" + s.place + " needs callee=");
        if (!at_call && !s.callee.empty()) return fail("place=" + s.place + " takes no callee=");
        if (!at_call && s.src_line == 0) return fail("place=" + s.place + " needs line=");
      }
      if (m->site_index.count(s.id) != 0) return fail("site id " + t[1] + " declared twice");
      if (m->label_index.count(s.label) != 0) return fail("site label '" + s.label + "' declared twice");
      s.line = ln;
      m->site_index[s.id] = m->sites.size();
      m->label_index[s.label] = m->sites.size();
      m->sites.push_back(s);
    } else if (kw == "value") {
      uint32_t sid = 0;
      Value v;
      if (t.size() < 3 || !parse_u32(t[1], 1, kMaxId, &sid) || !parse_u32(t[2], 0, KZ_RT_MAX_VALS - 1u, &v.slot))
        return fail("value <site-id> <slot 0..7> ...");
      auto it = m->site_index.find(sid);
      if (it == m->site_index.end()) return fail("value of an undeclared site " + t[1]);
      Site& s = m->sites[it->second];
      if (!attrs(t, 3, &kv, &bad) || !only(kv, {"role", "from", "type", "string", "bound", "ref"}, &bad)) return fail(bad);
      v.role = get(kv, "role");
      v.from = get(kv, "from");
      v.type = get(kv, "type");
      v.ref = get(kv, "ref");
      if (v.role.empty() || v.from.empty() || v.type.empty()) return fail("value needs role=, from= and type=");
      if (!one_of(v.type, {"u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64", "ptr"})) return fail("type is u8..u64, s8..s64 or ptr");
      const bool from_param = v.from.size() > 5 && v.from.rfind("param", 0) == 0;
      const bool from_ok = v.from == "ret" || (v.from.size() > 3 && (v.from.rfind("arg", 0) == 0 || v.from.rfind("out", 0) == 0)) ||
                           (v.from.size() > 7 && v.from.rfind("operand", 0) == 0) || from_param;
      if (!from_ok) return fail("from is arg<k>, ret, out<k>, operand<k> or param<k>");
      if (from_param && version < 2) return fail("from=param<k> needs 'kzs-sites 2'");
      if (kv.count("string") != 0) {
        v.string_mode = get(kv, "string");
        if (!one_of(v.string_mode, {"pointer-only", "validated"})) return fail("string is pointer-only|validated");
        if (v.type != "ptr") return fail("string= needs type=ptr");
      }
      if (v.string_mode == "validated") {
        if (!parse_u32(get(kv, "bound"), 1, KZ_RT_STR_MAX, &v.bound)) return fail("string=validated needs bound=<1..64>");
        if (v.ref.empty()) return fail("string=validated needs ref= (where the original code validated the pointer)");
      } else if (kv.count("bound") != 0) {
        return fail("bound= is only for string=validated");
      }
      if (s.value(v.slot) != nullptr) return fail("slot " + t[2] + " of site " + t[1] + " declared twice");
      v.line = ln;
      s.values.push_back(v);
    } else if (kw == "control") {
      uint32_t sid = 0;
      if (t.size() < 2 || !parse_u32(t[1], 1, kMaxId, &sid)) return fail("control <site-id> ctx=...");
      auto it = m->site_index.find(sid);
      if (it == m->site_index.end()) return fail("control of an undeclared site " + t[1]);
      Site& s = m->sites[it->second];
      if (!attrs(t, 2, &kv, &bad) || !only(kv, {"ctx", "ref", "note"}, &bad)) return fail(bad);
      if (ctx_code(get(kv, "ctx")) == 0) return fail("ctx is task|lock-held|isr|osal-internal|terminating|unsupported");
      if (s.has_control) return fail("control of site " + t[1] + " declared twice");
      s.has_control = true;
      s.ctx = get(kv, "ctx");
      s.ctl_ref = get(kv, "ref");
      s.ctl_note = get(kv, "note");
    } else if (kw == "bind") {
      Bind b;
      if (t.size() < 2 || !parse_u32(t[1], 1, kMaxId, &b.id)) return fail("bind <id 1..2147483647> ...");
      if (!attrs(t, 2, &kv, &bad) || !only(kv, {"site", "backend", "module", "at", "status", "vals", "reason"}, &bad)) return fail(bad);
      if (!parse_u32(get(kv, "site"), 1, kMaxId, &b.site) || m->site(b.site) == nullptr) return fail("bind needs site=<declared site id>");
      if (!parse_u32(get(kv, "module"), 1, kMaxId, &b.module) || m->module(b.module) == nullptr) return fail("bind needs module=<declared module id>");
      b.backend = get(kv, "backend");
      b.at = get(kv, "at");
      b.status = get(kv, "status");
      b.reason = get(kv, "reason");
      if (b.backend.empty() || b.at.empty()) return fail("bind needs backend= and at=");
      if (!one_of(b.status, {"bound", "inlined", "duplicated", "removed", "unsupported"}))
        return fail("status is bound|inlined|duplicated|removed|unsupported");
      if ((b.status == "removed" || b.status == "unsupported") && b.reason.empty()) return fail("status=" + b.status + " needs reason=");
      const uint32_t declared = m->site(b.site)->declared_mask();
      if (kv.count("vals") != 0) {
        if (!parse_mask(get(kv, "vals"), &b.vals)) return fail("vals=<mask 0..0xff>");
        if ((b.vals & ~declared) != 0) return fail("vals names a slot the site does not declare");
        b.vals_given = true;
      } else {
        b.vals = declared;
      }
      if (m->bind_index.count(b.id) != 0) return fail("bind id " + t[1] + " declared twice");
      b.line = ln;
      m->bind_index[b.id] = m->binds.size();
      m->binds.push_back(b);
    } else if (kw == "equiv") {
      Equiv e;
      if (t.size() < 2 || !parse_u32(t[1], 1, kMaxId, &e.site) || m->site(e.site) == nullptr) return fail("equiv <declared site id> ...");
      if (!attrs(t, 2, &kv, &bad) || !only(kv, {"backends", "status", "ref"}, &bad)) return fail(bad);
      e.backends = get(kv, "backends");
      e.status = get(kv, "status");
      e.ref = get(kv, "ref");
      if (e.backends.find(',') == std::string::npos) return fail("backends=<a>,<b>");
      if (!one_of(e.status, {"unconfirmed", "confirmed"})) return fail("status is unconfirmed|confirmed");
      if (e.status == "confirmed" && e.ref.empty()) return fail("status=confirmed needs ref= (the evidence)");
      e.line = ln;
      m->equivs.push_back(e);
    } else if (kw == "identity") {
      if (version < 2) return fail("identity needs 'kzs-sites 2'");
      uint32_t sid = 0;
      if (t.size() < 2 || !parse_u32(t[1], 1, kMaxId, &sid)) return fail("identity <site-id> decoder=...");
      auto it = m->site_index.find(sid);
      if (it == m->site_index.end()) return fail("identity of an undeclared site " + t[1]);
      Site& s = m->sites[it->second];
      if (!attrs(t, 2, &kv, &bad) || !only(kv, {"decoder", "id", "owner", "status", "ok", "ref"}, &bad)) return fail(bad);
      if (s.has_identity) return fail("identity of site " + t[1] + " declared twice");
      Identity idn;
      idn.decoder = get(kv, "decoder");
      idn.ref = get(kv, "ref");
      if (!identity_decoder_known(idn.decoder))
        return fail("decoder is spawn-create|spawn-return|spawn-id|thread-entry|owner-request|child-request|request-return");
      const bool opening = one_of(idn.decoder, {"spawn-create", "thread-entry", "owner-request", "child-request"});
      const bool closing = one_of(idn.decoder, {"spawn-return", "request-return"});
      if (opening && s.phase != "before") return fail("decoder=" + idn.decoder + " needs a site of phase=before");
      if (closing && s.phase != "after") return fail("decoder=" + idn.decoder + " needs a site of phase=after");
      if (idn.decoder == "spawn-id" && !one_of(s.phase, {"before", "point", "effect"})) return fail("decoder=spawn-id needs a site of phase=before|point|effect");
      const bool takes_id = one_of(idn.decoder, {"spawn-return", "spawn-id", "thread-entry"});
      const bool needs_id = idn.decoder == "spawn-id" || idn.decoder == "thread-entry";
      const bool takes_owner = idn.decoder == "owner-request";
      // the slot: a declared integer value of at most 4 bytes (the readers keep 4 bytes, as F0's decoders)
      auto slot_of = [&](const std::string& key, int* out, bool any_integer = false) -> bool {
        uint32_t k = 0;
        if (!parse_u32(get(kv, key), 0, KZ_RT_MAX_VALS - 1u, &k)) {
          bad = key + "=<slot 0..7>";
          return false;
        }
        const Value* v = s.value(k);
        if (v == nullptr) {
          bad = key + "=" + get(kv, key) + ": site " + t[1] + " declares no value in that slot (declare the value first)";
          return false;
        }
        if (any_integer ? v->type == "ptr" : !one_of(v->type, {"u8", "u16", "u32", "s8", "s16", "s32"})) {
          bad = key + "=" + get(kv, key) + ": the slot is " + v->type + (any_integer ? ", a status is an integer" : ", an identity is an integer of at most 4 bytes");
          return false;
        }
        if (!any_integer && idn.decoder == "spawn-return" && v->from != "ret" && v->from.rfind("out", 0) != 0) {
          bad = "id= of a spawn-return is the id the call assigned: from=out<k> or ret, not " + v->from;
          return false;
        }
        *out = static_cast<int>(k);
        return true;
      };
      if (kv.count("id") != 0) {
        if (!takes_id) return fail("decoder=" + idn.decoder + " takes no id=");
        if (!slot_of("id", &idn.id_slot)) return fail(bad);
      } else if (needs_id) {
        return fail("decoder=" + idn.decoder + " needs id=<slot>");
      }
      if (kv.count("status") != 0 || kv.count("ok") != 0) {
        if (idn.decoder != "spawn-return" || kv.count("id") == 0) return fail("status= and ok= belong to the id= of a spawn-return");
      }
      if (idn.decoder == "spawn-return" && kv.count("id") != 0) {
        if (kv.count("status") == 0 || kv.count("ok") == 0)
          return fail("the id= of a spawn-return needs status=<slot> ok=<value of success> (a failed call leaves the out parameter as it was)");
        if (!slot_of("status", &idn.status_slot, true)) return fail(bad);
        const std::string okv = get(kv, "ok");
        char* end = nullptr;
        errno = 0;
        const long long x = std::strtoll(okv.c_str(), &end, 0);
        if (okv.empty() || end == nullptr || *end != 0 || errno != 0) return fail("ok=<integer>");
        idn.ok = x;
      }
      if (kv.count("owner") != 0) {
        if (!takes_owner) return fail("decoder=" + idn.decoder + " takes no owner=");
        if (!slot_of("owner", &idn.owner_slot)) return fail(bad);
      } else if (takes_owner) {
        return fail("decoder=owner-request needs owner=<slot>");
      }
      idn.line = ln;
      s.identity = idn;
      s.has_identity = true;
    } else {
      return fail("unknown statement '" + kw + "'");
    }
    if (nl == text.size()) break;
  }
  if (!header) {
    ln = 1;
    return fail("empty manifest (no 'kzs-sites 1' or 'kzs-sites 2')");
  }
  return true;
}

}  // namespace kzrt

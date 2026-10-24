// profile.cpp -- see profile.hpp.
#include "profile.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace kz::plan {

namespace {

struct Tok {
  std::string text;
  bool quoted = false;
};

Status err(uint32_t line, const std::string& what) {
  return Status::error(Err::Invalid, "profile line " + std::to_string(line) + ": " + what);
}

// Splits a line into tokens; returns false on an unterminated quote.
bool tokenize(const std::string& line, std::vector<Tok>* out) {
  size_t i = 0;
  const size_t n = line.size();
  while (i < n) {
    if (std::isspace(static_cast<unsigned char>(line[i]))) { ++i; continue; }
    if (line[i] == '#') break;
    Tok t;
    while (i < n && !std::isspace(static_cast<unsigned char>(line[i]))) {
      if (line[i] == '"') {
        t.quoted = true;
        ++i;
        bool closed = false;
        while (i < n) {
          if (line[i] == '\\' && i + 1 < n && (line[i + 1] == '"' || line[i + 1] == '\\')) { t.text.push_back(line[i + 1]); i += 2; continue; }
          if (line[i] == '"') { closed = true; ++i; break; }
          t.text.push_back(line[i++]);
        }
        if (!closed) return false;
      } else if (line[i] == '#') {
        break;
      } else {
        t.text.push_back(line[i++]);
      }
    }
    out->push_back(std::move(t));
    if (i < n && line[i] == '#') break;
  }
  return true;
}

// "key=value" -> (key, value); a token without '=' has an empty key.
void split_kv(const Tok& t, std::string* k, std::string* v) {
  const size_t eq = t.text.find('=');
  if (eq == std::string::npos) { k->clear(); *v = t.text; return; }
  *k = t.text.substr(0, eq);
  *v = t.text.substr(eq + 1);
}

bool parse_uint(const std::string& s, uint32_t* v) {
  if (s.empty() || s.size() > 9) return false;
  uint64_t x = 0;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
    x = x * 10 + static_cast<uint64_t>(c - '0');
  }
  *v = static_cast<uint32_t>(x);
  return true;
}

struct Names { const char* name; uint32_t value; };

const Names kTiers[] = {{"unreferenced", KZ_TIER_UNREFERENCED}, {"contract-backed", KZ_TIER_DOC_BACKED}, {"admitted", KZ_TIER_ADMITTED},
                        {"code-derived", KZ_TIER_CODE_DERIVED}, {"issue-derived", KZ_TIER_ISSUE_DERIVED}, {"researcher", KZ_TIER_RESEARCHER}};
const Names kKinds[] = {{"none", KZ_PROV_KIND_NONE}, {"api_doc", KZ_PROV_KIND_API_DOC}, {"requirement", KZ_PROV_KIND_REQUIREMENT},
                        {"code_admission", KZ_PROV_KIND_CODE_ADMISSION}, {"code_pattern", KZ_PROV_KIND_CODE_PATTERN},
                        {"issue", KZ_PROV_KIND_ISSUE}, {"researcher", KZ_PROV_KIND_RESEARCHER}};
const Names kSides[] = {{"n/a", KZ_PROV_SIDE_NA}, {"supplier", KZ_PROV_SIDE_SUPPLIER}, {"caller", KZ_PROV_SIDE_CALLER}, {"rule", KZ_PROV_SIDE_RULE}};
const Names kDecoders[] = {{"generic", KZ_DECODER_GENERIC},           {"spawn-create", KZ_DECODER_SPAWN_CREATE},
                           {"spawn-return", KZ_DECODER_SPAWN_RETURN},  {"thread-entry", KZ_DECODER_THREAD_ENTRY},
                           {"owner-request", KZ_DECODER_OWNER_REQUEST}, {"child-request", KZ_DECODER_CHILD_REQUEST},
                           {"request-return", KZ_DECODER_REQUEST_RETURN}, {"spawn-id", KZ_DECODER_SPAWN_ID}};
const Names kRegs[] = {{"rax", KZ_REG_RAX}, {"rbx", KZ_REG_RBX}, {"rcx", KZ_REG_RCX}, {"rdx", KZ_REG_RDX}, {"rsi", KZ_REG_RSI},
                       {"rdi", KZ_REG_RDI}, {"rbp", KZ_REG_RBP}, {"rsp", KZ_REG_RSP}, {"r8", KZ_REG_R8}, {"r9", KZ_REG_R9},
                       {"r10", KZ_REG_R10}, {"r11", KZ_REG_R11}, {"r12", KZ_REG_R12}, {"r13", KZ_REG_R13}, {"r14", KZ_REG_R14},
                       {"r15", KZ_REG_R15}};

template <size_t N>
bool lookup(const Names (&table)[N], const std::string& name, uint32_t* v) {
  for (const Names& e : table)
    if (name == e.name) { *v = e.value; return true; }
  return false;
}

Status parse_slot(uint32_t line, const std::string& key, const std::string& val, kz::trace::SlotSpec* out) {
  // key = s0..s3 ; val = src:width:role
  const size_t c1 = val.find(':');
  const size_t c2 = c1 == std::string::npos ? c1 : val.find(':', c1 + 1);
  if (c2 == std::string::npos) return err(line, key + ": a slot is <src>:<width>:<role>");
  const std::string src = val.substr(0, c1), width = val.substr(c1 + 1, c2 - c1 - 1), role = val.substr(c2 + 1);
  out->slot = static_cast<uint8_t>(key[1] - '0');
  out->role = role;
  if (role.empty()) return err(line, key + ": a slot needs a role");
  const bool is_string = width == "string";
  if (width == "u8") out->width = 1;
  else if (width == "u16") out->width = 2;
  else if (width == "u32") out->width = 4;
  else if (width == "u64") out->width = 8;
  else if (is_string) out->width = 4;  // the slot keeps the string's byte length; the text goes beside the event
  else return err(line, key + ": width must be u8, u16, u32, u64 or string");
  uint32_t reg = 0;
  const bool deref = !src.empty() && (src[0] == '+' || src[0] == '-') && src.back() == ')' && src.find('(') != std::string::npos;
  if (is_string && !deref) return err(line, key + ": a string is read from memory: +<offset>(<register>)");
  if (deref) {
    // +<offset>(<register>) or -<offset>(<register>): memory at register + offset, read by the kernel when the probe fires
    const size_t open = src.find('(');
    uint32_t off = 0;
    if (!parse_uint(src.substr(1, open - 1), &off) || off > 0x7fffffffu) return err(line, key + ": +<offset>(<register>) with a decimal offset");
    if (!lookup(kRegs, src.substr(open + 1, src.size() - open - 2), &reg)) return err(line, key + ": unknown register in '" + src + "'");
    out->src = is_string ? KZ_SLOT_SRC_STR : KZ_SLOT_SRC_MEM;
    out->index = reg;
    out->offset = src[0] == '-' ? -static_cast<int32_t>(off) : static_cast<int32_t>(off);
  } else if (src == "ip") {
    out->src = KZ_SLOT_SRC_IP;
  } else if (src.rfind("stack+", 0) == 0) {
    uint32_t off = 0;
    if (!parse_uint(src.substr(6), &off)) return err(line, key + ": stack+<byte offset>");
    out->src = KZ_SLOT_SRC_STACK;
    out->index = off;
  } else if (lookup(kRegs, src, &reg)) {
    out->src = KZ_SLOT_SRC_REG;
    out->index = reg;
  } else {
    return err(line, key + ": unknown slot source '" + src + "'");
  }
  return Status::success();
}

}  // namespace

Status parse_profile(std::string_view text, Profile* out) {
  *out = Profile();
  out->sha256 = sha256(text);
  std::map<std::string, int> prov_index;
  std::set<std::string> labels;
  bool header = false;
  uint32_t lineno = 0;
  size_t pos = 0;
  while (pos <= text.size()) {
    const size_t nl = text.find('\n', pos);
    const std::string line(text.substr(pos, nl == std::string_view::npos ? std::string_view::npos : nl - pos));
    pos = nl == std::string_view::npos ? text.size() + 1 : nl + 1;
    ++lineno;
    std::vector<Tok> toks;
    if (!tokenize(line, &toks)) return err(lineno, "unterminated quote");
    if (toks.empty()) continue;
    const std::string& cmd = toks[0].text;
    if (!header) {
      if (cmd != "kzp-observe" || toks.size() != 2 || toks[1].text != "1") return err(lineno, "the first statement must be 'kzp-observe 1'");
      header = true;
      continue;
    }
    if (cmd == "binary") {
      if (toks.size() != 2) return err(lineno, "binary <name>");
      if (std::find(out->binaries.begin(), out->binaries.end(), toks[1].text) != out->binaries.end())
        return err(lineno, "binary '" + toks[1].text + "' is declared twice");
      if (!out->sites.empty()) return err(lineno, "binaries are declared before the first site");
      if (out->binaries.size() >= 0xffff) return err(lineno, "too many binaries");
      out->binaries.push_back(toks[1].text);
      if (out->binary_name.empty()) out->binary_name = toks[1].text;
    } else if (cmd == "prov") {
      if (toks.size() < 2) return err(lineno, "prov <id> key=value ...");
      ProvDecl p;
      p.id = toks[1].text;
      p.source_line = lineno;
      if (prov_index.count(p.id) != 0) return err(lineno, "provenance id '" + p.id + "' is declared twice");
      bool have_tier = false, have_file = false, have_lines = false;
      for (size_t i = 2; i < toks.size(); ++i) {
        std::string k, v;
        split_kv(toks[i], &k, &v);
        uint32_t x = 0;
        if (k == "tier") { if (!lookup(kTiers, v, &x)) return err(lineno, "unknown tier '" + v + "'"); p.tier = static_cast<uint8_t>(x); have_tier = true; }
        else if (k == "kind") { if (!lookup(kKinds, v, &x)) return err(lineno, "unknown provenance kind '" + v + "'"); p.kind = static_cast<uint8_t>(x); }
        else if (k == "side") { if (!lookup(kSides, v, &x)) return err(lineno, "unknown side '" + v + "'"); p.side = static_cast<uint8_t>(x); }
        else if (k == "file") { p.file = v; have_file = true; }
        else if (k == "cite") p.cite = v;
        else if (k == "authored") p.authored = v;
        else if (k == "root") p.root = v;
        else if (k == "sha256") {
          if (!digest_from_hex(v, &p.sha256)) return err(lineno, "sha256= takes exactly 64 hex digits");
          p.has_sha256 = true;
        }
        else if (k == "lines") {
          const size_t dash = v.find('-');
          uint32_t a = 0, b = 0;
          if (dash == std::string::npos || !parse_uint(v.substr(0, dash), &a) || !parse_uint(v.substr(dash + 1), &b) || a == 0 || b < a)
            return err(lineno, "lines=<from>-<to> with 1 <= from <= to");
          p.line_from = a; p.line_to = b; have_lines = true;
        } else return err(lineno, "unknown prov key '" + k + "'");
      }
      if (!have_tier) return err(lineno, "prov " + p.id + " needs tier=");
      // Every entry except `unreferenced` must say where it comes from: a file and lines (a researcher
      // entry names the document of its decision), so that nothing in the plan is a bare assertion.
      if (p.tier != KZ_TIER_UNREFERENCED && (!have_file || !have_lines)) return err(lineno, "prov " + p.id + " needs file= and lines=");
      prov_index[p.id] = static_cast<int>(out->provs.size());
      out->provs.push_back(std::move(p));
    } else if (cmd == "site") {
      if (toks.size() < 4) return err(lineno, "site <label> <entry|ret|point> <target> ...");
      ProfileSite s;
      s.label = toks[1].text;
      s.source_line = lineno;
      if (!labels.insert(s.label).second) return err(lineno, "site label '" + s.label + "' is declared twice");
      if (toks[2].text == "entry") s.kind = KZ_SITE_KIND_ENTRY_PROBE;
      else if (toks[2].text == "ret") s.kind = KZ_SITE_KIND_RET_PROBE;
      else if (toks[2].text == "point") s.kind = KZ_SITE_KIND_POINT_PROBE;
      else return err(lineno, "a site is entry, ret or point");
      bool have_slot[4] = {false, false, false, false};
      for (size_t i = 3; i < toks.size(); ++i) {
        std::string k, v;
        split_kv(toks[i], &k, &v);
        uint32_t x = 0;
        if (k == "func") s.func = v;
        else if (k == "in") s.in_suffix = v;
        else if (k == "at") {
          const size_t colon = v.rfind(':');
          if (colon == std::string::npos || !parse_uint(v.substr(colon + 1), &s.at_line) || s.at_line == 0) return err(lineno, "at=<file suffix>:<line>");
          s.at_file = v.substr(0, colon);
        } else if (k == "after-call") s.after_call = v;
        else if (k == "at-call") s.at_call = v;
        else if (k == "enter") {
          size_t b = 0;
          while (b <= v.size()) {
            const size_t e = v.find('>', b);
            const std::string part = v.substr(b, e == std::string::npos ? std::string::npos : e - b);
            if (part.empty()) return err(lineno, "enter=<function>[><function>...]: an empty function name");
            s.enter.push_back(part);
            if (e == std::string::npos) break;
            b = e + 1;
          }
        }
        else if (k.empty() && v == "line-start") s.line_start = true;
        else if (k == "decoder") { if (!lookup(kDecoders, v, &x)) return err(lineno, "unknown decoder '" + v + "'"); s.decoder = static_cast<uint16_t>(x); }
        else if (k == "bin") {
          const auto it = std::find(out->binaries.begin(), out->binaries.end(), v);
          if (it == out->binaries.end()) return err(lineno, "binary '" + v + "' is not declared before this site");
          s.bin = static_cast<uint16_t>(it - out->binaries.begin());
        }
        else if (k == "prov") {
          auto it = prov_index.find(v);
          if (it == prov_index.end()) return err(lineno, "provenance '" + v + "' is not declared before this site");
          s.prov = it->second;
        } else if (k.size() == 2 && k[0] == 's' && k[1] >= '0' && k[1] <= '3') {
          if (have_slot[k[1] - '0']) return err(lineno, "slot " + k + " is given twice");
          have_slot[k[1] - '0'] = true;
          kz::trace::SlotSpec sl;
          Status st = parse_slot(lineno, k, v, &sl);
          if (!st.ok()) return st;
          s.slots.push_back(std::move(sl));
        } else return err(lineno, "unknown site key '" + k + "'");
      }
      if (s.kind == KZ_SITE_KIND_POINT_PROBE) {
        const int line_forms = (s.after_call.empty() ? 0 : 1) + (s.line_start ? 1 : 0) + (s.at_call.empty() ? 0 : 1);
        const bool by_line = !s.at_file.empty() && s.func.empty() && line_forms == 1;
        const bool by_function = s.at_file.empty() && !s.func.empty() && !s.after_call.empty() && !s.line_start && s.at_call.empty() && s.in_suffix.empty() && s.enter.empty();
        const bool by_entry = s.at_file.empty() && !s.func.empty() && s.after_call.empty() && !s.line_start && s.at_call.empty() && s.in_suffix.empty() && !s.enter.empty();
        if ((!by_line || !s.enter.empty()) && !by_function && !by_entry)
          return err(lineno, "a point site is at=<file>:<line> with after-call=<symbol>, at-call=<symbol> or line-start, func=<symbol> after-call=<symbol>, or func=<symbol> enter=<function>[>...]");
      } else {
        if (s.func.empty() || !s.at_file.empty() || !s.after_call.empty() || !s.at_call.empty() || s.line_start || !s.enter.empty())
          return err(lineno, "an entry or ret site is func=<symbol> [in=<file suffix>]");
      }
      out->sites.push_back(std::move(s));
    } else {
      return err(lineno, "unknown statement '" + cmd + "'");
    }
  }
  if (!header) return Status::error(Err::Invalid, "profile: empty (no 'kzp-observe 1' header)");
  if (out->binaries.empty() && !out->sites.empty()) {
    // A plan without a binary statement has one, unnamed, primary binary (the earlier grammar did not
    // require the statement).
    out->binaries.push_back(std::string());
  }
  if (out->sites.empty()) return Status::error(Err::Invalid, "profile: no site is declared");
  // The slots of every site must be valid for the perf-sample backend, and the decoders and the
  // entry/return pairs (same func=) must find their keys (kz::trace::check_plan).  The function
  // name stands for the symbol here; `kz anchor` resolves it, and make_plan checks again.
  std::vector<kz::trace::SiteSpec> specs;
  for (const ProfileSite& s : out->sites) {
    kz::trace::SiteSpec spec;
    spec.site = static_cast<uint32_t>(specs.size());
    spec.label = s.label;
    spec.kind = s.kind;
    spec.phases = s.kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_SITE_PHASE_ENTRY : s.kind == KZ_SITE_KIND_RET_PROBE ? KZ_SITE_PHASE_RET : KZ_SITE_PHASE_POINT;
    spec.decoder = s.decoder;
    spec.symbol = s.func;
    spec.slots = s.slots;
    spec.binary = s.bin;
    if (s.entry_reached()) {
      spec.symbol = s.enter.back();  // the probe is in the last function of the chain
      spec.via_symbol = s.func;
    }
    Status st = kz::trace::check_site(spec);
    if (!st.ok()) return Status::error(Err::Invalid, "profile line " + std::to_string(s.source_line) + ": " + st.message);
    specs.push_back(std::move(spec));
  }
  Status st = kz::trace::check_plan(specs);
  if (!st.ok()) return Status::error(Err::Invalid, "profile: " + st.message);
  return Status::success();
}

}  // namespace kz::plan

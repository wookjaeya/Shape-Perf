// hold_plan.cpp -- see hold_plan.hpp.
#include "hold_plan.hpp"

#include <string>
#include <vector>

namespace kz::hold {

namespace {

// Split a line into tokens; a quoted token keeps its spaces (\" and \\ escapes).  A '#' outside
// quotes ends the line.  Returns false on an unterminated quote.
bool tokenize(const std::string& line, std::vector<std::string>* out) {
  out->clear();
  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= line.size()) break;
    if (line[i] == '#') break;
    std::string tok;
    if (line[i] == '"') {
      ++i;
      while (i < line.size() && line[i] != '"') {
        if (line[i] == '\\' && i + 1 < line.size()) {
          ++i;
          tok.push_back(line[i]);
        } else {
          tok.push_back(line[i]);
        }
        ++i;
      }
      if (i >= line.size()) return false;  // unterminated
      ++i;                                  // closing quote
    } else {
      while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '#') tok.push_back(line[i++]);
    }
    out->push_back(std::move(tok));
  }
  return true;
}

Status err(size_t line, const std::string& m) {
  return Status::error(Err::Invalid, "hold plan line " + std::to_string(line) + ": " + m);
}

// Parses "when name <role> "text"" / "when caller-in <bin>" (either or both), consuming tokens from
// `t` starting at `i`; also "occurrence first|each".  Fills cond and occurrence.
Status parse_tail(const std::vector<std::string>& t, size_t i, size_t ln, HoldCond* cond, bool* each) {
  while (i < t.size()) {
    if (t[i] == "when") {
      ++i;
      if (i >= t.size()) return err(ln, "when needs a condition");
      if (t[i] == "name") {
        if (i + 2 >= t.size()) return err(ln, "name needs a slot role and a string");
        cond->has_name = true;
        cond->name_role = t[i + 1];
        cond->name_text = t[i + 2];
        i += 3;
      } else if (t[i] == "caller-in") {
        if (i + 1 >= t.size()) return err(ln, "caller-in needs a binary name");
        cond->has_caller_in = true;
        cond->caller_binary = t[i + 1];
        i += 2;
      } else {
        return err(ln, "unknown condition '" + t[i] + "'");
      }
    } else if (t[i] == "occurrence") {
      if (i + 1 >= t.size()) return err(ln, "occurrence needs first or each");
      if (t[i + 1] == "first") *each = false;
      else if (t[i + 1] == "each") *each = true;
      else return err(ln, "occurrence is first or each");
      i += 2;
    } else {
      return err(ln, "unexpected token '" + t[i] + "'");
    }
  }
  return Status::success();
}

// Pulls the value of a key=value token (e.g. "site=foo").
bool kv(const std::string& tok, const std::string& key, std::string* val) {
  if (tok.size() <= key.size() + 1 || tok.compare(0, key.size(), key) != 0 || tok[key.size()] != '=') return false;
  *val = tok.substr(key.size() + 1);
  return true;
}

}  // namespace

Status parse_hold_plan(std::string_view text, HoldPlan* out) {
  *out = HoldPlan();
  size_t ln = 0, start = 0;
  bool header = false;
  while (start <= text.size()) {
    size_t nl = text.find('\n', start);
    std::string line(text.substr(start, nl == std::string_view::npos ? text.size() - start : nl - start));
    start = (nl == std::string_view::npos) ? text.size() + 1 : nl + 1;
    ++ln;
    std::vector<std::string> t;
    if (!tokenize(line, &t)) return err(ln, "unterminated quote");
    if (t.empty()) continue;
    if (!header) {
      if (t.size() != 2 || t[0] != "kzp-hold" || t[1] != "1") return err(ln, "the first statement must be 'kzp-hold 1'");
      header = true;
      continue;
    }
    if (t[0] == "gate") {
      if (t.size() < 3) return err(ln, "gate needs a label and site=");
      GateDecl g;
      g.label = t[1];
      if (!kv(t[2], "site", &g.site_label)) return err(ln, "gate needs site=<label>");
      Status s = parse_tail(t, 3, ln, &g.cond, &g.occurrence_each);
      if (!s.ok()) return s;
      out->gates.push_back(std::move(g));
    } else if (t[0] == "release") {
      if (t.size() < 5) return err(ln, "release needs a label, site= and releases <gate>");
      ReleaseDecl r;
      r.label = t[1];
      if (!kv(t[2], "site", &r.site_label)) return err(ln, "release needs site=<label>");
      if (t[3] != "releases") return err(ln, "release needs 'releases <gate-label>'");
      r.releases = t[4];
      Status s = parse_tail(t, 5, ln, &r.cond, &r.occurrence_each);
      if (!s.ok()) return s;
      out->releases.push_back(std::move(r));
    } else {
      return err(ln, "unknown statement '" + t[0] + "'");
    }
  }
  if (!header) return Status::error(Err::Invalid, "hold plan: empty or missing 'kzp-hold 1' header");
  // Every release must name an existing gate.
  for (const ReleaseDecl& r : out->releases) {
    bool found = false;
    for (const GateDecl& g : out->gates) found = found || g.label == r.releases;
    if (!found) return Status::error(Err::Invalid, "hold plan: release '" + r.label + "' releases unknown gate '" + r.releases + "'");
  }
  return Status::success();
}

}  // namespace kz::hold

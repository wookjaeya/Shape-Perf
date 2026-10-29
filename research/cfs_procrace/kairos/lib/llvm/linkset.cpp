// linkset.cpp -- see linkset.hpp.
#include "linkset.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "elf_info.hpp"
#include "kz/core/container.hpp"

namespace kzs {

namespace fs = std::filesystem;

std::string normalize_path(const std::string& path) { return fs::path(path).lexically_normal().generic_string(); }

size_t LinkSet::included() const {
  size_t n = 0;
  for (const LinkMember& m : members)
    if (m.mode != LinkMember::NotPulled) ++n;
  return n;
}

std::vector<std::string> split_command(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  bool in_token = false;
  char quote = 0;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (quote != 0) {
      if (c == quote) quote = 0;
      else if (c == '\\' && quote == '"' && i + 1 < line.size() && (line[i + 1] == '"' || line[i + 1] == '\\')) cur.push_back(line[++i]);
      else cur.push_back(c);
      continue;
    }
    if (c == '"' || c == '\'') {
      quote = c;
      in_token = true;
      continue;
    }
    if (c == '\\' && i + 1 < line.size()) {
      cur.push_back(line[++i]);
      in_token = true;
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      if (in_token) out.push_back(cur);
      cur.clear();
      in_token = false;
      continue;
    }
    cur.push_back(c);
    in_token = true;
  }
  if (in_token) out.push_back(cur);
  return out;
}

std::string join_command(const std::vector<std::string>& tokens) {
  std::string out;
  for (size_t i = 0; i < tokens.size(); ++i) {
    const std::string& t = tokens[i];
    if (i != 0) out += ' ';
    bool plain = !t.empty();
    for (char c : t)
      if (c == ' ' || c == '\t' || c == '"' || c == '\'' || c == '\\' || c == '\n') plain = false;
    if (plain) {
      out += t;
      continue;
    }
    out += '\'';
    for (char c : t) {
      if (c == '\'') out += "'\\''";
      else out += c;
    }
    out += '\'';
  }
  return out;
}

namespace {

bool ends_with(const std::string& s, const char* suffix) {
  const size_t n = std::char_traits<char>::length(suffix);
  return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

std::string absolute_in(const std::string& cwd, const std::string& p) {
  if (!p.empty() && p[0] == '/') return normalize_path(p);
  return normalize_path(cwd + "/" + p);
}

}  // namespace

kz::Status parse_link_command(const std::string& text, const std::string& cwd, LinkPlan* out) {
  out->cwd = normalize_path(cwd);
  bool whole = false;
  std::istringstream in(text);
  std::string line;
  bool saw_command = false;
  while (std::getline(in, line)) {
    const std::vector<std::string> tok = split_command(line);
    if (tok.empty()) continue;
    saw_command = true;
    for (size_t i = 1; i < tok.size(); ++i) {
      const std::string& t = tok[i];
      if (t == "-Wl,--whole-archive") { whole = true; continue; }
      if (t == "-Wl,--no-whole-archive") { whole = false; continue; }
      if (t == "-o") {  // the output
        if (i + 1 < tok.size()) out->output = absolute_in(out->cwd, tok[i + 1]);
        ++i;
        continue;
      }
      if (!t.empty() && t[0] == '-') {
        out->ignored.push_back(t);
        continue;
      }
      LinkInput li;
      li.as_written = t;
      li.path = absolute_in(out->cwd, t);
      li.whole = whole;
      if (ends_with(t, ".a")) li.kind = LinkInput::Archive;
      else if (ends_with(t, ".o")) li.kind = LinkInput::Object;
      else {
        out->ignored.push_back(t);
        continue;
      }
      out->inputs.push_back(li);
    }
  }
  if (!saw_command) return kz::Status::error(kz::Err::Invalid, "the link command file is empty");
  return kz::Status::success();
}

kz::Status archive_members(const std::string& archive, std::vector<std::string>* objects) {
  objects->clear();
  const fs::path ap(archive);
  const fs::path dir = ap.parent_path();
  const std::string name = ap.filename().string();
  std::error_code ec;
  const fs::path cmake_files = dir / "CMakeFiles";
  if (!fs::is_directory(cmake_files, ec)) return kz::Status::error(kz::Err::Io, "no CMakeFiles directory next to " + archive);
  std::vector<fs::path> link_files;
  for (const fs::directory_entry& e : fs::directory_iterator(cmake_files, ec)) {
    const fs::path lf = e.path() / "link.txt";
    if (fs::is_regular_file(lf, ec)) link_files.push_back(lf);
  }
  std::sort(link_files.begin(), link_files.end());
  for (const fs::path& lf : link_files) {
    std::ifstream f(lf);
    std::string line;
    while (std::getline(f, line)) {
      const std::vector<std::string> tok = split_command(line);
      // <ar> qc <archive> <objects...>
      size_t i = 0;
      while (i < tok.size() && fs::path(tok[i]).filename() != "ar") ++i;
      if (i + 3 > tok.size() || i >= tok.size()) continue;
      if (tok[i + 2] != name && fs::path(tok[i + 2]).filename() != name) continue;
      for (size_t k = i + 3; k < tok.size(); ++k)
        if (ends_with(tok[k], ".o")) objects->push_back(absolute_in(dir.string(), tok[k]));
      return kz::Status::success();
    }
  }
  return kz::Status::error(kz::Err::Io, "no `ar` command for " + archive + " among the link files under " + cmake_files.string());
}

kz::Status resolve_link_set(const LinkPlan& plan, LinkSet* out) {
  out->members.clear();
  out->elf = ElfTarget();
  std::set<std::string> defined, undefined;
  auto absorb = [&](const ObjectSymbols& syms) {
    for (const std::string& d : syms.defined) {
      defined.insert(d);
      undefined.erase(d);
    }
    for (const std::string& u : syms.undefined)
      if (defined.count(u) == 0) undefined.insert(u);
  };
  for (const LinkInput& in : plan.inputs) {
    if (in.kind == LinkInput::Object) {
      ObjectSymbols syms;
      kz::Status s = read_object_symbols(in.path, &syms);
      if (s.ok()) s = out->elf.add(in.path, syms.elf);
      if (!s.ok()) return s;
      absorb(syms);
      LinkMember m;
      m.object = in.path;
      m.mode = LinkMember::Named;
      out->members.push_back(m);
      continue;
    }
    ++out->archives;
    std::vector<std::string> objs;
    kz::Status s = archive_members(in.path, &objs);
    if (!s.ok()) return s;
    if (in.whole) {
      for (const std::string& o : objs) {
        ObjectSymbols syms;
        s = read_object_symbols(o, &syms);
        if (s.ok()) s = out->elf.add(o, syms.elf);
        if (!s.ok()) return s;
        absorb(syms);
        LinkMember m;
        m.object = o;
        m.archive = in.path;
        m.mode = LinkMember::Whole;
        out->members.push_back(m);
      }
      continue;
    }
    // lazy archive: pull members that define a symbol that is undefined now, until nothing changes
    std::vector<ObjectSymbols> syms(objs.size());
    for (size_t i = 0; i < objs.size(); ++i) {
      s = read_object_symbols(objs[i], &syms[i]);
      if (s.ok()) s = out->elf.add(objs[i], syms[i].elf);
      if (!s.ok()) return s;
    }
    std::vector<int> state(objs.size(), 0);  // 0 not pulled, 1 pulled
    std::vector<std::string> why(objs.size());
    bool changed = true;
    while (changed) {
      changed = false;
      for (size_t i = 0; i < objs.size(); ++i) {
        if (state[i] != 0) continue;
        std::string needed;
        for (const std::string& d : syms[i].defined)
          if (undefined.count(d) != 0) {
            needed = d;
            break;
          }
        if (needed.empty()) continue;
        state[i] = 1;
        why[i] = needed;
        absorb(syms[i]);
        changed = true;
      }
    }
    for (size_t i = 0; i < objs.size(); ++i) {
      LinkMember m;
      m.object = objs[i];
      m.archive = in.path;
      m.mode = state[i] != 0 ? LinkMember::Pulled : LinkMember::NotPulled;
      m.pulled_for = why[i];
      out->members.push_back(m);
    }
  }
  return kz::Status::success();
}

}  // namespace kzs

// gitrev.cpp -- see gitrev.hpp.
#include "gitrev.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace kzs {

namespace fs = std::filesystem;

namespace {

bool read_text(const fs::path& p, std::string* out) {
  std::ifstream f(p);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  while (!out->empty() && (out->back() == '\n' || out->back() == '\r' || out->back() == ' ')) out->pop_back();
  return true;
}

bool is_commit_id(const std::string& s) {
  if (s.size() != 40) return false;
  for (char c : s)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}

}  // namespace

kz::Status read_git_head(const std::string& tree_dir, std::string* commit) {
  std::error_code ec;
  fs::path git = fs::path(tree_dir) / ".git";
  fs::path repo;
  if (fs::is_directory(git, ec)) {
    repo = git;
  } else if (fs::is_regular_file(git, ec)) {
    std::string line;
    if (!read_text(git, &line) || line.compare(0, 8, "gitdir: ") != 0) return kz::Status::error(kz::Err::Invalid, git.string() + " is not a gitdir file");
    repo = fs::path(line.substr(8));
    if (repo.is_relative()) repo = (fs::path(tree_dir) / repo).lexically_normal();
  } else {
    return kz::Status::error(kz::Err::Io, "no .git in " + tree_dir);
  }
  std::string head;
  if (!read_text(repo / "HEAD", &head)) return kz::Status::error(kz::Err::Io, "cannot read HEAD of " + repo.string());
  if (is_commit_id(head)) {
    *commit = head;
    return kz::Status::success();
  }
  if (head.compare(0, 5, "ref: ") != 0) return kz::Status::error(kz::Err::Invalid, "unrecognized HEAD in " + repo.string());
  const std::string ref = head.substr(5);
  std::string id;
  if (read_text(repo / ref, &id) && is_commit_id(id)) {
    *commit = id;
    return kz::Status::success();
  }
  // a submodule's repository keeps its refs in the common directory
  fs::path common = repo;
  std::string cd;
  if (read_text(repo / "commondir", &cd)) common = (repo / cd).lexically_normal();
  if (common != repo && read_text(common / ref, &id) && is_commit_id(id)) {
    *commit = id;
    return kz::Status::success();
  }
  std::ifstream packed(common / "packed-refs");
  std::string line;
  while (std::getline(packed, line)) {
    if (line.empty() || line[0] == '#' || line[0] == '^') continue;
    const size_t sp = line.find(' ');
    if (sp == std::string::npos) continue;
    if (line.substr(sp + 1) == ref && is_commit_id(line.substr(0, sp))) {
      *commit = line.substr(0, sp);
      return kz::Status::success();
    }
  }
  return kz::Status::error(kz::Err::Invalid, "cannot resolve " + ref + " in " + repo.string());
}

}  // namespace kzs

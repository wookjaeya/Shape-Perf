#include "source_identity.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>

#include "kz/core/sha256.hpp"

namespace fs = std::filesystem;

namespace kzfix {

namespace {

bool hidden(const fs::path& relative) {
  for (const fs::path& part : relative)
    if (!part.empty() && part.string()[0] == '.') return true;
  return false;
}

std::string hash_file(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream all;
  all << in.rdbuf();
  const std::string bytes = all.str();
  return kz::to_hex(kz::sha256(bytes.data(), bytes.size()));
}

}  // namespace

std::string recompute_source_identity(const std::string& root, const std::vector<std::string>& paths) {
  std::set<std::string> files;  // std::set orders std::string by bytes, like CMake's list(SORT)
  std::error_code ec;
  for (const std::string& p : paths) {
    const fs::path full = fs::path(root) / p;
    if (fs::is_directory(full, ec)) {
      for (fs::recursive_directory_iterator it(full, ec), end; it != end; it.increment(ec)) {
        if (ec) return "";
        if (!it->is_regular_file(ec)) continue;
        files.insert(fs::relative(it->path(), root, ec).generic_string());
      }
    } else if (fs::is_regular_file(full, ec)) {
      files.insert(fs::path(p).generic_string());
    } else {
      return "";
    }
  }
  std::string text;
  for (const std::string& f : files) {
    if (hidden(fs::path(f)) || (!f.empty() && f.back() == '~')) continue;
    text += f + " " + hash_file(fs::path(root) / f) + "\n";
  }
  return kz::to_hex(kz::sha256(text.data(), text.size()));
}

std::vector<std::string> split_paths(const std::string& joined) {
  std::vector<std::string> out;
  std::stringstream in(joined);
  std::string item;
  while (std::getline(in, item, ':'))
    if (!item.empty()) out.push_back(item);
  return out;
}

}  // namespace kzfix

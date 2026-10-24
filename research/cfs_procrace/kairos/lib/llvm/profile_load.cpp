// profile_load.cpp -- see profile_load.hpp.
#include "profile_load.hpp"

#include <algorithm>
#include <filesystem>

#include "kz/core/container.hpp"

namespace kzs {

namespace fs = std::filesystem;

kz::Status load_profile_dir(const std::string& path, kz::phi::Profile* out, std::string* diag_text) {
  return load_profile_paths(std::vector<std::string>{path}, out, diag_text);
}

kz::Status load_profile_paths(const std::vector<std::string>& args, kz::phi::Profile* out, std::string* diag_text) {
  std::vector<std::string> paths;
  for (const std::string& path : args) {
    std::error_code ec;
    if (fs::is_directory(path, ec)) {
      std::vector<std::string> in_dir;
      for (const fs::directory_entry& e : fs::directory_iterator(path, ec))
        if (e.is_regular_file() && e.path().extension() == ".kzp") in_dir.push_back(e.path().string());
      std::sort(in_dir.begin(), in_dir.end());
      if (in_dir.empty()) return kz::Status::error(kz::Err::Invalid, "no .kzp file in " + path);
      paths.insert(paths.end(), in_dir.begin(), in_dir.end());
    } else {
      paths.push_back(path);
    }
  }
  if (paths.empty()) return kz::Status::error(kz::Err::Invalid, "no profile given");
  std::vector<kz::phi::SourceText> files;
  for (const std::string& p : paths) {
    kz::Bytes b;
    kz::Status s = kz::read_whole_file(p, &b);
    if (!s.ok()) return s;
    files.push_back(kz::phi::SourceText{p, std::string(b.begin(), b.end())});
  }
  std::vector<kz::phi::Diag> diags;
  kz::Status s = kz::phi::load_profile(files, out, &diags);
  for (const kz::phi::Diag& d : diags) *diag_text += kz::phi::format_diag(out->files, d) + "\n";
  return s;
}

}  // namespace kzs

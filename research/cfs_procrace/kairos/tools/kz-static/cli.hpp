// cli.hpp -- the small argument reader and the shared helpers of the kz-static commands.
#pragma once

#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/phi.hpp"

namespace kzs_cli {

struct Args {
  std::map<std::string, std::vector<std::string>> values;  // valued options, in the order given
  std::set<std::string> flags;                              // boolean options
  std::vector<std::string> positional;
  std::vector<std::string> after_dashes;                    // everything after a lone `--`

  bool has(const std::string& name) const { return values.count(name) != 0 || flags.count(name) != 0; }
  std::string one(const std::string& name, const std::string& fallback = "") const {
    auto it = values.find(name);
    return it == values.end() || it->second.empty() ? fallback : it->second.back();
  }
  const std::vector<std::string>& many(const std::string& name) const {
    static const std::vector<std::string> none;
    auto it = values.find(name);
    return it == values.end() ? none : it->second;
  }
};

// Parses argv[first..).  `valued` options take the next argument, `boolean` ones do not; a
// lone `--` ends the options.  Returns false (with `error`) on an unknown option or a missing value.
bool parse_args(int argc, char** argv, int first, const std::set<std::string>& valued, const std::set<std::string>& boolean, Args* out, std::string* error);

// Reads the profile directory or file; prints the diagnostics to stderr.  False on errors.
bool load_profile(const std::vector<std::string>& paths, kz::phi::Profile* profile);  // --profile, repeatable

bool read_text_file(const std::string& path, std::string* out);

// The default location of Clang's builtin headers, unless --resource-dir is given.
std::string resource_dir(const Args& args);

int fail(const char* what, const std::string& message);

}  // namespace kzs_cli

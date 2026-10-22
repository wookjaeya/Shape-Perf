// profile_load.hpp -- reading a profile directory for the analysis tools.
//
// A directory stands for its *.kzp files in the order of their names, as for `kz-phi`.
#pragma once

#include <string>
#include <vector>

#include "kz/core/phi.hpp"

namespace kzs {

// Reads, parses and checks the profile.  Diagnostics are appended to `diag_text`, one per line.
kz::Status load_profile_dir(const std::string& dir_or_file, kz::phi::Profile* out, std::string* diag_text);

}  // namespace kzs

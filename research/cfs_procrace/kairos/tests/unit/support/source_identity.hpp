// source_identity.hpp -- an independent computation of the source identity of
// cmake/KzSourceIdentity.cmake, to check the value that CMake compiled into a binary.
#pragma once

#include <string>
#include <vector>

namespace kzfix {

// SHA-256 (lower-case hex) of the lines "<relative path> <sha256 of the file>\n", sorted by
// path, over the regular files under `paths` (directories are searched recursively; paths are
// relative to `root`).  Files and directories whose name starts with '.' and files ending in
// '~' are left out.  Returns the empty string if a path does not exist.
std::string recompute_source_identity(const std::string& root, const std::vector<std::string>& paths);

// Splits "a:b:c" at ':'.
std::vector<std::string> split_paths(const std::string& joined);

}  // namespace kzfix

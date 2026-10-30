// gitrev.hpp -- the commit a source tree is at, read from the repository files.
//
// No process is started and the repository is not modified.  (The git command refuses to run in
// a tree owned by another user, and a basis must not depend on that.)  The commit is HEAD of the
// tree's repository: a `.git` directory, or a `.git` file `gitdir: <path>` that points at the
// repository of a submodule.  HEAD is a commit id or `ref: <name>`; a name is looked up in the
// loose refs and in packed-refs.
//
// What this does not say: whether the working tree differs from that commit.  The basis hashes
// every source file it reads, which is the stronger statement; the commit is the label.
#pragma once

#include <string>

#include "kz/core/status.hpp"

namespace kzs {

kz::Status read_git_head(const std::string& tree_dir, std::string* commit);

}  // namespace kzs

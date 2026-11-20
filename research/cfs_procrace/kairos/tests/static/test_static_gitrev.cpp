// Reading the commit of a tree from the repository files: loose ref, packed ref, detached HEAD,
// a `.git` file that points elsewhere (a submodule), and the cases that must fail.
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "gitrev.hpp"
#include "support/kz_test.hpp"

using namespace kzs;
namespace fs = std::filesystem;

namespace {

void write(const fs::path& p, const std::string& text) {
  fs::create_directories(p.parent_path());
  std::ofstream(p) << text;
}

const char* kA = "0123456789abcdef0123456789abcdef01234567";
const char* kB = "fedcba9876543210fedcba9876543210fedcba98";

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / ("kz_static_gitrev_" + std::to_string(static_cast<long>(::getpid())));
  fs::remove_all(root);
  std::string id;

  // a loose ref
  write(root / "loose/.git/HEAD", "ref: refs/heads/main\n");
  write(root / "loose/.git/refs/heads/main", std::string(kA) + "\n");
  KZ_CHECK_OK(read_git_head((root / "loose").string(), &id));
  KZ_CHECK_EQ(id, std::string(kA));

  // a packed ref (no loose file)
  write(root / "packed/.git/HEAD", "ref: refs/heads/dev\n");
  write(root / "packed/.git/packed-refs", std::string("# pack-refs with: peeled\n") + kB + " refs/heads/other\n" + kA + " refs/heads/dev\n^" + kB + "\n");
  KZ_CHECK_OK(read_git_head((root / "packed").string(), &id));
  KZ_CHECK_EQ(id, std::string(kA));

  // a detached HEAD
  write(root / "detached/.git/HEAD", std::string(kB) + "\n");
  KZ_CHECK_OK(read_git_head((root / "detached").string(), &id));
  KZ_CHECK_EQ(id, std::string(kB));

  // a submodule: `.git` is a file that points at the repository, which keeps its refs in a common directory
  write(root / "super/.git/modules/sub/HEAD", "ref: refs/heads/main\n");
  write(root / "super/.git/modules/sub/commondir", "../common\n");
  write(root / "super/.git/modules/common/refs/heads/main", std::string(kB) + "\n");
  write(root / "super/sub/.git", "gitdir: ../.git/modules/sub\n");
  KZ_CHECK_OK(read_git_head((root / "super/sub").string(), &id));
  KZ_CHECK_EQ(id, std::string(kB));

  // failures
  KZ_CHECK(!read_git_head((root / "nothing").string(), &id).ok());
  write(root / "bad/.git/HEAD", "something odd\n");
  KZ_CHECK(!read_git_head((root / "bad").string(), &id).ok());
  write(root / "unresolved/.git/HEAD", "ref: refs/heads/gone\n");
  KZ_CHECK(!read_git_head((root / "unresolved").string(), &id).ok());
  write(root / "notgitdir/.git", "nonsense\n");
  KZ_CHECK(!read_git_head((root / "notgitdir").string(), &id).ok());
  // a ref file that holds no commit id is not accepted
  write(root / "junk/.git/HEAD", "ref: refs/heads/main\n");
  write(root / "junk/.git/refs/heads/main", "not a commit\n");
  KZ_CHECK(!read_git_head((root / "junk").string(), &id).ok());

  fs::remove_all(root);
  return kztest::finish("test_static_gitrev");
}

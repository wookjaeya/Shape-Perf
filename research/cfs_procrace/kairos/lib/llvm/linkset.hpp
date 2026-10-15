// linkset.hpp -- which object files a link command puts into the program (SPEC 4.1 step 1).
//
// "Target membership comes from link.txt, not from paths": the closed program is what the link
// command of the target takes in.  Objects named on the command are in.  An archive that follows
// `--whole-archive` is in as a whole.  An archive without it contributes only the members that
// define a symbol that is undefined at that point (the rule of GNU ld, applied to the archive
// again until nothing more is pulled).  The members of an archive are read from the command that
// built it (`ar qc lib.a objects...` in the link.txt of the archive's target) and their symbols
// from the object files themselves.
//
// Libraries given as -l options are not part of the program under analysis and are ignored.
#pragma once

#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kzs {

struct LinkInput {
  enum Kind { Object, Archive } kind = Object;
  bool whole = false;     // inside --whole-archive
  std::string path;       // absolute, normalized
  std::string as_written;
};

struct LinkPlan {
  std::string cwd;                    // the directory the command ran in
  std::vector<LinkInput> inputs;      // in command-line order
  std::vector<std::string> ignored;   // -l options and other flags, for the record
};

// Parses a link.txt (or the text of one).  `cwd` is the directory the command ran in; for a
// CMake link.txt that is the directory above CMakeFiles.
kz::Status parse_link_command(const std::string& text, const std::string& cwd, LinkPlan* out);

// Splits a command line the way a POSIX shell does for the cases a build system writes
// (double and single quotes, backslash escapes).
std::vector<std::string> split_command(const std::string& line);
// The inverse: tokens that contain a blank, a quote or a backslash are single-quoted (a single
// quote inside becomes '\'' ), so split_command(join_command(t)) == t.
std::string join_command(const std::vector<std::string>& tokens);

struct LinkMember {
  std::string object;    // absolute path of the object file
  std::string archive;   // absolute path of the archive, empty for an object named directly
  enum Mode { Named, Whole, Pulled, NotPulled } mode = Named;
  std::string pulled_for;  // Pulled: a symbol it defines that was needed
};

struct LinkSet {
  std::vector<LinkMember> members;  // in link order; NotPulled members are listed but not in the program
  size_t archives = 0;
  size_t included() const;
};

// Resolves the plan against the files on disk.  Reads the archives' member lists from the
// `ar` commands under the archive's directory and the symbols of the member objects.
kz::Status resolve_link_set(const LinkPlan& plan, LinkSet* out);

// The members listed in the `ar` command that builds `archive` (found among the CMake link
// files under its directory); paths are absolute.
kz::Status archive_members(const std::string& archive, std::vector<std::string>* objects);

std::string normalize_path(const std::string& path);

}  // namespace kzs

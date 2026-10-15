// recipe.hpp -- the basis recipe: the framework-specific data that `kz-static basis` needs.
//
// The basis tool is generic; what is specific to a build lives in a recipe, a small declarative
// file (extension .kzb) kept next to the profile.  A recipe says where the link command of the
// target is, which generated files and tables to record, and how the startup script is chosen.
// The recipe is hashed into the basis, and every item it adds to the basis is marked with where
// it came from.
//
//   kzb 1;
//   name "text";
//   link "path/to/link.txt";                    # relative to the build directory
//   binary "path/to/built/target";
//   generated "role" "path/to/file";            # recorded by hash
//   table "SYMBOL" in "path/to/generated.c";    # the initializer of a global table is recorded
//   declared "name" "value" authored "text";    # a hand-entered fact (tier researcher)
//   startup_script {
//     reset POWERON;                            # the reset type the basis is evaluated for
//     rule PROCESSOR volatile nonvolatile;      # files tried in order for a reset type
//     rule POWERON nonvolatile;
//     file volatile const "NAME" in "src.c";    # the file name is a build constant, read in src.c
//     mount "/mount/point" "dir/relative/to/build";
//     entry library "WORD"; entry app "WORD";   # the first field of a script line
//     cite "path/in/bundle.c" 130 168 note "text";
//   }
//
// Comments run from '#' to the end of the line.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kzs {

struct Recipe {
  std::string name;
  std::string link;
  std::string binary;
  struct Generated {
    std::string role, path;
  };
  std::vector<Generated> generated;
  struct Table {
    std::string symbol, file;
  };
  std::vector<Table> tables;
  struct Declared {
    std::string name, value, authored;
  };
  std::vector<Declared> declared;
  struct Startup {
    bool present = false;
    std::string reset;
    struct Rule {
      std::string reset;
      std::vector<std::string> order;
    };
    std::vector<Rule> rules;
    struct File {
      std::string role, constant, source;
    };
    std::vector<File> files;
    struct Mount {
      std::string point, dir;
    };
    std::vector<Mount> mounts;
    std::string library_word, app_word;
    struct Cite {
      std::string file, note;
      uint32_t from = 0, to = 0;
    };
    std::vector<Cite> cites;
  } startup;
};

kz::Status parse_recipe(const std::string& text, Recipe* out);

}  // namespace kzs

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
//   module "NAME" link "path/to/link.txt";      # a unit the startup script loads at run time
//   startup_script {
//     reset POWERON;                            # the reset type the basis is evaluated for
//     rule PROCESSOR volatile nonvolatile;      # files tried in order for a reset type
//     rule POWERON nonvolatile;
//     file volatile const "NAME" in "src.c";    # the file name is a build constant, read in src.c
//     mount "/mount/point" "dir/relative/to/build";
//     entry library "WORD" global;              # the first field of a script line, and the
//     entry app "WORD" local;                   #   symbol scope the loader gives that kind
//     module_file field 1 path const "MOUNT" extension const "EXT" in "src.c";
//                                               # how a script line names the file it loads
//     cite "path/in/bundle.c" 130 168 note "text";
//   }
//
// A `module` names the link command that builds a dynamically loaded unit; NAME is the file-name
// field of the script line that loads it.  The basis links such a unit with the executable (SPEC
// 4.1 step 6) only when the selected script loads it; the scope words and `module_file` are then
// required, and so is `binary` (the executable's exported symbols are read from it).
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
  struct Module {
    std::string name, link;
  };
  std::vector<Module> modules;
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
    std::string library_scope, app_scope;  // "global", "local", or empty when the recipe does not say
    struct ModuleFile {
      bool present = false;
      uint32_t field = 0;  // the field of a script line that names the file (0 is the entry word)
      std::string path_const, ext_const, source;
    } module_file;
    struct Cite {
      std::string file, note;
      uint32_t from = 0, to = 0;
    };
    std::vector<Cite> cites;
  } startup;
};

kz::Status parse_recipe(const std::string& text, Recipe* out);

}  // namespace kzs

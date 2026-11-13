// commands.hpp -- the entry points of the kz-static commands.  Each returns the exit status.
#pragma once

#include "cli.hpp"
#include "profile_load.hpp"

namespace kzs_cli {

int cmd_basis(int argc, char** argv);
int cmd_extract(int argc, char** argv);
int cmd_show(int argc, char** argv);
int cmd_reach(int argc, char** argv);
int cmd_relations(int argc, char** argv);

}  // namespace kzs_cli

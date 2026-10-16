# kz: the command-line front end of the core.  Libraries only from the core side (no LLVM).
# Every cmd_<name>.cpp is one sub-command (command.hpp); a command that needs another library
# appends it to KZ_CLI_LIBS.
file(GLOB kz_cli_sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "cmd_*.cpp")
list(SORT kz_cli_sources)
set(KZ_CLI_LIBS kz-loader kz-plan kz-anchor kz-trace kz-core)
add_executable(kz main.cpp ${kz_cli_sources})
target_include_directories(kz PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(kz PRIVATE ${KZ_CLI_LIBS})
kz_apply_settings(kz)
kz_tool_identity(kz)  # after target_link_libraries: defines KZ_TOOL_SOURCE_SHA256

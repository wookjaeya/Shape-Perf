# kz-phi: parse, check, print, hash and verify a profile (.kzp).  libkz-core only.
add_executable(kz-phi main.cpp)
target_link_libraries(kz-phi PRIVATE kz-core)
kz_apply_settings(kz-phi)
kz_tool_identity(kz-phi)  # after target_link_libraries: defines KZ_TOOL_SOURCE_SHA256

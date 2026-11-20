# kz-dump: canonical text dump of a KZ container (the stand-alone `kz dump`).  libkz-core only.
add_executable(kz-dump main.cpp)
target_link_libraries(kz-dump PRIVATE kz-core)
kz_apply_settings(kz-dump)
kz_tool_identity(kz-dump)  # after target_link_libraries: defines KZ_TOOL_SOURCE_SHA256

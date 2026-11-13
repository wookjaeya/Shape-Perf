# Helper of check_source_identity.cmake: must fail because the path does not exist.
include("${KZ_MODULE}")
kz_source_identity(x ROOT "${ROOT}" PATHS no-such-directory)

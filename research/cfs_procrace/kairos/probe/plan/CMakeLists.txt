# The plan: the observation part of a profile, and the basis container that holds its anchors.
add_library(kz-plan STATIC
  basis.cpp
  profile.cpp)
target_include_directories(kz-plan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")
target_compile_features(kz-plan PUBLIC cxx_std_17)
target_compile_options(kz-plan PRIVATE -fno-exceptions)
target_link_libraries(kz-plan PUBLIC kz-anchor kz-trace kz-core)
kz_apply_settings(kz-plan)

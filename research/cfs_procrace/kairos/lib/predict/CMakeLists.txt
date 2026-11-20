# libkz-predict: the bounded explicit-state prediction of `kz predict` (SPEC 4.9 rev 1.3 "Engine"; P-Z3: no solver).
# Standard library and libkz-core only; no LLVM; no exceptions.  Framework knowledge is in the model files under
# profiles/, never here.
add_library(kz-predict STATIC
  explore.cpp
  model.cpp
  predict.cpp)
target_include_directories(kz-predict PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_compile_features(kz-predict PUBLIC cxx_std_17)
target_compile_options(kz-predict PRIVATE -fno-exceptions)
target_link_libraries(kz-predict PUBLIC kz-core)
kz_apply_settings(kz-predict)
kz_declare_sources(kz-predict PATHS include/kz/predict)

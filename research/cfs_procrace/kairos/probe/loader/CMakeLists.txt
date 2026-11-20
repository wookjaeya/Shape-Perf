# The loader: perf_event_open uprobes (perf-sample backend), the drain, the run of the target, the
# /proc capture of the conditions, and the run container.  Linux only; no library beyond libc and
# pthreads (P-LIBBPF: the existing environment).
find_package(Threads REQUIRED)
add_library(kz-loader STATIC
  perf_session.cpp
  proc_capture.cpp
  run_container.cpp
  run_record.cpp
  run_target.cpp)
target_include_directories(kz-loader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")
target_compile_features(kz-loader PUBLIC cxx_std_17)
target_compile_options(kz-loader PRIVATE -fno-exceptions)
target_link_libraries(kz-loader PUBLIC kz-plan kz-trace kz-core Threads::Threads)
kz_apply_settings(kz-loader)

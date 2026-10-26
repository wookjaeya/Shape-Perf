# Kairos instrumentation runtime (hook ABI 1, dump format 1.0): kairos/docs/event_semantics.md.
#
# Targets
#   kzrt           static C99 core: hooks, bootstrap and per-actor buffers, control (kzp-hold 1),
#                  serialization.  No library calls; built -ffreestanding.  Link it once per process
#                  (into the executable; loaded modules reach it through the dynamic symbol table).
#   kzrt-posix     static POSIX adapter (Linux host): TLS cell, gettid, CLOCK_MONOTONIC, pthread wait.
#   kzrt-posix-auto  optional object: init + attach before main from KZ_RT_* environment names, dump at exit.
#   kzrt-osal      static OSAL adapter skeleton; only with -DKZ_RT_OSAL_INCLUDE_DIRS="<osal>/src/os/inc;<build>/osal_public_api/inc"
#   kzrt-decode    host reader of the dump format (C), kzrt-manifest host parser of "kzs-sites 1" (C++)
#   kz-rt-dump     host tool: prints a dump and checks it against manifests
# None of them is linked into kz.  The runtime builds inside the Kairos tree (added by the top-level
# CMakeLists.txt) or on its own:  cmake -S kairos/runtime -B <dir> && cmake --build <dir> && ctest --test-dir <dir>
cmake_minimum_required(VERSION 3.20)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(kzrt VERSION 0.1.0 LANGUAGES C CXX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  enable_testing()
endif()

set(KZ_RT_OSAL_INCLUDE_DIRS "" CACHE STRING "OSAL public include dirs (osapi.h and the generated osconfig.h) for kzrt-osal; empty = not built")

function(kzrt_settings target)
  if(COMMAND kz_apply_settings)
    kz_apply_settings(${target})
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wundef -Wcast-qual
      "$<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes;-Wmissing-prototypes>")
  endif()
endfunction()

# Target-side libraries: C99, position independent (the cFS core executable is a PIE).
add_library(kzrt STATIC src/kz_rt.c)
target_include_directories(kzrt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(kzrt PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
# -ffreestanding: no hosted library assumptions; no stack protector and no loop-to-memcpy rewriting,
# so that the object references no library symbol (checked by the test rt_no_library_symbols).
target_compile_options(kzrt PRIVATE -ffreestanding -fno-stack-protector
  "$<$<C_COMPILER_ID:GNU>:-fno-tree-loop-distribute-patterns>")
kzrt_settings(kzrt)

add_library(kzrt-posix STATIC src/kz_rt_posix.c)
target_link_libraries(kzrt-posix PUBLIC kzrt)
find_package(Threads REQUIRED)
target_link_libraries(kzrt-posix PUBLIC Threads::Threads)
set_target_properties(kzrt-posix PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
kzrt_settings(kzrt-posix)

# Optional start-up object (constructor: init + POSIX attach from KZ_RT_* environment names, dump at
# exit) for a target whose sources are not edited.  Link its object: target_link_libraries(<exe> PRIVATE kzrt-posix-auto).
add_library(kzrt-posix-auto OBJECT src/kz_rt_posix_auto.c)
target_link_libraries(kzrt-posix-auto PUBLIC kzrt-posix)
set_target_properties(kzrt-posix-auto PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
kzrt_settings(kzrt-posix-auto)

if(KZ_RT_OSAL_INCLUDE_DIRS)
  add_library(kzrt-osal STATIC src/kz_rt_osal.c)
  target_link_libraries(kzrt-osal PUBLIC kzrt)
  target_include_directories(kzrt-osal PRIVATE ${KZ_RT_OSAL_INCLUDE_DIRS})
  set_target_properties(kzrt-osal PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
  kzrt_settings(kzrt-osal)
endif()

# Host-side reader, manifest parser and tool.
add_library(kzrt-decode STATIC tools/decode/kz_rt_decode.c)
target_include_directories(kzrt-decode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/tools/decode)
set_target_properties(kzrt-decode PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
kzrt_settings(kzrt-decode)

add_library(kzrt-manifest STATIC tools/manifest/kz_rt_manifest.cpp)
target_include_directories(kzrt-manifest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/tools/manifest)
kzrt_settings(kzrt-manifest)

add_executable(kz-rt-dump tools/kz-rt-dump/kz_rt_dump.cpp)
target_link_libraries(kz-rt-dump PRIVATE kzrt-decode kzrt-manifest)
kzrt_settings(kz-rt-dump)

add_subdirectory(tests)

# Kairos instrumentation runtime (kzrt 0.2.0: hook ABI 1, dump format 1.1): kairos/docs/event_semantics.md.
#
# Targets
#   kzrt           static C99 core: hooks, bootstrap buffer and the actors' chunk pool, control (kzp-hold 1),
#                  serialization.  No library calls; built -ffreestanding.  Link it once per process
#                  (into the executable; loaded modules reach it through the dynamic symbol table).
#   kzrt-posix     static POSIX adapter (Linux host): TLS cell, gettid, CLOCK_MONOTONIC, pthread wait.
#   kzrt-posix-auto  optional object: the mode from KZ_RT_MODE before any constructor (.preinit_array), init +
#                  attach before main from KZ_RT_* environment names, dump at exit.
#   kzrt-osal      static OSAL adapter skeleton; only with -DKZ_RT_OSAL_INCLUDE_DIRS="<osal>/src/os/inc;<build>/osal_public_api/inc"
#   kzrt-decode    host reader of the dump format (C), kzrt-manifest host parser of "kzs-sites 1" (C++)
#   kz-rt-dump     host tool: prints a dump and checks it against manifests
#   kzrt-build-info  OBJECT library (only when KZ_RT_BUILD_BACKEND is set): the build identity of an instrumented
#                  build, the fixed source src/kz_rt_build_info.c compiled with the KZ_RT_BUILD_* values below
# Build parameters of the target-side libraries (tool capacities, recorded in every dump): -DKZ_RT_POOL_BYTES,
# -DKZ_RT_CHUNK_BYTES, -DKZ_RT_MAX_ACTORS, ... through KZ_RT_DEFINES below (kz_rt.h "static sizes").
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
set(KZ_RT_DEFINES "" CACHE STRING "Size macros for the target-side libraries, e.g. KZ_RT_POOL_BYTES=8388608 (tool capacities, recorded in the dump)")
get_filename_component(KZ_RT_KAIROS "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
# The build identity (kz_rt.h, event_semantics.md 8.5): given by the instrumented build, recorded as given.
set(KZ_RT_BUILD_BACKEND "" CACHE STRING "build identity: the compiler binding, e.g. \"kzgcc 0.1 (gcc-plugin)\" (empty = no kzrt-build-info target)")
set(KZ_RT_BUILD_COMPILER "" CACHE STRING "build identity: the target compiler, e.g. the first line of its --version (empty = its __VERSION__)")
set(KZ_RT_BUILD_TARGET "" CACHE STRING "build identity: the target, e.g. the compiler's -dumpmachine (empty = not recorded)")
set(KZ_RT_BUILD_MANIFEST "" CACHE FILEPATH "build identity: the site manifest file the compiler binding read (its sha256 is recorded)")

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
if(KZ_RT_DEFINES)
  target_compile_definitions(kzrt PUBLIC ${KZ_RT_DEFINES})
endif()
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

# The host tools hash manifests with the Kairos SHA-256, compiled from its own source (no copy), so that
# the runtime still builds on its own.
add_executable(kz-rt-dump tools/kz-rt-dump/kz_rt_dump.cpp ${KZ_RT_KAIROS}/lib/core/sha256.cpp)
target_include_directories(kz-rt-dump PRIVATE ${KZ_RT_KAIROS}/include)
target_link_libraries(kz-rt-dump PRIVATE kzrt-decode kzrt-manifest)
kzrt_settings(kz-rt-dump)

# kzrt_build_info(<target> BACKEND <text> [COMPILER <text>] [TARGET <text>] [MANIFEST <file>]): an OBJECT
# library of src/kz_rt_build_info.c with these values (the manifest's sha256 is taken at configure time).
function(kzrt_build_info name)
  cmake_parse_arguments(B "" "BACKEND;COMPILER;TARGET;MANIFEST" "" ${ARGN})
  add_library(${name} OBJECT ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/kz_rt_build_info.c)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include)
  set_target_properties(${name} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(${name} PRIVATE "KZ_RT_BUILD_BACKEND=\"${B_BACKEND}\"")
  if(B_COMPILER)
    target_compile_definitions(${name} PRIVATE "KZ_RT_BUILD_COMPILER=\"${B_COMPILER}\"")
  endif()
  if(B_TARGET)
    target_compile_definitions(${name} PRIVATE "KZ_RT_BUILD_TARGET=\"${B_TARGET}\"")
  endif()
  if(B_MANIFEST)
    file(SHA256 ${B_MANIFEST} sha)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${B_MANIFEST})
    target_compile_definitions(${name} PRIVATE "KZ_RT_BUILD_MANIFEST_SHA256=\"${sha}\"")
  endif()
  kzrt_settings(${name})
endfunction()
if(KZ_RT_BUILD_BACKEND)
  kzrt_build_info(kzrt-build-info BACKEND "${KZ_RT_BUILD_BACKEND}" COMPILER "${KZ_RT_BUILD_COMPILER}" TARGET "${KZ_RT_BUILD_TARGET}"
                  MANIFEST "${KZ_RT_BUILD_MANIFEST}")
endif()

add_subdirectory(tests)

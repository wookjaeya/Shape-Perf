# kzllvm -- LLVM new-pass-manager pass plugin, compiler binding candidate of the Kairos instrumentation
# family (stage C fitness prototype; kairos/docs/instr_candidate_llvm.md).  Standalone project, not part
# of the Kairos top-level build:
#
#   cmake -S kairos/instrument/llvm -B <dir> && cmake --build <dir> && ctest --test-dir <dir>
#
# Built against the pinned LLVM build (KZ_LLVM_BUILD_DIR, the same tree the Kairos analysis tools use) and
# loaded only into that build's clang (-fpass-plugin=).  The plugin does not link LLVM: its LLVM symbols
# resolve against the clang executable, which exports them (LLVM_ENABLE_PLUGINS=ON).  It compiles the
# runtime's manifest parser and the Kairos SHA-256 from their own sources (no copies).  The unit tests reuse
# the GCC candidate's test target and manifests (instrument/gcc/tests) so that both candidates are checked on
# the same program.
cmake_minimum_required(VERSION 3.20)
project(kzllvm VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

get_filename_component(KZ_KAIROS "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(KZ_LLVM_BUILD_DIR "/home/user/work/llvm-project/build" CACHE PATH "the pinned LLVM/Clang build whose clang loads the plugin")
find_package(LLVM REQUIRED CONFIG PATHS "${KZ_LLVM_BUILD_DIR}/lib/cmake/llvm" NO_DEFAULT_PATH)
set(KZLLVM_CLANG "${LLVM_TOOLS_BINARY_DIR}/clang" CACHE FILEPATH "the clang that loads the plugin")
message(STATUS "kzllvm: LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}; clang ${KZLLVM_CLANG}")

add_library(kzllvm MODULE kzllvm.cpp
  ${KZ_KAIROS}/runtime/tools/manifest/kz_rt_manifest.cpp
  ${KZ_KAIROS}/lib/core/sha256.cpp)
set_target_properties(kzllvm PROPERTIES PREFIX "" POSITION_INDEPENDENT_CODE ON)
target_include_directories(kzllvm SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(kzllvm PRIVATE ${KZ_KAIROS}/runtime/include ${KZ_KAIROS}/runtime/tools/manifest ${KZ_KAIROS}/include)
separate_arguments(KZLLVM_LLVM_DEFS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(kzllvm PRIVATE ${KZLLVM_LLVM_DEFS} -Wall -Wno-unused-parameter)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(kzllvm PRIVATE -fno-rtti)
endif()

# The runtime the tests link: built on its own from kairos/runtime (event_semantics.md 8.7), never with the
# plugin, with the host gcc as for the GCC candidate (the same runtime for every candidate).
include(ExternalProject)
ExternalProject_Add(kzrt_standalone SOURCE_DIR ${KZ_KAIROS}/runtime BINARY_DIR ${CMAKE_BINARY_DIR}/kzrt
  CMAKE_ARGS -DCMAKE_C_COMPILER=/usr/bin/gcc INSTALL_COMMAND "" BUILD_ALWAYS ON)

# Host tool of the GCC candidate (events of a dump with native keys and pcs), built from its source.
add_executable(kz-events ${KZ_KAIROS}/instrument/gcc/tools/kzgcc_events.c ${KZ_KAIROS}/runtime/tools/decode/kz_rt_decode.c)
target_include_directories(kz-events PRIVATE ${KZ_KAIROS}/runtime/include ${KZ_KAIROS}/runtime/tools/decode)
set_target_properties(kz-events PROPERTIES C_STANDARD 99)

add_subdirectory(tests)

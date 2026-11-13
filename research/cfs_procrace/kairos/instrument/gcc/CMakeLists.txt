# kzgcc -- GCC GIMPLE plugin, compiler binding candidate of the Kairos instrumentation family (stage C
# fitness prototype; kairos/docs/instr_candidate_gcc.md).  Standalone project, not part of the Kairos
# top-level build (its component glob does not include instrument/):
#
#   cmake -S kairos/instrument/gcc -B <dir> && cmake --build <dir> && ctest --test-dir <dir>
#
# Needs the plugin headers of the GCC that will load it (distro package gcc-13-plugin-dev for the
# reference gcc 13.3.0); the plugin accepts only that GCC (plugin_default_version_check).  The plugin is
# built with that GCC's g++ and without RTTI, as GCC itself.  It compiles the runtime's manifest parser
# and the Kairos SHA-256 from their own sources (no copies).
cmake_minimum_required(VERSION 3.20)
project(kzgcc VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

get_filename_component(KZ_KAIROS "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(KZGCC_GCC "/usr/bin/gcc" CACHE FILEPATH "the GCC that will load the plugin (its -print-file-name=plugin)")
execute_process(COMMAND ${KZGCC_GCC} -print-file-name=plugin OUTPUT_VARIABLE KZGCC_PLUGIN_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT EXISTS "${KZGCC_PLUGIN_DIR}/include/gcc-plugin.h")
  message(FATAL_ERROR "no GCC plugin headers in ${KZGCC_PLUGIN_DIR}/include (install the matching gcc-<N>-plugin-dev)")
endif()

add_library(kzgcc MODULE kzgcc.cpp
  ${KZ_KAIROS}/runtime/tools/manifest/kz_rt_manifest.cpp
  ${KZ_KAIROS}/lib/core/sha256.cpp)
set_target_properties(kzgcc PROPERTIES PREFIX "" POSITION_INDEPENDENT_CODE ON)
target_include_directories(kzgcc PRIVATE ${KZGCC_PLUGIN_DIR}/include ${KZ_KAIROS}/runtime/include ${KZ_KAIROS}/runtime/tools/manifest
  ${KZ_KAIROS}/include)
target_compile_options(kzgcc PRIVATE -fno-rtti -Wall -Wno-unused-parameter)

# The runtime the tests link: built on its own from kairos/runtime (event_semantics.md 8.7, "cmake -S
# kairos/runtime -B <dir>"), never with the plugin.
include(ExternalProject)
ExternalProject_Add(kzrt_standalone SOURCE_DIR ${KZ_KAIROS}/runtime BINARY_DIR ${CMAKE_BINARY_DIR}/kzrt
  CMAKE_ARGS -DCMAKE_C_COMPILER=${KZGCC_GCC} INSTALL_COMMAND "" BUILD_ALWAYS ON)

add_subdirectory(tests)

# Host tool: the events of a dump with native keys and pcs (tab separated), for the comparison scripts.
add_executable(kzgcc-events tools/kzgcc_events.c ${KZ_KAIROS}/runtime/tools/decode/kz_rt_decode.c)
target_include_directories(kzgcc-events PRIVATE ${KZ_KAIROS}/runtime/include ${KZ_KAIROS}/runtime/tools/decode)
set_target_properties(kzgcc-events PROPERTIES C_STANDARD 99)

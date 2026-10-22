# Kairos (prefix kz): a detector for process-level readiness and order violations.
#
# Build trees live OUTSIDE the repository, under /home/user/work/kairos-build/, e.g.
#   cmake -S . -B /home/user/work/kairos-build/gcc -G Ninja
#   cmake --build /home/user/work/kairos-build/gcc && ctest --test-dir /home/user/work/kairos-build/gcc
#
# libkz-core, tools/kz and lib/proofcheck never use LLVM.  The tools that do (lib/llvm,
# tools/kz-static) are built only with -DKZ_BUILD_LLVM_TOOLS=ON, against the pinned LLVM
# build, and only those targets get LLVM include paths (kz_use_llvm()).
cmake_minimum_required(VERSION 3.20)
project(kairos VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(KZ_WERROR "Treat warnings in Kairos code as errors" ON)
option(KZ_BUILD_LLVM_TOOLS "Build the analysis tools that link LLVM/Clang (lib/llvm, tools/kz-static)" OFF)
set(KZ_LLVM_BUILD_DIR "/home/user/work/llvm-project/build" CACHE PATH
    "Pinned LLVM/Clang build tree used by the analysis tools (never by libkz-core, tools/kz or lib/proofcheck)")
set(KZ_SANITIZE "" CACHE STRING "Sanitizers for all targets, e.g. address,undefined")
option(KZ_SANITIZE_TRAP "Make undefined-behaviour checks trap instead of calling the runtime (for a clang without compiler-rt: the system clang 18 here has none)" OFF)
set(KZ_TOOL_VERSION "${PROJECT_VERSION}-S0" CACHE STRING "Version string folded into the tool hash")

include(cmake/KzSettings.cmake)
include(cmake/KzSourceIdentity.cmake)

if(KZ_BUILD_LLVM_TOOLS)
  find_package(LLVM REQUIRED CONFIG PATHS "${KZ_LLVM_BUILD_DIR}/lib/cmake/llvm" NO_DEFAULT_PATH)
  find_package(Clang REQUIRED CONFIG PATHS "${KZ_LLVM_BUILD_DIR}/lib/cmake/clang" NO_DEFAULT_PATH)
  message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}; Clang at ${Clang_DIR}")
endif()

enable_testing()

# Every component directory brings its own CMakeLists.txt; the top level adds the ones that
# exist.  lib/core comes first because every other component links it.
add_subdirectory(lib/core)

set(KZ_LLVM_COMPONENT_DIRS lib/llvm tools/kz-static)
file(GLOB kz_component_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS
     "lib/*/CMakeLists.txt" "probe/*/CMakeLists.txt" "tools/*/CMakeLists.txt" "gate/CMakeLists.txt")
list(SORT kz_component_files)
foreach(kz_file IN LISTS kz_component_files)
  get_filename_component(kz_dir "${kz_file}" DIRECTORY)
  if(kz_dir STREQUAL "lib/core")
    continue()
  endif()
  list(FIND KZ_LLVM_COMPONENT_DIRS "${kz_dir}" kz_llvm_index)
  if(NOT kz_llvm_index EQUAL -1 AND NOT KZ_BUILD_LLVM_TOOLS)
    message(STATUS "Skipping ${kz_dir}: it needs LLVM (configure with -DKZ_BUILD_LLVM_TOOLS=ON)")
    continue()
  endif()
  add_subdirectory("${kz_dir}")
endforeach()

add_subdirectory(tests)

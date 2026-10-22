# KzSettings.cmake -- compiler settings and helpers shared by all Kairos targets.

# Warnings.  Kairos code is built with warnings as errors (KZ_WERROR) on gcc 13 and clang 18.
set(KZ_WARNINGS_COMMON -Wall -Wextra -Wpedantic -Wshadow -Wundef -Wformat=2 -Wcast-qual)
set(KZ_WARNINGS_CXX ${KZ_WARNINGS_COMMON} -Wnon-virtual-dtor -Woverloaded-virtual -Wimplicit-fallthrough)
set(KZ_WARNINGS_C ${KZ_WARNINGS_COMMON} -Wstrict-prototypes -Wmissing-prototypes)
if(KZ_WERROR)
  list(APPEND KZ_WARNINGS_CXX -Werror)
  list(APPEND KZ_WARNINGS_C -Werror)
endif()

# Sanitizers apply to every target, so that all objects and libraries agree.
if(KZ_SANITIZE)
  add_compile_options(-fsanitize=${KZ_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${KZ_SANITIZE})
  if(KZ_SANITIZE_TRAP)
    add_compile_options(-fsanitize-trap=undefined)
    add_link_options(-fsanitize-trap=undefined)
  endif()
endif()

# kz_apply_settings(<target>): warnings (language specific) for a Kairos target.
function(kz_apply_settings target)
  target_compile_options(${target} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${KZ_WARNINGS_CXX}>"
    "$<$<COMPILE_LANGUAGE:C>:${KZ_WARNINGS_C}>")
endfunction()

# kz_add_unit_test(<name> SOURCES <files...> [LIBS <libs...>] [ARGS <args...>] [LABEL <label>])
# Builds an executable and registers it with ctest.  Tests are plain programs that exit
# non-zero on failure (see tests/unit/support/kz_test.hpp).
function(kz_add_unit_test name)
  cmake_parse_arguments(T "" "LABEL" "SOURCES;LIBS;ARGS" ${ARGN})
  if(NOT T_LABEL)
    set(T_LABEL unit)
  endif()
  add_executable(${name} ${T_SOURCES})
  target_link_libraries(${name} PRIVATE ${T_LIBS})
  kz_apply_settings(${name})
  add_test(NAME ${name} COMMAND ${name} ${T_ARGS})
  set_tests_properties(${name} PROPERTIES LABELS ${T_LABEL})
endfunction()

# kz_use_llvm(<target>): the ONLY way LLVM reaches a target.  Applies the include paths,
# definitions and the exception / RTTI settings of the pinned LLVM build.  Allowed only in
# lib/llvm and tools/kz-static: lint_no-llvm rejects the name in any other CMake file of the
# tree (and rejects every LLVM, Clang and MLIR name in the CMake files and headers of libkz-core,
# tools/kz, tools/kz-dump, lib/proofcheck, include/ and the core tests).
function(kz_use_llvm target)
  if(NOT KZ_BUILD_LLVM_TOOLS)
    message(FATAL_ERROR "kz_use_llvm(${target}) needs -DKZ_BUILD_LLVM_TOOLS=ON")
  endif()
  target_include_directories(${target} SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
  separate_arguments(kz_llvm_defs NATIVE_COMMAND "${LLVM_DEFINITIONS}")
  target_compile_options(${target} PRIVATE ${kz_llvm_defs})
  if(NOT LLVM_ENABLE_EH)
    target_compile_options(${target} PRIVATE -fno-exceptions)
  endif()
  if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(${target} PRIVATE -fno-rtti)
  endif()
endfunction()

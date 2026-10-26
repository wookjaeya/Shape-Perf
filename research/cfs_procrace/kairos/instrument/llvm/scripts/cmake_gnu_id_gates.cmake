# cmake_gnu_id_gates.cmake -- translation T3 of the clang builds (build_c1.sh, build_r2.sh), included after every
# project() of the arch build through CMake's documented hook CMAKE_PROJECT_INCLUDE.  The arch builds of the
# references were configured with gcc, so the build system's own "compiler id is GNU" branches were taken there:
# C1 cfe/cmake/target/CMakeLists.txt L159-L164 (--whole-archive around the core libraries, without which the
# loaded apps miss core symbols: CF, BP, bplib, TO_LAB, SCH_LAB fail to load, see instr_candidate_llvm.md),
# bplib CMakeLists.txt L79 and L170, osal/src/bsp/generic-linux/CMakeLists.txt L42 (unit-test coverage).  The
# toolchain rules and flags CMake loaded for the detected compiler (Clang) are not touched: only the id that
# those branches and $<C_COMPILER_ID:...> read is the reference's.  The real id is kept in KZ_T3_DETECTED_C_ID.
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  set(KZ_T3_DETECTED_C_ID "${CMAKE_C_COMPILER_ID}")
  set(CMAKE_C_COMPILER_ID "GNU")
  message(STATUS "kz T3: C compiler id presented to the build system's gates as GNU (detected ${KZ_T3_DETECTED_C_ID})")
endif()

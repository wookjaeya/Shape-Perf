# The anchor table: the binary's identity, its function symbols and the file offsets of the probe
# targets, read with <elf.h> and a DWARF line-table reader.  No LLVM (see elf_image.hpp for why).
add_library(kz-anchor STATIC
  anchor_resolve.cpp
  dwarf_line.cpp
  elf_image.cpp)
target_include_directories(kz-anchor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")
target_compile_features(kz-anchor PUBLIC cxx_std_17)
target_compile_options(kz-anchor PRIVATE -fno-exceptions)
target_link_libraries(kz-anchor PUBLIC kz-core)
kz_apply_settings(kz-anchor)

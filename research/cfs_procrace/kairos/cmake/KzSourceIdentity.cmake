# KzSourceIdentity.cmake -- the source identity that goes into every tool hash.
#
# A source identity is the SHA-256 of the text
#     <path relative to the source root> <sha256 of the file>\n
# one line per file, sorted by path (byte order), over the regular files under the given
# paths.  Hidden files and directories (a name starting with '.') and backup files (ending in
# '~') are left out.  Paths are relative, so the identity does not depend on where the tree
# is checked out, and it does not depend on the compiler.  It is computed at configure time;
# every covered file is registered in CMAKE_CONFIGURE_DEPENDS, so editing a file re-runs the
# configure step and refreshes the identity (and therefore the tool hash) on the next build.
#
# This is a compile definition computed from the sources.  It is not generated code.
#
#   kz_source_identity(<var> PATHS <path>... [ROOT <dir>])
#       <var> := identity of the files under PATHS (files or directories, relative to ROOT,
#       default KZ_SOURCE_ROOT)
#   kz_declare_sources(<target> PATHS <path>...)
#       the paths (relative to the source root) that a library target is built from, besides
#       its own source directory, e.g. its public header directories
#   kz_tool_identity(<target>)
#       call after the target's target_link_libraries(): defines KZ_TOOL_SOURCE_SHA256 for
#       the target.  The identity covers the source directory and the declared paths of the
#       target and of every non-imported library in its link closure, and always include/,
#       cmake/ and the top-level CMakeLists.txt.  The paths are kept in the target property
#       KZ_TOOL_IDENTITY_PATHS (joined by ":").  Use KZ_THIS_TOOL_HASH(<name>) in the tool.

get_filename_component(KZ_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# What libkz-core is built from.  Public headers of the schema and of the core, its sources
# and its build recipe.
set(KZ_CORE_SOURCE_PATHS
    include/kz/core include/kz/schema lib/core
    cmake/KzSettings.cmake cmake/KzSourceIdentity.cmake CMakeLists.txt)

function(kz_source_identity out)
  cmake_parse_arguments(A "" "ROOT" "PATHS" ${ARGN})
  if(NOT A_ROOT)
    set(A_ROOT "${KZ_SOURCE_ROOT}")
  endif()
  if(CMAKE_SCRIPT_MODE_FILE)
    set(kz_depends "")
  else()
    set(kz_depends CONFIGURE_DEPENDS)
  endif()
  set(files "")
  foreach(p IN LISTS A_PATHS)
    if(IS_DIRECTORY "${A_ROOT}/${p}")
      file(GLOB_RECURSE found LIST_DIRECTORIES false RELATIVE "${A_ROOT}" ${kz_depends} "${A_ROOT}/${p}/*")
      list(APPEND files ${found})
    elseif(EXISTS "${A_ROOT}/${p}")
      list(APPEND files "${p}")
    else()
      message(FATAL_ERROR "kz_source_identity: ${p} does not exist under ${A_ROOT}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES files)
  list(FILTER files EXCLUDE REGEX "(^|/)\\.[^/]*")  # a hidden file or a file in a hidden directory
  list(FILTER files EXCLUDE REGEX "~$")             # editor backup files
  list(SORT files COMPARE STRING CASE SENSITIVE)
  set(text "")
  foreach(f IN LISTS files)
    file(SHA256 "${A_ROOT}/${f}" h)
    string(APPEND text "${f} ${h}\n")
    if(NOT CMAKE_SCRIPT_MODE_FILE)
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${A_ROOT}/${f}")
    endif()
  endforeach()
  string(SHA256 digest "${text}")
  set(${out} "${digest}" PARENT_SCOPE)
endfunction()

function(kz_declare_sources target)
  cmake_parse_arguments(A "" "" "PATHS" ${ARGN})
  set_property(TARGET ${target} PROPERTY KZ_SOURCE_PATHS "${A_PATHS}")
endfunction()

function(kz_tool_identity target)
  set(todo ${target})
  set(seen "")
  set(paths "")
  while(todo)
    list(POP_FRONT todo t)
    # $<LINK_ONLY:x> is how a static library records a private dependency.
    string(REGEX REPLACE "^\\$<LINK_ONLY:(.*)>$" "\\1" t "${t}")
    if("${t}" STREQUAL "" OR t IN_LIST seen OR NOT TARGET ${t})
      continue()
    endif()
    list(APPEND seen ${t})
    get_target_property(imported ${t} IMPORTED)
    if(imported)
      continue()
    endif()
    get_target_property(srcdir ${t} SOURCE_DIR)
    file(RELATIVE_PATH rel "${KZ_SOURCE_ROOT}" "${srcdir}")
    list(APPEND paths "${rel}")
    get_target_property(declared ${t} KZ_SOURCE_PATHS)
    if(declared)
      list(APPEND paths ${declared})
    endif()
    get_target_property(type ${t} TYPE)
    if(NOT type STREQUAL "INTERFACE_LIBRARY")
      get_target_property(deps ${t} LINK_LIBRARIES)
    else()
      get_target_property(deps ${t} INTERFACE_LINK_LIBRARIES)
    endif()
    if(deps)
      list(APPEND todo ${deps})
    endif()
  endwhile()
  list(APPEND paths include cmake CMakeLists.txt)
  list(REMOVE_DUPLICATES paths)
  kz_source_identity(id PATHS ${paths})
  list(JOIN paths ":" joined)
  set_property(TARGET ${target} PROPERTY KZ_TOOL_IDENTITY_PATHS "${joined}")
  target_compile_definitions(${target} PRIVATE "KZ_TOOL_SOURCE_SHA256=\"${id}\"")
endfunction()

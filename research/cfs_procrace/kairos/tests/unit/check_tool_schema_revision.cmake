# check_tool_schema_revision.cmake -- every tool of one build reports the same schema revision and fingerprint.
#
#   cmake -DDUMP=<kz-dump> -DTOOLS="<tool>|<tool>|..." -P check_tool_schema_revision.cmake
#
# The layout that kz-dump prints (`kz-dump --layout`: first line "kz-layout abi=<n> revision=<r>", last line
# "fingerprint=<hex>") is the reference.  Each tool's `--identity` must name itself, carry a tool hash and print
# "schema_revision=<r>" and "schema_fingerprint=<hex>" equal to it.  A build tree in which some tools were rebuilt
# after a schema change and others were not fails here.
execute_process(COMMAND "${DUMP}" --layout OUTPUT_VARIABLE layout RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "${DUMP} --layout: exit ${rc}")
endif()
string(REGEX MATCH "^kz-layout abi=[0-9]+ revision=([0-9]+)\n" head "${layout}")
set(revision "${CMAKE_MATCH_1}")
string(REGEX MATCH "\nfingerprint=([0-9a-f]+)\n$" tail "${layout}")
set(fingerprint "${CMAKE_MATCH_1}")
if(revision STREQUAL "" OR fingerprint STREQUAL "")
  message(FATAL_ERROR "the layout of ${DUMP} has no revision or fingerprint line")
endif()
string(REPLACE "|" ";" tools "${TOOLS}")
set(n 0)
foreach(tool IN LISTS tools)
  get_filename_component(name "${tool}" NAME)
  execute_process(COMMAND "${tool}" --identity OUTPUT_VARIABLE id RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "${tool} --identity: exit ${rc}")
  endif()
  foreach(want "tool=${name}\n" "schema_revision=${revision}\n" "schema_fingerprint=${fingerprint}\n")
    string(FIND "${id}" "${want}" at)
    if(at EQUAL -1)
      string(STRIP "${want}" w)
      message(FATAL_ERROR "${tool} --identity does not print ${w}:\n${id}")
    endif()
  endforeach()
  if(NOT id MATCHES "\ntool_sha256=[0-9a-f]+\n")
    message(FATAL_ERROR "${tool} --identity prints no tool hash:\n${id}")
  endif()
  math(EXPR n "${n} + 1")
endforeach()
message(STATUS "${n} tools report schema revision ${revision}, fingerprint ${fingerprint}")

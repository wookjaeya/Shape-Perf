# cmake -DNM=<nm> -DLIB=<libkzrt.a> -P check_undefined.cmake
# Fails if the runtime core archive references any undefined symbol (it calls no library function).
# _GLOBAL_OFFSET_TABLE_ is defined by the linker for position-independent code, not by a library.
execute_process(COMMAND ${NM} -u ${LIB} OUTPUT_VARIABLE out RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nm failed on ${LIB}")
endif()
string(REGEX MATCHALL "[ \t]+U [^\n]+" undefined "${out}")
list(FILTER undefined EXCLUDE REGEX "U _GLOBAL_OFFSET_TABLE_$")
if(undefined)
  message(FATAL_ERROR "the runtime core references undefined symbols:${undefined}")
endif()
message(STATUS "no undefined symbols in ${LIB}")

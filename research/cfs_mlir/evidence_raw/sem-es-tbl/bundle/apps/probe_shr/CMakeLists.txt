project(PROBE_shr C)
add_cfe_app(probe_shr ${CMAKE_CURRENT_SOURCE_DIR}/../probe_common/probe_tbl.c)
target_compile_definitions(probe_shr PRIVATE PROBE_NAME="PROBE_SHR" PROBE_ENTRY=Probe_SHR_Main PROBE_ROLE=2)

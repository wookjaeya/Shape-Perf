project(PROBE_own C)
add_cfe_app(probe_own ${CMAKE_CURRENT_SOURCE_DIR}/../probe_common/probe_tbl.c)
target_compile_definitions(probe_own PRIVATE PROBE_NAME="PROBE_OWN" PROBE_ENTRY=Probe_OWN_Main PROBE_ROLE=1)

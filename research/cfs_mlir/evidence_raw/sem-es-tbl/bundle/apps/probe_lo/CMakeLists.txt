project(PROBE_lo C)
add_cfe_app(probe_lo ${CMAKE_CURRENT_SOURCE_DIR}/../probe_common/probe_order.c)
target_compile_definitions(probe_lo PRIVATE PROBE_NAME="PROBE_LO" PROBE_ENTRY=Probe_LO_Main PROBE_PRE_DELAY_MS=0 PROBE_WAIT_MS=5000)

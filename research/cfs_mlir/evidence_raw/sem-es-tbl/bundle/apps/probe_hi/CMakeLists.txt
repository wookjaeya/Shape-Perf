project(PROBE_hi C)
add_cfe_app(probe_hi ${CMAKE_CURRENT_SOURCE_DIR}/../probe_common/probe_order.c)
target_compile_definitions(probe_hi PRIVATE PROBE_NAME="PROBE_HI" PROBE_ENTRY=Probe_HI_Main PROBE_PRE_DELAY_MS=0 PROBE_WAIT_MS=5000)

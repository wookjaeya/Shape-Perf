project(PROBE_slow C)
add_cfe_app(probe_slow ${CMAKE_CURRENT_SOURCE_DIR}/../probe_common/probe_order.c)
target_compile_definitions(probe_slow PRIVATE PROBE_NAME="PROBE_SLOW" PROBE_ENTRY=Probe_SLOW_Main PROBE_PRE_DELAY_MS=2500 PROBE_WAIT_MS=5000)

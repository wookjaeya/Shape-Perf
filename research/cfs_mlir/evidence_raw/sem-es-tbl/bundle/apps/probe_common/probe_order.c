/* Probe app used only to observe startup ordering / sync semantics at a pinned cFE commit.
 * Built several times with different PROBE_NAME / PROBE_ENTRY / PROBE_PRE_DELAY_MS defines. */
#define _GNU_SOURCE
#include "cfe.h"
#include <time.h>
#include <pthread.h>
#include <sched.h>

static unsigned long long probe_now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000ULL + (unsigned long long)ts.tv_nsec / 1000ULL;
}

static void probe_log_sched(const char *tag)
{
    int                policy = -1;
    struct sched_param sp;
    sp.sched_priority = -1;
    pthread_getschedparam(pthread_self(), &policy, &sp);
    CFE_ES_WriteToSysLog("PROBE %s %s mono_us=%llu policy=%d rtprio=%d cpu=%d\n", PROBE_NAME, tag,
                         probe_now_us(), policy, sp.sched_priority, sched_getcpu());
}

void PROBE_ENTRY(void)
{
    uint32       RunStatus = CFE_ES_RunStatus_APP_RUN;
    CFE_Status_t st;
    unsigned long long t0;

    probe_log_sched("ENTRY");

#if PROBE_PRE_DELAY_MS > 0
    /* deliberately late: do not call any ES sync API for a while */
    OS_TaskDelay(PROBE_PRE_DELAY_MS);
    probe_log_sched("AFTER_PRE_DELAY");
#endif

    /* EVS before registration */
    st = CFE_EVS_SendEvent(1, CFE_EVS_EventType_INFORMATION, "probe %s before register", PROBE_NAME);
    CFE_ES_WriteToSysLog("PROBE %s SendEvent_before_Register st=0x%08x\n", PROBE_NAME, (unsigned int)st);
    st = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    CFE_ES_WriteToSysLog("PROBE %s EVS_Register st=0x%08x\n", PROBE_NAME, (unsigned int)st);

    t0 = probe_now_us();
    st = CFE_ES_WaitForSystemState(CFE_ES_SystemState_OPERATIONAL, PROBE_WAIT_MS);
    CFE_ES_WriteToSysLog("PROBE %s WaitForSystemState(OPERATIONAL,%d) st=0x%08x waited_us=%llu mono_us=%llu\n",
                         PROBE_NAME, PROBE_WAIT_MS, (unsigned int)st, probe_now_us() - t0, probe_now_us());

    while (CFE_ES_RunLoop(&RunStatus))
    {
        OS_TaskDelay(500);
    }
    CFE_ES_WriteToSysLog("PROBE %s leaving RunLoop RunStatus=%u mono_us=%llu\n", PROBE_NAME, (unsigned int)RunStatus,
                         probe_now_us());
    CFE_ES_ExitApp(RunStatus);
}

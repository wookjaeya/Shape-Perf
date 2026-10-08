/* Probe apps (owner / sharer) used only to observe Table Services and restart semantics
 * at a pinned cFE commit. Time-based choreography with 1 s steps after OPERATIONAL. */
#define _GNU_SOURCE
#include "cfe.h"
#include <time.h>

typedef struct { uint32 v[4]; } ProbeTbl_t;

static unsigned long long now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000ULL + (unsigned long long)ts.tv_nsec / 1000ULL;
}
#define LOG(...) CFE_ES_WriteToSysLog("PROBE " PROBE_NAME " " __VA_ARGS__)

static void sleep_until(unsigned long long t0, unsigned int step_ms)
{
    while (now_us() < t0 + (unsigned long long)step_ms * 1000ULL) OS_TaskDelay(20);
}

#if PROBE_ROLE == 1 /* ===================== OWNER ===================== */
static CFE_TBL_Handle_t HT = CFE_TBL_BAD_TABLE_HANDLE, HD = CFE_TBL_BAD_TABLE_HANDLE, HE = CFE_TBL_BAD_TABLE_HANDLE;
static bool RegT, RegD, RegE;

static void try_register(void)
{
    CFE_Status_t st;
    ProbeTbl_t   init = { { 1, 1, 1, 1 } };
    if (!RegT)
    {
        st = CFE_TBL_Register(&HT, "T", sizeof(ProbeTbl_t), CFE_TBL_OPT_DEFAULT, NULL);
        LOG("Register(T,single) st=0x%08x mono_us=%llu\n", (unsigned int)st, now_us());
        if (st >= CFE_SUCCESS)
        {
            void *p = (void *)0x1;
            RegT = true;
            st   = CFE_TBL_GetAddress(&p, HT);
            LOG("GetAddress(T) BEFORE load st=0x%08x ptr=%p\n", (unsigned int)st, p);
            st = CFE_TBL_ReleaseAddress(HT);
            LOG("ReleaseAddress(T) BEFORE load st=0x%08x\n", (unsigned int)st);
            st = CFE_TBL_Load(HT, CFE_TBL_SRC_ADDRESS, &init);
            LOG("Load(T,v1) st=0x%08x\n", (unsigned int)st);
        }
    }
    if (!RegD)
    {
        st = CFE_TBL_Register(&HD, "D", sizeof(ProbeTbl_t), CFE_TBL_OPT_DBL_BUFFER, NULL);
        LOG("Register(D,double) st=0x%08x mono_us=%llu\n", (unsigned int)st, now_us());
        if (st >= CFE_SUCCESS)
        {
            RegD = true;
            st   = CFE_TBL_Load(HD, CFE_TBL_SRC_ADDRESS, &init);
            LOG("Load(D,v1) st=0x%08x\n", (unsigned int)st);
        }
    }
    if (!RegE)
    {
        st = CFE_TBL_Register(&HE, "E", sizeof(ProbeTbl_t), CFE_TBL_OPT_DBL_BUFFER, NULL);
        LOG("Register(E,double) st=0x%08x\n", (unsigned int)st);
        if (st >= CFE_SUCCESS)
        {
            RegE = true;
            st   = CFE_TBL_Load(HE, CFE_TBL_SRC_ADDRESS, &init);
            LOG("Load(E,v1) st=0x%08x\n", (unsigned int)st);
        }
    }
}

void PROBE_ENTRY(void)
{
    uint32             RunStatus = CFE_ES_RunStatus_APP_RUN;
    CFE_Status_t       st;
    CFE_ES_AppId_t     self;
    ProbeTbl_t        *p;
    ProbeTbl_t         v2 = { { 2, 2, 2, 2 } }, v3 = { { 3, 3, 3, 3 } };
    unsigned long long t0;

    CFE_ES_GetAppID(&self);
    LOG("ENTRY appid=%lu mono_us=%llu\n", CFE_RESOURCEID_TO_ULONG(self), now_us());
    CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    try_register();

    /* single-buffer: own lock blocks own load */
    if (RegT)
    {
        st = CFE_TBL_GetAddress((void **)&p, HT);
        LOG("GetAddress(T) st=0x%08x val=%u (holding)\n", (unsigned int)st, p ? p->v[0] : 999);
        st = CFE_TBL_Load(HT, CFE_TBL_SRC_ADDRESS, &v2);
        LOG("Load(T,v2) while self holds lock st=0x%08x val_seen=%u\n", (unsigned int)st, p ? p->v[0] : 999);
        st = CFE_TBL_ReleaseAddress(HT);
        LOG("ReleaseAddress(T) st=0x%08x\n", (unsigned int)st);
        st = CFE_TBL_GetStatus(HT);
        LOG("GetStatus(T) st=0x%08x\n", (unsigned int)st);
        st = CFE_TBL_Manage(HT);
        LOG("Manage(T) st=0x%08x\n", (unsigned int)st);
        st = CFE_TBL_GetAddress((void **)&p, HT);
        LOG("GetAddress(T) after manage st=0x%08x val=%u\n", (unsigned int)st, p ? p->v[0] : 999);
        CFE_TBL_ReleaseAddress(HT);
    }

    CFE_ES_WaitForStartupSync(10000);
    t0 = now_us();
    LOG("OPERATIONAL t0 mono_us=%llu\n", t0);

    if (RegD)
    {
        sleep_until(t0, 2000);
        st = CFE_TBL_Load(HD, CFE_TBL_SRC_ADDRESS, &v2);
        LOG("t+2s Load(D,v2) while SHR holds D st=0x%08x\n", (unsigned int)st);
        st = CFE_TBL_Load(HE, CFE_TBL_SRC_ADDRESS, &v2);
        LOG("t+2s Load(E,v2) while SHR holds E (once) st=0x%08x\n", (unsigned int)st);
        sleep_until(t0, 4000);
        st = CFE_TBL_Load(HD, CFE_TBL_SRC_ADDRESS, &v3);
        LOG("t+4s Load(D,v3) while SHR still holds old D buffer st=0x%08x\n", (unsigned int)st);
        st = CFE_TBL_Load(HE, CFE_TBL_SRC_ADDRESS, &v3);
        LOG("t+4s Load(E,v3) while SHR holds first E buffer st=0x%08x\n", (unsigned int)st);
        st = CFE_TBL_GetStatus(HE);
        LOG("t+4s GetStatus(E) st=0x%08x\n", (unsigned int)st);
        sleep_until(t0, 6000);
        st = CFE_TBL_Load(HD, CFE_TBL_SRC_ADDRESS, &v3);
        LOG("t+6s Load(D,v3) after SHR release st=0x%08x\n", (unsigned int)st);
        st = CFE_TBL_Load(HE, CFE_TBL_SRC_ADDRESS, &v3);
        LOG("t+6s Load(E,v3) after SHR release st=0x%08x\n", (unsigned int)st);
    }

    while (CFE_ES_RunLoop(&RunStatus))
    {
        OS_TaskDelay(500);
        if (!RegT || !RegD || !RegE)
        {
            try_register();
        }
    }
    LOG("leaving RunLoop RunStatus=%u mono_us=%llu\n", (unsigned int)RunStatus, now_us());
    CFE_ES_ExitApp(RunStatus);
}

#else /* ===================== SHARER ===================== */
void PROBE_ENTRY(void)
{
    uint32             RunStatus = CFE_ES_RunStatus_APP_RUN;
    CFE_Status_t       st;
    CFE_TBL_Handle_t   HT = CFE_TBL_BAD_TABLE_HANDLE, HD = CFE_TBL_BAD_TABLE_HANDLE, HE = CFE_TBL_BAD_TABLE_HANDLE;
    ProbeTbl_t        *p = NULL, *pd = NULL, *pd2 = NULL, *pe = NULL;
    CFE_ES_AppId_t     own;
    unsigned long long t0;
    int                unreg_seen = 0, iter = 0;
    bool               unregistered = false, reshared = false;

    LOG("ENTRY mono_us=%llu\n", now_us());
    CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    CFE_ES_WaitForStartupSync(10000);
    t0 = now_us();
    LOG("OPERATIONAL t0 mono_us=%llu\n", t0);

    st = CFE_TBL_Share(&HT, "PROBE_OWN.T");
    LOG("Share(T) st=0x%08x\n", (unsigned int)st);
    st = CFE_TBL_Share(&HD, "PROBE_OWN.D");
    LOG("Share(D) st=0x%08x\n", (unsigned int)st);
    st = CFE_TBL_Share(&HE, "PROBE_OWN.E");
    LOG("Share(E) st=0x%08x\n", (unsigned int)st);

    sleep_until(t0, 1000);
    st = CFE_TBL_GetAddress((void **)&pd, HD);
    LOG("t+1s GetAddress(D) st=0x%08x val=%u ptr=%p (holding)\n", (unsigned int)st, pd ? pd->v[0] : 999, (void *)pd);
    st = CFE_TBL_GetAddress((void **)&pe, HE);
    LOG("t+1s GetAddress(E) st=0x%08x val=%u ptr=%p (holding once)\n", (unsigned int)st, pe ? pe->v[0] : 999, (void *)pe);
    sleep_until(t0, 3000);
    LOG("t+3s held D ptr=%p val=%u (owner loaded v2 at t+2s)\n", (void *)pd, pd ? pd->v[0] : 999);
    st = CFE_TBL_GetAddress((void **)&pd2, HD);
    LOG("t+3s second GetAddress(D) st=0x%08x ptr=%p val=%u\n", (unsigned int)st, (void *)pd2, pd2 ? pd2->v[0] : 999);
    LOG("t+3s held E ptr=%p val=%u (owner loaded v2 at t+2s)\n", (void *)pe, pe ? pe->v[0] : 999);
    sleep_until(t0, 5000);
    LOG("t+5s first D ptr=%p val=%u ; second D ptr=%p val=%u ; E ptr=%p val=%u (owner loaded v3 at t+4s)\n",
        (void *)pd, pd ? pd->v[0] : 999, (void *)pd2, pd2 ? pd2->v[0] : 999, (void *)pe, pe ? pe->v[0] : 999);
    st = CFE_TBL_ReleaseAddress(HE);
    LOG("t+5s ReleaseAddress(E) st=0x%08x\n", (unsigned int)st);
    st = CFE_TBL_ReleaseAddress(HD);
    LOG("t+5s ReleaseAddress(D) st=0x%08x\n", (unsigned int)st);

    sleep_until(t0, 8000);
    st = CFE_ES_GetAppIDByName(&own, "PROBE_OWN");
    st = CFE_ES_RestartApp(own);
    LOG("t+8s RestartApp(PROBE_OWN id=%lu) st=0x%08x mono_us=%llu\n", CFE_RESOURCEID_TO_ULONG(own), (unsigned int)st,
        now_us());

    while (CFE_ES_RunLoop(&RunStatus))
    {
        OS_TaskDelay(500);
        ++iter;
        if (iter > 60) continue;
        if (!unregistered)
        {
            st = CFE_TBL_GetAddress((void **)&p, HT);
            LOG("poll GetAddress(T) st=0x%08x ptr=%p mono_us=%llu\n", (unsigned int)st, (void *)p, now_us());
            if (st >= CFE_SUCCESS) CFE_TBL_ReleaseAddress(HT);
            if (st == CFE_TBL_ERR_UNREGISTERED && ++unreg_seen >= 6)
            {
                st = CFE_TBL_Unregister(HT);
                LOG("Unregister(T) st=0x%08x mono_us=%llu\n", (unsigned int)st, now_us());
                st = CFE_TBL_Unregister(HD);
                LOG("Unregister(D) st=0x%08x mono_us=%llu\n", (unsigned int)st, now_us());
                st = CFE_TBL_Unregister(HE);
                LOG("Unregister(E) st=0x%08x mono_us=%llu\n", (unsigned int)st, now_us());
                unregistered = true;
            }
        }
        else if (!reshared)
        {
            st = CFE_TBL_Share(&HT, "PROBE_OWN.T");
            LOG("re-Share(T) st=0x%08x mono_us=%llu\n", (unsigned int)st, now_us());
            if (st == CFE_SUCCESS)
            {
                reshared = true;
                st       = CFE_TBL_GetAddress((void **)&p, HT);
                LOG("GetAddress(T) after re-share st=0x%08x val=%u\n", (unsigned int)st, p ? p->v[0] : 999);
                CFE_TBL_ReleaseAddress(HT);
            }
        }
    }
    CFE_ES_ExitApp(RunStatus);
}
#endif

/*
 * sbprobe: empirical probe of cFE Software Bus semantics (research note sem-sb).
 * Built against cFE 546a0025, OSAL dad0ee99, PSP 53df1d54, SIMULATION=native.
 * Each test prints "SBPROBE <test> key=value ..." lines to stdout.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include "cfe.h"
#include "cfe_sb_msgids.h"
#include "cfe_sb_msg.h"

/* Test MIDs: telemetry range, not used by cFE core (base 0x0800 | topic 0x1xx) */
#define MID_A 0x0901 /* never subscribed */
#define MID_B 0x0902 /* subscribed then unsubscribed */
#define MID_C 0x0903 /* sequence counter */
#define MID_D 0x0904 /* MsgLim */
#define MID_E 0x0905 /* pipe full */
#define MID_F 0x0906 /* delivery order to 3 pipes */
#define MID_G 0x0907 /* inversion trigger (high prio relay) */
#define MID_H 0x0908 /* relay output (high prio relay) */
#define MID_G2 0x0909 /* inversion trigger (low prio relay) */
#define MID_H2 0x090A /* relay output (low prio relay) */
#define MID_T 0x090B  /* deep pipe / truncation test */

typedef struct
{
    CFE_MSG_TelemetryHeader_t Hdr;
    uint32                    Value;
} Probe_Msg_t;

static CFE_SB_PipeId_t HkPipe;

static void InitMsg(Probe_Msg_t *m, uint32 mid, uint32 val)
{
    memset(m, 0, sizeof(*m));
    CFE_MSG_Init(CFE_MSG_PTR(m->Hdr), CFE_SB_ValueToMsgId(mid), sizeof(*m));
    m->Value = val;
}

static CFE_Status_t Send(uint32 mid, uint32 val, bool orig)
{
    Probe_Msg_t m;
    InitMsg(&m, mid, val);
    return CFE_SB_TransmitMsg(CFE_MSG_PTR(m.Hdr), orig);
}

/* Request SB housekeeping and copy the payload */
static int GetSbHk(CFE_SB_HousekeepingTlm_Payload_t *out)
{
    CFE_MSG_CommandHeader_t cmd;
    CFE_SB_Buffer_t        *buf;
    CFE_SB_MsgId_t          mid;
    int                     tries;

    /* drain stale HK */
    while (CFE_SB_ReceiveBuffer(&buf, HkPipe, CFE_SB_POLL) == CFE_SUCCESS)
    {
    }
    memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init(CFE_MSG_PTR(cmd), CFE_SB_ValueToMsgId(CFE_SB_SEND_HK_MID), sizeof(cmd));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(cmd), true);
    for (tries = 0; tries < 10; ++tries)
    {
        if (CFE_SB_ReceiveBuffer(&buf, HkPipe, 1000) == CFE_SUCCESS)
        {
            CFE_MSG_GetMsgId(&buf->Msg, &mid);
            if (CFE_SB_MsgIdToValue(mid) == CFE_SB_HK_TLM_MID)
            {
                memcpy(out, &((const CFE_SB_HousekeepingTlm_t *)buf)->Payload, sizeof(*out));
                return 0;
            }
        }
    }
    return -1;
}

static int DrainCount(CFE_SB_PipeId_t p)
{
    CFE_SB_Buffer_t *buf;
    int              n = 0;
    while (CFE_SB_ReceiveBuffer(&buf, p, CFE_SB_POLL) == CFE_SUCCESS)
    {
        ++n;
    }
    return n;
}

static const char *PolicyName(int p)
{
    return p == SCHED_FIFO ? "FIFO" : (p == SCHED_RR ? "RR" : (p == SCHED_OTHER ? "OTHER" : "?"));
}

static void ReportSched(const char *who)
{
    int                policy;
    struct sched_param sp;
    pthread_getschedparam(pthread_self(), &policy, &sp);
    OS_printf("SBPROBE SCHED who=%s policy=%s prio=%d cpu=%d\n", who, PolicyName(policy), sp.sched_priority,
              sched_getcpu());
}

/* ---------------- T5: delivery order to 3 pipes ---------------- */
static CFE_SB_PipeId_t   OrdPipe[3];
static volatile uint32   OrdCounter;
static volatile uint32   OrdSeen[3];
static volatile uint32   OrdBeforeReturn[3];
static volatile uint32   TransmitReturned;

static void OrdChild(int idx)
{
    CFE_SB_Buffer_t *buf;
    char             nm[16];
    snprintf(nm, sizeof(nm), "ORD%d", idx);
    ReportSched(nm);
    while (CFE_SB_ReceiveBuffer(&buf, OrdPipe[idx], CFE_SB_PEND_FOREVER) == CFE_SUCCESS)
    {
        OrdSeen[idx]         = __atomic_add_fetch(&OrdCounter, 1, __ATOMIC_SEQ_CST);
        OrdBeforeReturn[idx] = (TransmitReturned == 0);
    }
}
static void OrdChild0(void) { OrdChild(0); }
static void OrdChild1(void) { OrdChild(1); }
static void OrdChild2(void) { OrdChild(2); }

/* ---------------- T6: cross-pipe causality inversion ---------------- */
static CFE_SB_PipeId_t RelayPipeHi, RelayPipeLo;

static void RelayLoop(CFE_SB_PipeId_t p, uint32 outmid, const char *nm)
{
    CFE_SB_Buffer_t *buf;
    Probe_Msg_t      m;
    ReportSched(nm);
    while (CFE_SB_ReceiveBuffer(&buf, p, CFE_SB_PEND_FOREVER) == CFE_SUCCESS)
    {
        InitMsg(&m, outmid, ((const Probe_Msg_t *)buf)->Value);
        CFE_SB_TransmitMsg(CFE_MSG_PTR(m.Hdr), true);
    }
}
static void RelayHi(void) { RelayLoop(RelayPipeHi, MID_H, "RELAY_HI"); }
static void RelayLo(void) { RelayLoop(RelayPipeLo, MID_H2, "RELAY_LO"); }

static void InversionTest(const char *label, uint32 trig, uint32 out, CFE_SB_PipeId_t obsPipe, int trials)
{
    int              t, inv = 0, normal = 0, other = 0;
    CFE_SB_Buffer_t *buf;
    CFE_SB_MsgId_t   mid;
    uint32           first, second;

    for (t = 0; t < trials; ++t)
    {
        Send(trig, (uint32)t, true);
        OS_TaskDelay(5);
        first = second = 0;
        if (CFE_SB_ReceiveBuffer(&buf, obsPipe, 100) == CFE_SUCCESS)
        {
            CFE_MSG_GetMsgId(&buf->Msg, &mid);
            first = CFE_SB_MsgIdToValue(mid);
        }
        if (CFE_SB_ReceiveBuffer(&buf, obsPipe, 100) == CFE_SUCCESS)
        {
            CFE_MSG_GetMsgId(&buf->Msg, &mid);
            second = CFE_SB_MsgIdToValue(mid);
        }
        if (first == out && second == trig)
            ++inv;
        else if (first == trig && second == out)
            ++normal;
        else
            ++other;
    }
    OS_printf("SBPROBE T6 %s trials=%d relay_output_before_trigger=%d trigger_before_relay_output=%d other=%d\n",
              label, trials, inv, normal, other);
}

void SBPROBE_Main(void)
{
    CFE_SB_HousekeepingTlm_Payload_t h0, h1;
    CFE_SB_PipeId_t                  p;
    CFE_SB_Buffer_t                 *buf;
    CFE_MSG_SequenceCount_t          s1 = 0, s2 = 0;
    CFE_ES_TaskId_t                  tid;
    CFE_Status_t                     rc[8];
    int                              i, trial, n;
    uint32                           pattern[6][3];
    int                              patcount[6];
    int                              npat = 0, allBeforeReturn = 0;

    CFE_ES_WaitForStartupSync(10000);
    ReportSched("MAIN");

    CFE_SB_CreatePipe(&HkPipe, 8, "SBP_HK");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CFE_SB_HK_TLM_MID), HkPipe);

    /* ---- T1: NoSubscribersCounter for never-subscribed vs unsubscribed MID ---- */
    GetSbHk(&h0);
    rc[0] = Send(MID_A, 1, true);
    GetSbHk(&h1);
    OS_printf("SBPROBE T1 never_subscribed rc=0x%08x NoSubDelta=%d MsgSendErrDelta=%d\n", (unsigned)rc[0],
              (int)(uint8)(h1.NoSubscribersCounter - h0.NoSubscribersCounter),
              (int)(uint8)(h1.MsgSendErrorCounter - h0.MsgSendErrorCounter));
    CFE_SB_CreatePipe(&p, 4, "SBP_T1");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_B), p);
    CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(MID_B), p);
    GetSbHk(&h0);
    rc[0] = Send(MID_B, 1, true);
    GetSbHk(&h1);
    OS_printf("SBPROBE T1 subscribed_then_unsubscribed rc=0x%08x NoSubDelta=%d MsgSendErrDelta=%d\n",
              (unsigned)rc[0], (int)(uint8)(h1.NoSubscribersCounter - h0.NoSubscribersCounter),
              (int)(uint8)(h1.MsgSendErrorCounter - h0.MsgSendErrorCounter));

    /* ---- T2: sequence counter advances while route has zero destinations ---- */
    CFE_SB_CreatePipe(&p, 4, "SBP_T2");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_C), p);
    Send(MID_C, 1, true);
    if (CFE_SB_ReceiveBuffer(&buf, p, 1000) == CFE_SUCCESS)
        CFE_MSG_GetSequenceCount(&buf->Msg, &s1);
    CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(MID_C), p);
    for (i = 0; i < 3; ++i)
        Send(MID_C, 2, true);
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_C), p);
    Send(MID_C, 3, true);
    if (CFE_SB_ReceiveBuffer(&buf, p, 1000) == CFE_SUCCESS)
        CFE_MSG_GetSequenceCount(&buf->Msg, &s2);
    OS_printf("SBPROBE T2 seq_first=%u seq_after_3_unsubscribed_sends=%u diff=%d\n", (unsigned)s1, (unsigned)s2,
              (int)s2 - (int)s1);

    /* ---- T3: MsgLim exceeded ---- */
    CFE_SB_CreatePipe(&p, 10, "SBP_T3");
    CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(MID_D), p, CFE_SB_DEFAULT_QOS, 2);
    GetSbHk(&h0);
    for (i = 0; i < 5; ++i)
        rc[i] = Send(MID_D, i, true);
    GetSbHk(&h1);
    n = DrainCount(p);
    OS_printf("SBPROBE T3 msglim=2 depth=10 sent=5 rc=[%x,%x,%x,%x,%x] received=%d MsgLimErrDelta=%d "
              "MsgSendErrDelta=%d\n",
              (unsigned)rc[0], (unsigned)rc[1], (unsigned)rc[2], (unsigned)rc[3], (unsigned)rc[4], n,
              (int)(uint16)(h1.MsgLimitErrorCounter - h0.MsgLimitErrorCounter),
              (int)(uint8)(h1.MsgSendErrorCounter - h0.MsgSendErrorCounter));

    /* ---- T4: pipe full ---- */
    CFE_SB_CreatePipe(&p, 3, "SBP_T4");
    CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(MID_E), p, CFE_SB_DEFAULT_QOS, 10);
    GetSbHk(&h0);
    for (i = 0; i < 5; ++i)
        rc[i] = Send(MID_E, i, true);
    GetSbHk(&h1);
    n = DrainCount(p);
    OS_printf("SBPROBE T4 msglim=10 depth=3 sent=5 rc=[%x,%x,%x,%x,%x] received=%d PipeOverflowDelta=%d "
              "MsgSendErrDelta=%d\n",
              (unsigned)rc[0], (unsigned)rc[1], (unsigned)rc[2], (unsigned)rc[3], (unsigned)rc[4], n,
              (int)(uint16)(h1.PipeOverflowErrorCounter - h0.PipeOverflowErrorCounter),
              (int)(uint8)(h1.MsgSendErrorCounter - h0.MsgSendErrorCounter));

    /* ---- T4b: deep pipe (20) with MsgLim 30: reveals OS queue truncation if any ---- */
    CFE_SB_CreatePipe(&p, 20, "SBP_T4B");
    CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(MID_T), p, CFE_SB_DEFAULT_QOS, 30);
    GetSbHk(&h0);
    for (i = 0; i < 20; ++i)
        Send(MID_T, i, true);
    GetSbHk(&h1);
    n = DrainCount(p);
    OS_printf("SBPROBE T4b requested_depth=20 sent=20 received=%d PipeOverflowDelta=%d\n", n,
              (int)(uint16)(h1.PipeOverflowErrorCounter - h0.PipeOverflowErrorCounter));

    /* ---- T5: delivery order to 3 pipes subscribed in order 0,1,2; receivers higher prio (50) ---- */
    for (i = 0; i < 3; ++i)
    {
        char nm[16];
        snprintf(nm, sizeof(nm), "SBP_ORD%d", i);
        CFE_SB_CreatePipe(&OrdPipe[i], 4, nm);
        CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_F), OrdPipe[i]);
    }
    CFE_ES_CreateChildTask(&tid, "ORD0", OrdChild0, CFE_ES_TASK_STACK_ALLOCATE, 16384, 50, 0);
    CFE_ES_CreateChildTask(&tid, "ORD1", OrdChild1, CFE_ES_TASK_STACK_ALLOCATE, 16384, 50, 0);
    CFE_ES_CreateChildTask(&tid, "ORD2", OrdChild2, CFE_ES_TASK_STACK_ALLOCATE, 16384, 50, 0);
    OS_TaskDelay(200);
    memset(patcount, 0, sizeof(patcount));
    for (trial = 0; trial < 200; ++trial)
    {
        OrdCounter       = 0;
        OrdSeen[0] = OrdSeen[1] = OrdSeen[2] = 0;
        TransmitReturned = 0;
        Send(MID_F, (uint32)trial, true);
        TransmitReturned = 1;
        OS_TaskDelay(5);
        if (OrdBeforeReturn[0] && OrdBeforeReturn[1] && OrdBeforeReturn[2])
            ++allBeforeReturn;
        for (i = 0; i < npat; ++i)
        {
            if (pattern[i][0] == OrdSeen[0] && pattern[i][1] == OrdSeen[1] && pattern[i][2] == OrdSeen[2])
                break;
        }
        if (i == npat && npat < 6)
        {
            pattern[npat][0] = OrdSeen[0];
            pattern[npat][1] = OrdSeen[1];
            pattern[npat][2] = OrdSeen[2];
            ++npat;
        }
        if (i < 6)
            ++patcount[i];
    }
    for (i = 0; i < npat; ++i)
        OS_printf("SBPROBE T5 wake_rank(pipe0,pipe1,pipe2)=(%u,%u,%u) count=%d\n", (unsigned)pattern[i][0],
                  (unsigned)pattern[i][1], (unsigned)pattern[i][2], patcount[i]);
    OS_printf("SBPROBE T5 trials=200 all_three_received_before_TransmitMsg_returned=%d\n", allBeforeReturn);

    /* ---- T6: causality inversion. Observer pipe subscribes trigger FIRST, relay pipe SECOND
     * (so the relay pipe is at the list head and is written first). ---- */
    CFE_SB_CreatePipe(&p, 8, "SBP_OBS_HI");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_G), p);
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_H), p);
    CFE_SB_CreatePipe(&RelayPipeHi, 8, "SBP_RLY_HI");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_G), RelayPipeHi);
    CFE_ES_CreateChildTask(&tid, "RELAY_HI", RelayHi, CFE_ES_TASK_STACK_ALLOCATE, 16384, 50, 0);
    OS_TaskDelay(100);
    InversionTest("relay_prio_higher_than_sender", MID_G, MID_H, p, 500);

    CFE_SB_CreatePipe(&p, 8, "SBP_OBS_LO");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_G2), p);
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_H2), p);
    CFE_SB_CreatePipe(&RelayPipeLo, 8, "SBP_RLY_LO");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MID_G2), RelayPipeLo);
    CFE_ES_CreateChildTask(&tid, "RELAY_LO", RelayLo, CFE_ES_TASK_STACK_ALLOCATE, 16384, 150, 0);
    OS_TaskDelay(100);
    InversionTest("relay_prio_lower_than_sender", MID_G2, MID_H2, p, 500);

    OS_printf("SBPROBE DONE\n");
    while (1)
    {
        OS_TaskDelay(1000);
    }
}

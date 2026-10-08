/*
 * verify_C09 diagnostic probe: does the SB NO_SUBS / MSGID_LIM / Q_FULL event actually get emitted
 * when an *application* task transmits? Measures SB HK counters and EVS HK counters around each send.
 * Built against the existing sem-sb build tree (cFE 546a0025, OSAL dad0ee99, PSP 53df1d54).
 */
#include <string.h>
#include "cfe.h"
#include "cfe_sb_msgids.h"
#include "cfe_sb_msg.h"
#include "cfe_evs_msgids.h"
#include "cfe_evs_msg.h"
#include "cfe_sb_priv.h"
#include "cfe_evs_task.h"
#include "cfe_sb_eventids.h"

#define MID_A 0x0911 /* never subscribed */
#define MID_B 0x0912 /* never subscribed (2nd) */
#define MID_D 0x0914 /* MsgLim */

typedef struct
{
    CFE_MSG_TelemetryHeader_t Hdr;
    uint32                    Value;
} Probe_Msg_t;

static CFE_SB_PipeId_t HkPipe;
static CFE_ES_AppId_t  SbAppId;

static CFE_Status_t Send(uint32 mid, uint32 val)
{
    Probe_Msg_t m;
    memset(&m, 0, sizeof(m));
    CFE_MSG_Init(CFE_MSG_PTR(m.Hdr), CFE_SB_ValueToMsgId(mid), sizeof(m));
    m.Value = val;
    return CFE_SB_TransmitMsg(CFE_MSG_PTR(m.Hdr), true);
}

static int GetHk(uint32 cmdmid, uint32 tlmmid, void *out, size_t sz)
{
    CFE_MSG_CommandHeader_t cmd;
    CFE_SB_Buffer_t        *buf;
    CFE_SB_MsgId_t          mid;
    int                     tries;
    while (CFE_SB_ReceiveBuffer(&buf, HkPipe, CFE_SB_POLL) == CFE_SUCCESS)
    {
    }
    memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init(CFE_MSG_PTR(cmd), CFE_SB_ValueToMsgId(cmdmid), sizeof(cmd));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(cmd), true);
    for (tries = 0; tries < 10; ++tries)
    {
        if (CFE_SB_ReceiveBuffer(&buf, HkPipe, 1000) == CFE_SUCCESS)
        {
            CFE_MSG_GetMsgId(&buf->Msg, &mid);
            if (CFE_SB_MsgIdToValue(mid) == tlmmid)
            {
                memcpy(out, ((const uint8 *)buf) + sizeof(CFE_MSG_TelemetryHeader_t), sz);
                return 0;
            }
        }
    }
    return -1;
}

typedef struct
{
    CFE_SB_HousekeepingTlm_Payload_t  sb;
    CFE_EVS_HousekeepingTlm_Payload_t evs;
} Snap_t;

static void Snap(Snap_t *s)
{
    GetHk(CFE_SB_SEND_HK_MID, CFE_SB_HK_TLM_MID, &s->sb, sizeof(s->sb));
    GetHk(CFE_EVS_SEND_HK_MID, CFE_EVS_HK_TLM_MID, &s->evs, sizeof(s->evs));
}

static int SbSent(const Snap_t *s)
{
    int i;
    for (i = 0; i < CFE_MISSION_ES_MAX_APPLICATIONS; ++i)
        if (CFE_RESOURCEID_TEST_EQUAL(s->evs.AppData[i].AppID, SbAppId))
            return s->evs.AppData[i].AppMessageSentCounter;
    return -1;
}

static void Report(const char *label, const Snap_t *a, const Snap_t *b, CFE_Status_t rc)
{
    OS_printf("SBDIAG %s rc=0x%08x NoSubDelta=%d MsgSendErrDelta=%d MsgLimDelta=%d EVS_MsgSendDelta=%d "
              "EVS_SB_AppSentDelta=%d (SB_AppSent before=%d after=%d)\n",
              label, (unsigned)rc, (int)(uint8)(b->sb.NoSubscribersCounter - a->sb.NoSubscribersCounter),
              (int)(uint8)(b->sb.MsgSendErrorCounter - a->sb.MsgSendErrorCounter),
              (int)(uint16)(b->sb.MsgLimitErrorCounter - a->sb.MsgLimitErrorCounter),
              (int)(uint16)(b->evs.MessageSendCounter - a->evs.MessageSendCounter), SbSent(b) - SbSent(a),
              SbSent(a), SbSent(b));
}

void SBPROBE_Main(void)
{
    Snap_t          s0, s1;
    CFE_ES_TaskId_t tid;
    uint32          idx = 9999;
    CFE_Status_t    st1, st2, rc;
    CFE_SB_PipeId_t p;
    int             i;

    CFE_ES_WaitForStartupSync(10000);
    OS_printf("SBDIAG EVS_Register rc=0x%08x\n", (unsigned)CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY));
    CFE_SB_CreatePipe(&HkPipe, 8, "SBD_HK");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CFE_SB_HK_TLM_MID), HkPipe);
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CFE_EVS_HK_TLM_MID), HkPipe);
    CFE_ES_GetAppIDByName(&SbAppId, "CFE_SB");

    st1 = CFE_ES_GetTaskID(&tid);
    st2 = CFE_ES_TaskID_ToIndex(tid, &idx);
    OS_printf("SBDIAG task GetTaskID=0x%08x ToIndex=0x%08x idx=%u\n", (unsigned)st1, (unsigned)st2, (unsigned)idx);

    OS_printf("SBDIAG StopRecurseFlags[idx]=0x%08x\n", (unsigned)CFE_SB_Global.StopRecurseFlags[idx]);
    {
        EVS_AppData_t *ad = NULL;
        uint32 k, j;
        for (k = 0; k < CFE_PLATFORM_ES_MAX_APPLICATIONS; ++k)
            if (CFE_RESOURCEID_TEST_EQUAL(CFE_EVS_Global.AppData[k].AppID, SbAppId)) ad = &CFE_EVS_Global.AppData[k];
        if (ad != NULL)
            for (j = 0; j < CFE_PLATFORM_EVS_MAX_EVENT_FILTERS; ++j)
                OS_printf("SBDIAG SB filter[%u] EventID=%u Mask=0x%04x Count=%u ActiveFlag=%d SquelchTokens=%d SquelchedCount=%u\n", (unsigned)j,
                          (unsigned)ad->BinFilters[j].EventID, (unsigned)ad->BinFilters[j].Mask,
                          (unsigned)ad->BinFilters[j].Count, (int)ad->ActiveFlag, (int)ad->SquelchTokens, (unsigned)ad->SquelchedCount);
    }
    Snap(&s0);
    rc = Send(MID_A, 1);
    Snap(&s1);
    Report("T1a never_subscribed_first", &s0, &s1, rc);
    OS_printf("SBDIAG after T1a StopRecurseFlags[idx]=0x%08x\n", (unsigned)CFE_SB_Global.StopRecurseFlags[idx]);
    Snap(&s0);
    rc = CFE_EVS_SendEventWithAppID(CFE_SB_SEND_NO_SUBS_EID, CFE_EVS_EventType_INFORMATION, SbAppId, "SBDIAG direct test event via SB AppId");
    Snap(&s1);
    Report("T0 direct_EVS_SendEventWithAppID_SB", &s0, &s1, rc);

    Snap(&s0);
    rc = Send(MID_B, 1);
    Snap(&s1);
    Report("T1b never_subscribed_second", &s0, &s1, rc);

    CFE_SB_CreatePipe(&p, 10, "SBD_T3");
    CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(MID_D), p, CFE_SB_DEFAULT_QOS, 2);
    Snap(&s0);
    for (i = 0; i < 5; ++i)
        rc = Send(MID_D, i);
    Snap(&s1);
    Report("T3 msglim2_5sends", &s0, &s1, rc);

    OS_printf("SBDIAG DONE\n");
    while (1)
        OS_TaskDelay(1000);
}

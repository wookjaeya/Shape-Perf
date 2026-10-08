# sem-sb: cFE Software Bus + OSAL queue semantics from source (2026-10-01)

Pins: see `commits.txt`. Abbreviations for permalinks:
- CFE = https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f
- OSAL = https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1
- SA = https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a

Labels: [FACT-src] = read in pinned source; [FACT-run] = observed in my probe runs (probe_runs/); [INF] = inference for the static analysis; [UNVERIFIED].

## 1. CreatePipe / depth
- [FACT-src] Rejects Depth==0 or Depth>OS_QUEUE_MAX_DEPTH (CFE/modules/sb/fsw/src/cfe_sb_api.c#L126). Creates one OSAL queue whose element is a *pointer* to a buffer descriptor (`sizeof(CFE_SB_BufferD_t *)`, #L163). Stores requested Depth as MaxQueueDepth (#L191). Pipe owned by caller AppId (#L192).
- [FACT-src] OS_QUEUE_MAX_DEPTH default 50 (OSAL/default_config.cmake#L391-L396; OSAL/src/os/shared/src/osapi-queue.c#L98).
- [FACT-src] POSIX impl silently truncates mq_maxmsg to BSP limit when OSAL_CONFIG_DEBUG_PERMISSIVE_MODE (OSAL/src/os/posix/src/os-impl-queues.c#L59-L69,#L97-L105). BSP reads /proc/sys/fs/mqueue/msg_max only if geteuid()!=0 (OSAL/src/bsp/generic-linux/src/bsp_start.c#L66-L78). cFE native sample config enables PERMISSIVE (CFE/cmake/sample_defs/native_osconfig.cmake#L22-L39); OSAL default is FALSE (OSAL/default_config.cmake#L159-L181).
- [FACT-run] non-root euid, msg_max=10: CreatePipe(depth 20) succeeded, 20 sends -> 10 received, PipeOverflowErrorCounter +10 (run4 T4b). Root in this container (no CAP_SYS_RESOURCE) with msg_max=10: core pipes fail, mq_open EINVAL, cFE EVS init aborts (run0).
- [INF] Effective pipe capacity is a platform/deployment parameter, not the source literal. An analyzer must take capacity from the deployment model (OSAL config + BSP + privileges), and SB's MaxQueueDepth telemetry can overstate it.

## 2. Subscribe / SubscribeEx / SubscribeLocal / Unsubscribe
- [FACT-src] All three map to CFE_SB_SubscribeFull (cfe_sb_api.c#L900-L929). Subscribe uses MsgLim=CFE_PLATFORM_SB_DEFAULT_MSG_LIMIT (default 4; CFE/modules/sb/fsw/inc/cfe_sb_internal_cfg.h#L107), Local uses Scope=LOCAL.
- [FACT-src] Only the pipe owner app may subscribe (#L975-L979). Route created on first subscribe (#L989-L1012). Duplicate subscription: event + DuplicateSubscriptionsCounter, Status stays CFE_SUCCESS, MsgLim NOT updated (#L1023-L1028, #L1040). New destination node inserted at list HEAD (cfe_sb_priv.c#L361-L391); header documents "message will first be sent to the last subscriber" (CFE/modules/core_api/fsw/inc/cfe_sb.h#L238-L244).
- [FACT-src] Quality (QoS) is not used for local routing: only copied into the subscription report when reporting is enabled (cfe_sb_task.c#L645-L674); typedef comment "Currently an unused parameter" (CFE/modules/sb/config/default_cfe_sb_extern_typedefs.h#L114-L125). Scope only affects reporting; FindDestinations ignores it (cfe_sb_priv.c#L1067-L1089).
- [FACT-src] Unsubscribe of a non-subscribed pair returns CFE_SUCCESS with an INFORMATION event (cfe_sb_api.c#L1216-L1235). No unsubscription report is sent.
- [FACT-src] Routes are append-only: CFE_SBR has AddRoute but no remove; only reset by CFE_SBR_Init at SB EarlyInit (CFE/modules/sbr/fsw/src/cfe_sbr_route_unsorted.c#L72-L114; cfe_sb_init.c#L74).
- [FACT-src] Ground commands can disable/enable a (MsgId,Pipe) destination at runtime (cfe_sb_task.c#L440-L468, #L355-L392); disabled destinations are skipped silently (cfe_sb_priv.c#L1070-L1077).

## 3. TransmitMsg (copy) path
Order of operations [FACT-src], cfe_sb_api.c#L1538-L1597 + cfe_sb_priv.c#L1032-L1297:
1. read MsgId and size from header (priv#L923-L955), validate MsgId (nonzero, <= 0x1FFF default; cfe_sb_msg_id_util.c#L200-L204) and size.
2. allocate SB buffer from the shared SB pool (api#L1557; cfe_sb_buf.c#L95-L137), memcpy whole message (api#L1579). Note: the comment at api#L1564-L1575 says no buffer when no route; the code allocates before routing.
3. FindDestinations under SB mutex (priv#L1049): route lookup by MsgId hash (cfe_sbr_map_hash.c#L151-L175); if route valid and IsOrigination: increment per-route sequence counter and write it into the copy (priv#L1058-L1064); walk dest list from head; per dest: skip inactive, skip IGNOREMINE self-pipe (priv#L1081-L1082); MsgLim check `BuffCount >= MsgId2PipeLim` -> mark error, MsgLimitErrorCounter++ (priv#L1006-L1012); else optimistic accounting BuffCount++, CurrentQueueDepth++ (priv#L1015-L1022). Unlock (priv#L1114).
4. If IsOrigination and transaction OK: CFE_MSG_OriginationAction, default = SetMsgTime(CFE_TIME_GetTime()) + checksum (CFE/modules/msg/fsw/src/cfe_msg_integrity.c#L30-L53) (priv#L1122-L1130).
5. For each recorded pipe, OS_QueuePut of the descriptor pointer WITHOUT the SB mutex (priv#L1173-L1231, ProcessPipes #L1239-L1257); mutex re-taken only on error to undo accounting.
6. Drop sender's reference (priv#L1294-L1296); events reported after all unlocks (api#L1594; priv#L802-L860).
- No route (MsgId never subscribed since boot): NoSubscribersCounter++ and SEND_NO_SUBS INFORMATION event, status CFE_SUCCESS (priv#L1091-L1097); seq counter not touched; origination action skipped (IsOK needs TransactionEventId==0, cfe_sb_priv.h#L785-L788).
- Route exists but zero destinations (all unsubscribed / pipes deleted): no counter, no event, CFE_SUCCESS, and the sequence counter still increments. [FACT-run] run1-5 T1: NoSubDelta=1 for never-subscribed, 0 for subscribed-then-unsubscribed; T2: seq 1 -> 5 after 3 sends with no destinations.
- MsgLim exceeded or pipe full: per-pipe error only; TransmitMsg still returns CFE_SUCCESS. [FACT-src] no status change in priv#L1006-L1012 / #L1194-L1227; documented by cFE functional test (CFE/modules/cfe_testcase/src/sb_sendrecv_test.c#L103-L107). [FACT-run] T3 (MsgLim 2, 5 sends): rc all 0, 2 received, MsgLimitErrorCounter +3; T4 (depth 3): rc all 0, 3 received, PipeOverflowErrorCounter +2.
- Events for NO_SUBS, DUP_SUBSCRIP (first 4), MSGID_LIM, Q_FULL (first 16) are EVS-filtered by default (cfe_sb_internal_cfg.h#L222-L243); NoSubscribersCounter is uint8 (CFE/modules/sb/config/default_cfe_sb_msgdefs.h#L73). [INF] events/counters are lossy observables; absence of an event is not evidence of delivery.
- Max destinations per MsgId = 16 default (cfe_sb_internal_cfg.h#L91); max routes 256 (#L56).

## 4. Lock / preemption during transmit
- [FACT-src] SB mutex is an OSAL mutex (cfe_sb_priv.c#L132-L172; cfe_sb_init.c#L52), POSIX: PTHREAD_PRIO_INHERIT + RECURSIVE (OSAL/src/os/posix/src/os-impl-mutex.c#L82,#L94). Not held during OS_QueuePut. OS_QueuePut uses OS_LOCK_MODE_NONE (osapi-queue.c#L200).
- [FACT-src] Header: "if a higher priority task is pending and subscribed to this message, that task may get to run before returning control to the caller" (cfe_sb.h#L417-L422).
- [FACT-run] root, SCHED_RR priorities active, pinned to 1 CPU (runs 1,5): 3 receivers (prio 50) of one TransmitMsg from sender (prio 100) all ran before TransmitMsg returned in 200/200 trials, wake order = reverse subscription order (pipe2, pipe1, pipe0) in 200/200.
- [FACT-run] Cross-pipe causality inversion: sender S publishes G to pipes {R (relay, subscribed 2nd), O (observer, subscribed 1st)}; relay task on R republishes H to O. With relay priority > sender on 1 CPU: O dequeued H before G in 500/500 trials (runs 1,5). Relay priority < sender: 0/500. 4 CPUs (runs 2,3): 0/500 inversions, but wake order of the 3 receivers took all 6 permutations. Non-root (priorities silently ignored, SCHED_OTHER) on 1 CPU (run4): 15/500 and 30/500 inversions regardless of OSAL priority.
- [INF] TransmitMsg is not an atomic broadcast. Order of arrival of one message at different pipes, and order between a message and its causal consequences on another pipe, depends on subscription order (runtime), task priorities, CPU count and OSAL permissive mode. A sound analysis must treat the per-destination puts as separate events after the transmit call starts; "publish happens-before every subscriber's receive of a later message" does not hold.

## 5. Zero copy
- [FACT-src] AllocateMessageBuffer: buffer from SB pool, owned by caller AppId, tracked in ZeroCopyList, memset 0 (cfe_sb_api.c#L1404-L1463). TransmitBuffer uses the same Execute path (#L1505-L1530); success transfers ownership (cfe_sb.h#L609-L624); on failure ownership stays with app. ReleaseMessageBuffer only for untransmitted buffers (#L1471-L1497).
- [FACT-src] Fan-out is by reference: the same descriptor pointer is enqueued on every pipe and UseCount incremented per pipe (cfe_sb_priv.c#L1015, #L1187-L1188). All subscribers read the same bytes.

## 6. ReceiveBuffer
- [FACT-src] Timeout: >0 ms -> absolute deadline (cfe_sb_priv.c#L556-L578); CFE_SB_POLL=0, CFE_SB_PEND_FOREVER=-1 (cfe_sb_api_typedefs.h#L45-L46); other negatives -> BAD_ARGUMENT. Map: OS_QUEUE_EMPTY->CFE_SB_NO_MESSAGE, OS_QUEUE_TIMEOUT->CFE_SB_TIME_OUT, other->CFE_SB_PIPE_RD_ERR (priv#L1476-L1526). POSIX timed wait uses CLOCK_REALTIME absolute time (OSAL/src/os/posix/src/os-impl-common.c#L147-L149).
- [FACT-src] Buffer validity: at the start of every ReceiveBuffer on that pipe (if args valid), the previous LastBuffer reference is dropped (priv#L1384-L1389), regardless of later outcome (timeout etc.). Comment: one pipe cannot be shared by several worker tasks (priv#L1372-L1383). Header: read-only, valid until next call on same pipe (cfe_sb.h#L454-L463).
- [FACT-src] On receive, the per-(MsgId,pipe) BuffCount is decremented only if the destination still exists (priv#L1437-L1449): unsubscribe/resubscribe while queued is "nominal".
- [FACT-src] Default CFE_MSG_VerificationAction accepts everything (cfe_msg_integrity.c#L60-L76).

## 7. FIFO / ordering
- [FACT-src] POSIX put: mq_timedsend with constant priority 1 and zero absolute timeout (non-blocking), flags argument ignored (os-impl-queues.c#L285-L323; API says flags reserved, osapi-queue.h#L123). SB passes a timeout value as the flags argument (cfe_sb_priv.c#L1188).
- [FACT-src ext] Linux v6.12 ipc/mqueue.c: same-priority messages appended with list_add_tail and dequeued with list_first_entry (L191-L231, L248-L288); man-pages mq_send(3) "newer messages of the same priority being placed after older messages" (ext/mq_send.3 L71-L76). => per-pipe FIFO on Linux.
- [FACT-src] RTEMS impl: rtems_message_queue_send (os-impl-queues.c rtems #L253-L256); RTEMS docs: send puts at rear (rtems-docs directives.md L589). VxWorks impl: msgQSend(..., NO_WAIT, MSG_PRI_NORMAL) on MSG_Q_FIFO queue (vxworks os-impl-queues.c #L76,#L186). [UNVERIFIED] VxWorks ordering semantics (vendor docs not accessed).
- [INF] Per-pipe FIFO holds for puts into the same pipe; puts from one task are in program order, so one sender -> one pipe is FIFO. Across different senders to the same pipe the queue order = order of OS_QueuePut calls, which can differ from the order in which SB assigned sequence numbers (seq assigned under the mutex, put done after unlock). Across different pipes there is no global order.

## 8. Message IDs
- [FACT-src] CFE_SB_MsgId_t = struct { uint32 Value } (default_cfe_sb_extern_typedefs.h#L89,#L102-L105); wrap/unwrap macros (cfe_sb_api_typedefs.h#L64,#L89); CFE_SB_ValueToMsgId / MsgIdToValue / MsgId_Equal are static inline (cfe_sb.h#L876-L940). Valid = nonzero and <= CFE_PLATFORM_SB_HIGHEST_VALID_MSGID (0x1FFF default).
- [FACT-src] Non-EDS default mapping is compile-time: CMD MIDV = 0x1800|topic, TLM = 0x0800|topic (CFE/modules/core_api/config/default_cfe_core_api_msgid_mapping.h#L49-L64; base values default_cfe_core_api_base_msgid_values.h#L61-L62). sample_app chain: topic ids (SA/fsw/inc/sample_app_topicids.h#L28-L33) -> SA/config/default_sample_app_msgid_values.h#L29-L30 -> SA/config/default_sample_app_msgids.h#L29-L31. Dispatch uses if/else with CFE_SB_MsgId_Equal on lazily cached static MIDs (SA/fsw/src/sample_app_dispatch.c#L132-L167).
- [FACT-src] EDS mapping makes MIDs run-time function calls depending on CFE_PSP_GetProcessorId() (CFE/modules/core_api/config/eds_cfe_core_api_msgid_mapping.h#L37,#L46; cfe_sb_eds_msg_id_util.c#L231-L246). Which file is used depends on build config (default_ vs eds_ prefixed config headers).
- [FACT-src, observation] non-EDS CFE_SB_CmdTopicIdToMsgId with InstanceNum==0 calls CFE_SB_GlobalTlmTopicIdToMsgId (cfe_sb_msg_id_util.c#L108-L123). [UNVERIFIED] intent/impact.
- Routing key: hash of MsgId value into map of size 4*MAX_MSG_IDS with linear probing (cfe_sbr_map_hash.c#L54-L99); default implementation HASH (CFE/modules/sbr/CMakeLists.txt).
- [INF] MID resolution needs constant propagation through the struct wrapper and the config header actually selected by the build (use compile_commands.json); EDS builds need a processor-id model.

## 9. Cleanup on delete/restart
- [FACT-src] ES control request (delete/restart/reload) -> CFE_ES_CleanUpApp then (restart/reload) CFE_ES_AppCreate with a new AppId (CFE/modules/es/fsw/src/cfe_es_appctrl.c#L318-L379). CleanUpApp calls core-module cleanup callbacks BEFORE deleting tasks (cfe_es_apps.c#L1077-L1078; callbacks #L855-L904); SB's callback is CFE_SB_CleanUpApp (cfe_sb_objtab.c#L36).
- [FACT-src] CFE_SB_CleanUpApp: delete every pipe owned by the AppId, then release its zero-copy buffers (cfe_sb_priv.c#L87-L124). DeletePipeFull: remove pipe from all routes under lock, mark slot reserved, drain and discard queued messages, OS_QueueDelete (cfe_sb_api.c#L352-L511). Routes and their sequence counters persist.
- [INF] Restart window: between cleanup and the new instance's Subscribe calls, messages to those MIDs are dropped with no NO_SUBS indication if the route existed; seq counters continue. New instance gets new AppId and (normally) new PipeIds.

## 10. SB task itself
- [FACT-src] SB task: AppInit creates its command pipe (depth = OS_QUEUE_MAX_DEPTH, cfe_sb_internal_cfg.h#L367-L368) and subscribes, then waits CORE_READY before its receive loop (cfe_sb_task.c#L60-L112, #L173-L204). Routing tables are initialised earlier in CFE_SB_EarlyInit (cfe_sb_init.c#L44-L82).

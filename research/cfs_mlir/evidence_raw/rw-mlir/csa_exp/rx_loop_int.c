/* cFS-shaped probe (not cFS code): one app run loop, two handlers sharing a global. */
#include <stdint.h>
#include <string.h>
typedef struct { uint16_t mid; int32_t att; } Msg_t;
typedef struct { int32_t att; int valid; } State_t;
State_t g_state;
int32_t CFE_SB_ReceiveBuffer(Msg_t **buf, uint32_t pipe, int32_t timeout);
void Control(int32_t a);                      /* consumer = taint sink */
static void SetState(int32_t x) { g_state.att = x; g_state.valid = 1; }
static void HandleAttitude(const Msg_t *m) { SetState(m->att); }
static void Guidance(void) { Control(g_state.att); }       /* no 'valid' guard */
void APP_RunLoop(uint32_t pipe) {
  Msg_t *buf;
  for (;;) {
    if (CFE_SB_ReceiveBuffer(&buf, pipe, -1) != 0) break;
    switch (buf->mid) { case 1: HandleAttitude(buf); break; case 2: Guidance(); break; }
  }
}
/* Separate entry point (e.g. called from another task / scheduler wakeup): */
void APP_Wakeup(void) { Guidance(); }

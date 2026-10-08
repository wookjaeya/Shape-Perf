#include <stdint.h>
#include <string.h>
typedef struct { uint16_t mid; uint8_t payload[8]; } Msg_t;
typedef struct { double att; int valid; uint32_t count; } State_t;
State_t g_state;
extern int32_t CFE_SB_ReceiveBuffer(Msg_t **buf, uint32_t pipe, int32_t timeout);
extern int32_t CFE_SB_TransmitMsg(const Msg_t *msg, int inc);
static void SetState(double x) { g_state.att = x; g_state.valid = 1; }
void HandleAttitude(const Msg_t *m) { double d; memcpy(&d, m->payload, sizeof d); SetState(d); }
double Control(double a);
void Guidance(void) { if (g_state.valid) Control(g_state.att); }
void Main(uint32_t pipe) {
  Msg_t *buf; 
  for (;;) {
    int32_t st = CFE_SB_ReceiveBuffer(&buf, pipe, -1);
    if (st != 0) break;
    switch (buf->mid) { case 0x0801: HandleAttitude(buf); break; case 0x0802: Guidance(); break; default: g_state.count++; }
  }
}

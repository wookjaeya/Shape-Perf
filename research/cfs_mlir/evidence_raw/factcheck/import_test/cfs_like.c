#include <string.h>
#include <stdarg.h>
#include <stdint.h>
typedef struct { uint32_t Mid; double q[4]; } AttMsg_t;
typedef struct { double att[4]; int valid; } AppState_t;
static AppState_t G;
extern int32_t CFE_SB_ReceiveBuffer(void **buf, uint32_t pipe, int32_t timeout);
extern int32_t CFE_EVS_SendEvent(uint16_t id, uint16_t type, const char *fmt, ...);
typedef void (*Handler_t)(const void *);
static void OnAtt(const void *p) { const AttMsg_t *m = p; memcpy(G.att, m->q, sizeof G.att); G.valid = 1; }
static Handler_t Table[1] = { OnAtt };
void Compute(void);
void AppMain(uint32_t pipe) {
  void *buf;
  if (CFE_SB_ReceiveBuffer(&buf, pipe, -1) == 0) {
    const AttMsg_t *m = buf;
    if (m->Mid == 0x1801) Table[0](buf);
    else CFE_EVS_SendEvent(3, 2, "bad mid 0x%x", (unsigned)m->Mid);
  }
  if (G.valid) Compute();
}

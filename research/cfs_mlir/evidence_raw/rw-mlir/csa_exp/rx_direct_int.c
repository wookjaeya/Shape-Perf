#include <stdint.h>
typedef struct { uint16_t mid; int att; } Msg_t;
int32_t CFE_SB_ReceiveBuffer(Msg_t **buf, uint32_t pipe, int32_t timeout);
void Control(int a);
int g_att;
void V1_direct(uint32_t pipe) { Msg_t *buf; if (CFE_SB_ReceiveBuffer(&buf, pipe, -1) == 0) Control(buf->att); }
void V2_via_global(uint32_t pipe) { Msg_t *buf; if (CFE_SB_ReceiveBuffer(&buf, pipe, -1) == 0) { g_att = buf->att; Control(g_att); } }

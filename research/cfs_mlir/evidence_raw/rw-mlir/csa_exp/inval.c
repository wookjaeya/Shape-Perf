#include <stdint.h>
typedef struct { uint16_t mid; int32_t att; } Msg_t;
int32_t CFE_SB_ReceiveBuffer(Msg_t **buf, uint32_t pipe, int32_t timeout);
void Control(int32_t a);
void Opaque(void);
int32_t g_pub; static int32_t g_stat;
void A_no_call(uint32_t p)     { Msg_t *b; CFE_SB_ReceiveBuffer(&b, p, -1); g_pub = b->att; Control(g_pub); }
void B_pub_opaque(uint32_t p)  { Msg_t *b; CFE_SB_ReceiveBuffer(&b, p, -1); g_pub = b->att; Opaque(); Control(g_pub); }
void C_stat_opaque(uint32_t p) { Msg_t *b; CFE_SB_ReceiveBuffer(&b, p, -1); g_stat = b->att; Opaque(); Control(g_stat); }
void D_stat_recv(uint32_t p)   { Msg_t *b; CFE_SB_ReceiveBuffer(&b, p, -1); g_stat = b->att; Msg_t *c; CFE_SB_ReceiveBuffer(&c, p, -1); Control(g_stat); }

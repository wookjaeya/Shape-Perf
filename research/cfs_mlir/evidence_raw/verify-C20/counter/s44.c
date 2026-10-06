typedef struct { double att; int valid; } State_t;
typedef struct { int mid; double att; } AttMsg_t;
State_t global;
void Control(double);
static void SetState(double x) { global.att = x; }
void HandleAttitude(const AttMsg_t *msg) { SetState(msg->att); }
void Guidance(void) { Control(global.att); }

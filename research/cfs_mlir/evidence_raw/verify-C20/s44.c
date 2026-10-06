typedef struct { double att; int valid; } Msg_t;
typedef struct { double att; int valid; } State_t;
State_t global;
void Control(double);
static void SetState(double x) { global.att = x; }
void HandleAttitude(const Msg_t *msg) { SetState(msg->att); }
void Guidance(void) { Control(global.att); }
void RunLoop(const Msg_t *m) { HandleAttitude(m); Guidance(); }

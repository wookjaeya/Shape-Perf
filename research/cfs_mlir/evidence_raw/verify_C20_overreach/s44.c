typedef struct { double att; int valid; } State_t;
State_t global;
void Control(double);
static void SetState(double x) { global.att = x; }
void HandleAttitude(const double *msg) { SetState(*msg); }
void Guidance(void) { Control(global.att); }

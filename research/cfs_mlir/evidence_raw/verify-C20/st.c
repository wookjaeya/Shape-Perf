typedef struct { double att; int valid; int mode; } State_t;
State_t g_state;
static State_t s_state;
static void SetState(double x) { g_state.att = x; s_state.att = x; }
void HandleAttitude(double x) { SetState(x); }
double Guidance(void) { return g_state.att; }

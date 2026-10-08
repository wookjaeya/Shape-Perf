typedef struct { double att; int valid; int mode; } State_t;
State_t g_state;
static void SetState(double x) { g_state.att = x; g_state.valid = 1; }
void HandleAttitude(double x) { SetState(x); }

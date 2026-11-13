/* outptr.c -- writers through an out parameter (Slot.id): the effect of a callee that keeps to
 * stores through the pointer, and the callees whose effect cannot be summarized.
 * Target: Slot.id (--target on the command line). */
typedef struct {
  int id;
  int aux;
} Slot;

Slot g_slot;
Slot g_other;
int g_ready;

/* store-only: two guarded stores through the parameter */
int fetch_id(int *out) {
  if (g_ready) {
    *out = 3;
    return 0;
  }
  *out = 0;
  return -1;
}

/* passes the pointer on to a store-only callee */
int fetch_nested(int *out) { return fetch_id(out); }

/* reads through the pointer and stores through it: still store-only for the stores */
int fetch_twice(int *out) {
  *out = 1;
  if (*out > 0) *out = 2;
  return 0;
}

/* keeps the pointer: the callee escapes */
int *g_kept;
void keep_pointer(int *out) { g_kept = out; }

/* defined elsewhere: no body in the closed program */
int fetch_external(int *out);

void publish(void) { (void)fetch_id(&g_slot.id); }
void publish_nested(void) { (void)fetch_nested(&g_slot.id); }
void publish_twice(void) { (void)fetch_twice(&g_slot.id); }
void publish_kept(void) { keep_pointer(&g_other.id); }
void publish_external(void) { (void)fetch_external(&g_other.id); }

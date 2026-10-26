/* kzgcc_shapes.c -- unit-test target of the kzs-sites 2 point and effect shapes of the kzgcc plugin (not cFS;
 * generic names, unit-test values).  The shapes the stage D sites have: a field store through a local pointer
 * inside a branch (the AppState raise), a store into a global object's field (a shutdown request), a memset of a
 * global as a call with two sites after it (the EarlyInit and ES main clears), a point at a call that reads the
 * enclosing function's parameter as a validated string (a create's publish), a point after a call whose result
 * is tested (a registry check), and a line-start point.  One thread.  Line numbers are named by the manifest
 * kzgcc_shapes.kzs: keep them. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct rec { uint32_t state; uint32_t id; };
struct idle { int pad; bool shutdown_req; };
struct table { char names[8][20]; uint32_t ids[8]; };

struct rec recs[4];
struct idle idle_state;
struct table table;
volatile int sink;

__attribute__((noinline)) struct rec *rec_by_id(uint32_t id) { return &recs[id & 3u]; }

__attribute__((noinline)) void raise_state(uint32_t id, uint32_t required) {
  struct rec *r = rec_by_id(id);
  if (r->state < required)
  {
    r->state = required;
  }
}

__attribute__((noinline)) void request_shutdown(void) {
  idle_state.shutdown_req = true;
  sink = 7;
}

__attribute__((noinline)) void table_init(void) {
  memset(&table, 0, sizeof(table));
  table.ids[0] = 1;
}

__attribute__((noinline)) int32_t check_name(const char *n) { return n != NULL && memchr(n, 0, 20) != NULL ? 0 : -13; }
__attribute__((noinline)) int32_t finalize(int32_t status, uint32_t *out, uint32_t id) { if (status == 0) *out = id; return status; }

__attribute__((noinline)) int32_t obj_create(uint32_t *id, const char *name, uint32_t initial) {
  int32_t r;
  if (id == NULL) return -1;
  r = check_name(name);
  if (r != 0) return r;
  r = finalize(0, id, 0x30000u + initial);
  return r;
}

__attribute__((noinline)) bool is_match(uint32_t id) { return recs[id & 3u].id == id; }

__attribute__((noinline)) int32_t send(uint32_t id) {
  int32_t status = 0;
  if (!is_match(id))
  {
    status = -2;
  }
  sink = status;
  return status;
}

int main(void) {
  uint32_t id = 0;
  recs[1].state = 1;
  recs[1].id = 1;
  raise_state(1, 3); /* raises: one store */
  raise_state(1, 2); /* does not raise: no store */
  request_shutdown();
  table_init();
  obj_create(&id, "S_name", 5);
  send(1); /* matches */
  send(2); /* does not match */
  printf("shapes %u %u %u 0x%x state=%p shutdown=%p table=%p size=%zu\n", (unsigned)recs[1].state, (unsigned)idle_state.shutdown_req,
         (unsigned)table.ids[0], (unsigned)id, (void *)&recs[1].state, (void *)&idle_state.shutdown_req, (void *)&table, sizeof(table));
  return 0;
}

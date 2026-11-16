/* objaccess.c -- a fixture for types and object access (extract --extended-facts --object-access, static_facts.md 5s):
 *
 *   target array fields whose pointer leaves the expression: passed to a function that keeps it as an integer address,
 *   to a function that stores through it, to a function without a body, stored in a global pointer, converted to an
 *   integer; a store to one element; the destination of a data function;
 *   a character array of a record reached through a global pointer, passed to a function without a body once under a
 *   test of its first character (a read that dominates the call) and once before such a read;
 *   reads and writes of globals, of a local and through a parameter (OBJECT_ACCESS), the fields of the record types the
 *   reads name (FIELD_TYPES) and the type of every call argument (ARG_TYPES).
 */
#include <string.h>

#define NAMELEN 20
#define POOLSZ 32

typedef unsigned char u8;

typedef struct {
  char name[NAMELEN];
  int used;
} Chan;

typedef struct {
  Chan chan[2];
} Config;

typedef struct {
  u8 pool[POOLSZ];
  u8 buf[8];
  char label[NAMELEN];
  char tag[4];
  u8 data[4];
  u8 raw[4];
  unsigned long addr;
  int count;
  unsigned flag : 3;
} Mem;

Mem mem;
Config *cfg;
static Config cfg_store;
const char *last;
unsigned long base_addr;

int lookup(int *out, const char *name);

void pool_create(void *p, unsigned long n) {
  base_addr = (unsigned long)p;
  (void)n;
}

void fill(u8 *p) { *p = 7; }

void set_count(Mem *m, int v) { m->count = v; }

void init(void) {
  int h = 0, i;
  Chan local;
  cfg = &cfg_store;
  pool_create(mem.pool, sizeof mem.pool);
  fill(mem.buf);
  (void)lookup(&h, mem.label);
  last = mem.tag;
  mem.addr = (unsigned long)mem.raw;
  mem.data[1] = 5;
  memset(mem.buf, 0, sizeof mem.buf);
  mem.count = 1;
  set_count(&mem, 2);
  for (i = 0; i < 2; i++)
    if (cfg->chan[i].name[0]) (void)lookup(&h, cfg->chan[i].name);
  (void)lookup(&h, cfg_store.chan[1].name);
  if (cfg_store.chan[1].name[0]) mem.count = 3;
  local.used = mem.count;
  (void)lookup(&h, local.name);
  if (mem.flag != 0) mem.count = 4;
}

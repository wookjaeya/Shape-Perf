/* anonrec.c -- the names of record types without a file-scope tag or typedef name (static_facts.md 5q), read in the
 * profile-free mode with the rule targets (extract --no-profile --targets-rule R-T1,R-T2), golden anonrec_np:
 *
 *   - a macro that makes an unnamed union type, used for a member of two named records: each member's type has a name of
 *     its own (<record>::<member>), so the two are two types, not one;
 *   - a file-scope object of an unnamed struct type (internal linkage), cleared by memset, its field passed to a creator
 *     by address and another field written: the type is named by the object (<object key>::type) and the memset is a
 *     writer of its fields;
 *   - a tag declared inside a record (file scope in C): named by its tag, so its fields are found and written;
 *   - a tag and an unnamed union in a function body: named with the position of their declaration;
 *   - an unnamed struct that is the type of an array member: named by the member.
 *
 * C11 unnamed members (an anonymous struct or union) are not C99; test_static_cli reads that case from a source it writes.
 */
#include <string.h>

typedef unsigned char ar_u8;

#define AR_POOL_TYPE(n) \
  union {               \
    long Align;         \
    ar_u8 Data[n];      \
  }

typedef struct {
  int Hdl;
  AR_POOL_TYPE(16) Partition;
} ArSbParams_t;

typedef struct {
  int Hdl;
  AR_POOL_TYPE(32) Partition;
} ArTblParams_t;

ArSbParams_t ar_sb;
ArTblParams_t ar_tbl;

static struct {
  int sys_id;
  int count;
} ar_global;

struct ArOuter {
  struct ArInner {
    int ix;
  } in;
  int n;
};
struct ArOuter ar_outer;
struct ArInner ar_inner;

struct ArTable {
  struct {
    int key;
    int value;
  } rows[4];
};
struct ArTable ar_table;

int ar_create(int *out); /* no body: stores through its parameter */
int ar_use(const void *p); /* no body */

void ar_init(void) {
  memset(&ar_global, 0, sizeof(ar_global));
  ar_create(&ar_global.sys_id);
  ar_global.count = 1;
  ar_sb.Partition.Data[0] = 1;
  ar_tbl.Partition.Data[1] = 2;
  ar_outer.in.ix = 3;
  ar_inner.ix = 4;
  ar_table.rows[1].key = 7;
  {
    struct ArLocal {
      int v;
    } loc;
    loc.v = 5;
    ar_use(&loc);
  }
  {
    union {
      int w;
      ar_u8 b[4];
    } check;
    check.w = 1;
    ar_use(&check);
  }
}

int ar_read(void) { return ar_global.sys_id + ar_global.count + ar_sb.Partition.Data[0] + ar_tbl.Partition.Data[1] + ar_outer.in.ix + ar_table.rows[1].key; }

int ar_main(void) {
  ar_init();
  return ar_read();
}

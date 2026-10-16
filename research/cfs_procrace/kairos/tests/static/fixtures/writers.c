/* writers.c -- the writers of one struct field (Cell.id) and its static-initializer image.
 * Target: Cell.id (the profile location). */
/* No system header: the fixtures are the same program on every machine. */
typedef __SIZE_TYPE__ size_t;
void *memset(void *dest, int fill, size_t n);

typedef struct {
  int id;
  int aux;
} Cell;

typedef struct {
  Cell inner;
  int tag;
} Holder;

Cell g_cell;                                   /* no initializer: zero */
Cell g_seeded = {7, 0};                        /* static initializer: id = 7 */
Holder g_holder = {{3, 4}, 5};                 /* nested initializer: inner.id = 3 */
static Cell g_table[2] = {{11, 0}, {12, 0}};   /* array of cells */

void set_const(void) { g_cell.id = 5; }
void set_param(int v) { g_cell.id = v; }
void set_via_pointer(Cell *c, int v) { c->id = v + 1; }
void set_nested(Holder *h) { h->inner.id = 9; }
void set_array(int i) { g_table[i].id = 8; }
void bump(void) { g_cell.id++; }
void reset(void) { memset(&g_cell, 0, sizeof g_cell); }
void copy_all(const Cell *src) { g_cell = *src; }

void guarded(int x) {
  if (x > 0)
    g_cell.id = 1;
  else
    g_cell.id = 2;
}

int read_only(void) { return g_cell.id + g_seeded.id + g_holder.inner.id; }

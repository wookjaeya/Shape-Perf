/* derives.c -- which values reach the stamp argument of a call (copy chains).
 * Targets: Cell.id (the profile location).  The profile names fx_consume as an operation
 * whose first parameter is a stamp. */
typedef struct {
  int id;
  int aux;
} Cell;

Cell g_cell;
int g_other;

int fx_consume(int who);

void direct(void) { fx_consume(g_cell.id); }

void through_locals(void) {
  int a = g_cell.id;
  int b = a;
  fx_consume(b);
}

void from_parameter(int p) { fx_consume(p); }

void helper(int p) { fx_consume(p); }
void through_call(void) { helper(g_cell.id); }

void conditional(int c) {
  int a = 0;
  if (c) a = g_cell.id;
  fx_consume(a);
}

void arithmetic(void) { fx_consume(g_cell.id + 1); }

void reassigned(void) {
  int a = g_cell.id;
  a = g_other;
  fx_consume(a);
}

void constant(void) { fx_consume(5); }

void other_field(void) { fx_consume(g_cell.aux); }

void via_pointer(const Cell *c) { fx_consume(c->id); }

/* a conversion that cannot lose information (to a type at least as wide) keeps the copy */
short g_small;
void widened(void) { fx_consume(g_small); }

/* a narrowing conversion is not a copy: two identities may give the same value */
void narrowed(void) { fx_consume((unsigned char)g_cell.id); }

/* an element of a global array: the array is the location */
int g_ids[4];
void from_array(int i) { fx_consume(g_ids[i]); }

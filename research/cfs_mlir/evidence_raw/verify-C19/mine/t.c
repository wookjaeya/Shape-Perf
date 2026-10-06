struct S { int a; };
static int g;
int f(struct S *s, int x) { if (x > 0) g = s->a + x; return g; }

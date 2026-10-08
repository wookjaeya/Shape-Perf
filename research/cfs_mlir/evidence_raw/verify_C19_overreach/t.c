struct S { int a; };
int g;
int f(struct S *s, int x) { if (x) g = s->a; return g + x; }

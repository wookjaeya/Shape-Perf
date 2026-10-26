/* kzgcc_plain.c -- a unit no site names: the plugin must leave it byte-identical (unit test). */
int plain_add(int a, int b) { return a + b; }
int plain_loop(const int *p, int n) { int s = 0; for (int i = 0; i < n; ++i) s += p[i] * 3; return s; }

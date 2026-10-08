typedef void (*Handler_t)(const void *);
void H1(const void *m); void H2(const void *m);
static const Handler_t Table[2] = { H1, H2 };
int g;
void H1(const void *m) { g = 1; }
void H2(const void *m) { g = 2; }
void Dispatch(unsigned i, const void *m) { if (i < 2) Table[i](m); }

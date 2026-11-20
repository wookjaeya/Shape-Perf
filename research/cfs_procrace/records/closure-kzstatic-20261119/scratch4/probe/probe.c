#include <string.h>
typedef struct { char name[8]; int n; } Rec;
typedef struct { char other[8]; int k; } Rec2;
typedef struct { const char *name_entry; int active; } Com;
Rec recs[4];
Rec2 recs2[4];
Com com[4];
static void wr(char *p) { p[0] = 'x'; }
static void wr2(char *p) { *p = 'y'; }
static int rd(const char *p) { return p[0] == 'a'; }
static void cp(char *d, const char *s) { int i; for (i = 0; i < 7; i++) d[i] = s[i]; }
int use(int i) {
  recs[i].name[1] = 0;
  recs2[i].other[2]++;
  com[i].name_entry = recs[i].name;
  wr(recs2[i].other);
  wr2(recs2[i].other);
  return rd(com[i].name_entry);
}
int use2(char *d, int i) { cp(d, com[i].name_entry); return 0; }

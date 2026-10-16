/* Second unit: a table whose initializer the basis records, and a configuration constant. */
typedef struct {
  const char *name;
  int stack;
} Item;

#define ITEM_LIMIT 40
const Item TBL[] = {{"first", 1}, {"second", ITEM_LIMIT}, {0, 0}};
int limit(void) { return ITEM_LIMIT; }

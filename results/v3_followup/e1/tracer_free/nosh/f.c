#define CAT(a, b) a##b
#define XCAT(a, b) CAT(a, b)
int XCAT(main_graph_, TAG)(void) { return VALUE; }
int XCAT(run_main_graph_, TAG)(void) { return XCAT(main_graph_, TAG)(); }

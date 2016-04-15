#include <stddef.h>

void *gcmalloc(size_t Size);
void  gcfree(void *Pointer);
void  gcdebug();
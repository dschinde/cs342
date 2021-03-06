#include "gc.h"
#include "malloc.h"

void *gcmalloc(size_t Size)
{
	return GCGetBuffer(GCAlloc(Size));
}

void gcdebug()
{
	GCListUsedObjects();
	GCListFreeObjects();
}

void gccollect()
{
	GCCollect();
}
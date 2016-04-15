#include "gc.h"
#include "malloc.h"

void *gcmalloc(size_t Size)
{
	return GCGetBuffer(GCAlloc(Size));
}

void gcfree(void *Pointer)
{
	//GCFree(GCGetObject(Pointer));
	GCMarkObject(GCGetObject(Pointer));
	GCSweep();
}

void gcdebug()
{
	GCListUsedObjects();
}
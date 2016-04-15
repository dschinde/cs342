#include <stddef.h>

typedef struct GCObject GCObject;

void 	 *GCGetBuffer(GCObject *Object);
GCObject *GCGetObject(void *Buffer);

GCObject *GCAlloc(size_t Size);
void 	  GCFree(GCObject *Object);

void 	  GCListUsedObjects();

void	  GCMarkObject(GCObject *Object);
void      GCSweep();
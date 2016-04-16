#include <stddef.h>

typedef struct GCObject GCObject;

void 	 *GCGetBuffer(GCObject *Object);
GCObject *GCGetObject(void *Buffer);

GCObject *GCAlloc(size_t Size);

void 	  GCListUsedObjects();

void	  GCCollect();
#include <stddef.h>

typedef struct GCList GCList;
typedef struct GCObject GCObject;

#define GCNoSuchObject (size_t)-1

GCList   *GCCreateList();
void      GCDestroyList(GCList *List);

void 	  GCPushList(GCList *List, GCObject *Object);
GCObject *GCPopList(GCList *List, size_t Index);

size_t    GCQueryListSize(const GCList *List);
GCObject *GCGetListEntry(const GCList *List, size_t Index);
size_t    GCIndexOfList(
	const GCList *List, 
	const GCObject *Object);
	
void GCPinList(GCList *List);
void GCUnpinList(GCList *List);
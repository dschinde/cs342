#include <stddef.h>

/*
 * A GCList an unordered collection of pointers.
 * Elements are inserted with GCPushList() and
 * removed with GCPopList().
 *
 * To iterate through a GCList:
 *    size_t ListSize = GCQueryListSize(List);
 *    for (size_t i = 0; i < ListSize; ++i)
 *    {
 *        GCObject *Object = GCGetListEntry(List, i);
 *       // Do something with the object
 *    }
 *
 * Any call to GCPushList() or GCPopList() while
 * iterating through a GCList causes unspecified
 * behavior. There is no safe way to insert items
 * while iterating.
 * If items are to be removed while iterating, call
 * GCPinList() before starting the loop. GCPinList()
 * prevents the GCList's elements from moving, which
 * normally only happends when an item is removed.
 * Call GCUnpinList() after the loop.
 */

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

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "log.h"

#define MinimumAllocationSize (24 - sizeof(GCObject))

static GCList *FreeBlocks = NULL;
static GCList *UsedBlocks = NULL;

static void *MemoryBase = NULL;
static void *MemoryEnd  = NULL;

typedef struct GCObject {
	size_t Size;
	bool Marked;
} GCObject;

static void GCFreeByIndex(GCObject *Object, size_t Index);

void *GCGetBuffer(GCObject *Object)
{
	return (void*)(Object + 1);
}

GCObject *GCGetObject(void *Buffer)
{
	return ((GCObject*)Buffer) - 1;
}

void GCInitialize()
{
	static bool Initialized = false;
	
	if (!Initialized) {
		FreeBlocks = GCCreateList();
		UsedBlocks = GCCreateList();
	
		size_t MemorySize = 1024 * 1024;
		size_t ByteCount = sizeof(GCObject) + MemorySize;
		
		GCObject *Block = malloc(ByteCount);
		assert(Block);
		
		MemoryBase = Block;
		MemoryEnd = Block + ByteCount;
		
		Block->Size = MemorySize;
		GCPushList(FreeBlocks, Block);
		
		Initialized = true;
	}
}

GCObject *GCAlloc(size_t Size)
{
	GCDebug("Size = %lu", Size);
	Size = Size < MinimumAllocationSize ? 
		MinimumAllocationSize : Size;
	if (Size % 8 != 0) {
		Size += (Size % 8);
	}
	GCDebug("Adjusted size = %lu", Size);
	
	GCInitialize();
	
	size_t ListSize = GCQueryListSize(FreeBlocks);
	for (size_t i = 0; i < ListSize; ++i) {
		GCObject *Object = GCGetListEntry(FreeBlocks, i);
		
		if (Size < Object->Size) {
			GCPopList(FreeBlocks, i);
			
			GCDebug("Found object %p with size = %lu",
				Object, Object->Size);
			
			if (MinimumAllocationSize < Object->Size - Size)
			{
				GCDebug("Splitting object %p", Object);
				GCObject *Next = (GCObject*)((char*)GCGetBuffer(Object) + Size);
				GCDebug("++ New object at %p", Next);
				Next->Size = Object->Size - Size - sizeof(GCObject);
				GCDebug("++ New object size is %lu", Next->Size);
				Object->Size -= (Next->Size + sizeof(GCObject));
				GCDebug("++ Found object size is now %lu", Object->Size);
					
				GCPushList(FreeBlocks, Next);
			}
			
			Object->Marked = false;
			memset(GCGetBuffer(Object), 0, Object->Size);
			GCPushList(UsedBlocks, Object);
			
			return Object;
		}
	}
	
	GCError("Unable to allocate %lu bytes.", Size);
	return NULL;
}

void GCFree(GCObject *Object)
{
	assert(Object);
	
	size_t Index = GCIndexOfList(UsedBlocks, Object);
	assert(Index != GCNoSuchObject);
	
	GCFreeByIndex(Object, Index);
}

void GCFreeByIndex(GCObject *Object, size_t Index)
{
	GCDebug("Freeing object %p with size %lu", 
		Object, Object->Size);
		
	GCPopList(UsedBlocks, Index);
	GCPushList(FreeBlocks, Object);
}

bool IsGCBuffer(void *Pointer)
{
	if (Pointer == NULL) {
		return false;
	}
	
	if (MemoryBase < Pointer && Pointer < MemoryEnd)
	{
		GCObject *Object = GCGetObject(Pointer);
		size_t Index = GCIndexOfList(UsedBlocks, Object);
		return Index != GCNoSuchObject;
	}
	
	return false;
}

void GCMarkObject(GCObject *Object)
{
	assert(Object);
	
	GCDebug("Marking object %p", Object);
	
	Object->Marked = true;
	
	void **Buffer = GCGetBuffer(Object);
	
	for (size_t i = 0; i < Object->Size / sizeof(void*); ++i)
	{
		if (IsGCBuffer(Buffer[i])) {
			GCObject *Child = GCGetObject(Buffer[i]);
			
			GCDebug("++ Found child object %p", Child);
			
			if (!Child->Marked) {
				GCMarkObject(Child);
			}
		}
	}
}

void GCSweep()
{
	GCPinList(UsedBlocks);
	size_t ListSize = GCQueryListSize(UsedBlocks);
	
	for (size_t i = 0; i < ListSize; ++i) {
		GCObject *Object = GCGetListEntry(UsedBlocks, i);
		if (Object->Marked) {
			GCFreeByIndex(Object, i);
		}
	}
	
	GCUnpinList(UsedBlocks);
}

void GCListUsedObjects()
{
	size_t ListSize = GCQueryListSize(UsedBlocks);
	for (size_t i = 0; i < ListSize; ++i) {
		GCObject *Object = GCGetListEntry(UsedBlocks, i);
		GCTrace("Object: %p, Size: %lu", 
			Object, 
			Object->Size);
	}
}
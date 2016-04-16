#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

static void GCInitialize();
static void GCFreeByIndex(GCObject *Object, size_t Index);
static void GCMarkObject(GCObject *Object);
static void GCSweep();
static void GCCompactBlocks(GCList *List);
static void GCReportBlocks(const GCList *List);
static bool IsGCBuffer(void *Buffer);

void *GCGetBuffer(GCObject *Object)
{
	return (void*)(Object + 1);
}

GCObject *GCGetObject(void *Buffer)
{
	return ((GCObject*)Buffer) - 1;
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

void GCReportBlocks(const GCList *List)
{
	size_t ListSize = GCQueryListSize(List);
	for (size_t i = 0; i < ListSize; ++i) {
		GCObject *Object = GCGetListEntry(List, i);
		GCTrace("++ Object: %p, Size: %lu", 
			Object, 
			Object->Size);
	}
}

void GCListUsedObjects()
{
	GCTrace("Used Objects");
	GCReportBlocks(UsedBlocks);
}

void GCListFreeObjects()
{
	GCTrace("Free Objects");
	GCReportBlocks(FreeBlocks);
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

void GCFreeByIndex(GCObject *Object, size_t Index)
{
	assert(Object);
	
	GCTrace("Freeing object %p with size %lu", 
		Object, Object->Size);
		
	GCPopList(UsedBlocks, Index);
	GCPushList(FreeBlocks, Object);
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
			Object->Marked = false;
		}
		else {
			GCFreeByIndex(Object, i);
		}
	}
	
	GCUnpinList(UsedBlocks);
}

bool IsGCBuffer(void *Buffer)
{
	if (Buffer == NULL) {
		return false;
	}
	
	if (MemoryBase < Buffer && Buffer < MemoryEnd)
	{
		GCObject *Object = GCGetObject(Buffer);
		size_t Index = GCIndexOfList(UsedBlocks, Object);
		return Index != GCNoSuchObject;
	}
	
	return false;
}

static void GCCompactBlocks(GCList *List)
{
	assert(List);
	
	GCSortList(List);
	GCPinList(List);
	
	size_t ListSize = GCQueryListSize(List);
	
	if (ListSize < 2) {
		return;
	}
	
	GCObject *Current = GCGetListEntry(List, 0);
	size_t NextIndex = 1;
	
	for (; NextIndex < ListSize; ++NextIndex)
	{
		GCObject *Next = GCGetListEntry(List, NextIndex);
		size_t Offset = sizeof(GCObject) + Current->Size;
		if ((char*)Current + Offset == (char*)Next) {
			GCTrace("Combining object %p of size %lu with"
				    " object %p of size %lu.",
					Current, Current->Size,
					Next, Next->Size);
					
			Current->Size += Offset;
			GCPopList(List, NextIndex);
		}
		else {
			Current = Next;
		}
	}
	
	GCUnpinList(List);
}

static void *GetStackBase()
{
	const char *Format =
		"%*d %*s %*c %*d %*d %*d %*d %*d %*u %*lu "
		"%*lu %*lu %*lu %*lu %*lu %*ld %*ld %*ld %*ld %*ld "
		"%*ld %*llu %*lu %*ld %*lu %*lu %*lu %lu %*lu %*lu ";

	FILE *Stat = fopen("/proc/self/stat", "r");
	assert(Stat);
	
	uint64_t StackBase = 0;
	
	if (fscanf(Stat, Format, &StackBase) != 1) {
		assert(false);
	}
	
	fclose(Stat);
	
	return (void*)StackBase;
}

void GCCollect()
{
	register void *rsp asm("rsp");
	void *StackBase = GetStackBase();
			
	void **Stack = rsp + ((uintptr_t)rsp % sizeof(void*));
	
	GCDebug("rsp is %p and the stack base is %p",
		rsp,
		StackBase);
	GCDebug("Scanning from %p", Stack);
	
	for (; (void*)Stack < StackBase; ++Stack)
	{
		if (IsGCBuffer(*Stack)) {
			GCObject *Object = GCGetObject(*Stack);
			GCMarkObject(Object);
		}
	}
	
	register void *r12 asm("r12");
	register void *r13 asm("r13");
	register void *r14 asm("r14");
	register void *r15 asm("r15");
	register void *rdi asm("rdi");
	register void *rsi asm("rsi");
	register void *rbx asm("rbx");
	register void *rbp asm("rbp");
	
#define CheckRegister(Register)					  \
	if (IsGCBuffer(Register)) {					  \
		GCObject *Object = GCGetObject(Stack);	  \
		GCMarkObject(Object);					  \
	}
	
	CheckRegister(r12);
	CheckRegister(r13);
	CheckRegister(r14);
	CheckRegister(r15);
	CheckRegister(rdi);
	CheckRegister(rsi);
	CheckRegister(rbx);
	CheckRegister(rbp);
	
	GCSweep();
	GCCompactBlocks(FreeBlocks);
}
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "list.h"

#define GC_LIST_INITIAL_CAPACITY 10

typedef struct GCList {
	size_t Size;
	size_t Capacity;
	bool Pinned;
	GCObject **Buffer;
} GCList;

GCList *GCCreateList()
{
	GCList *List = malloc(sizeof(GCList));
	assert(List);
	
	List->Size = 0;
	List->Capacity = GC_LIST_INITIAL_CAPACITY;
	List->Pinned = false;
	List->Buffer = malloc(sizeof(GCObject*) * 
		GC_LIST_INITIAL_CAPACITY);
	assert(List->Buffer);
	
	return List;
}

void GCDestroyList(GCList *List)
{
	assert(List);
	
	free(List->Buffer);
	free(List);
}

void GCPushList(GCList *List, GCObject *Object)
{
	assert(List);
	assert(!List->Pinned);
	
	if (List->Size == List->Capacity) {
		GCObject **Buffer = realloc(List->Buffer, 
			sizeof(GCObject*) * List->Capacity * 2);
		assert(Buffer);
		List->Buffer = Buffer;
		List->Capacity *= 2;
	}
	
	List->Buffer[List->Size] = Object;
	++List->Size;
}

GCObject *GCPopList(GCList *List, size_t Index)
{
	assert(List);
	assert(Index < List->Size);
	
	GCObject *Object = List->Buffer[Index];
	if (List->Pinned) {
		List->Buffer[Index] = NULL;
	}
	else {
		List->Buffer[Index] = List->Buffer[List->Size - 1];
		List->Buffer[List->Size - 1] = NULL;
		--List->Size;
	}
	
	return Object;
}

size_t GCQueryListSize(const GCList *List)
{
	assert(List);
	
	return List->Size;
}

GCObject *GCGetListEntry(const GCList *List, size_t Index)
{
	assert(List);
	assert(Index < List->Size);
	
	return List->Buffer[Index];
}

size_t GCIndexOfList(
	const GCList *List, 
	const GCObject *Object)
{
	assert(List);
	assert(Object);
	
	for (size_t i = 0; i < List->Size; ++i) {
		if (List->Buffer[i] == Object) {
			return i;
		}
	}
	
	return GCNoSuchObject;
}

void GCPinList(GCList *List)
{
	assert(List);
	assert(!List->Pinned);
	
	List->Pinned = true;
}

void GCUnpinList(GCList *List)
{
	assert(List);
	assert(List->Pinned);
	
	List->Pinned = false;
	
	if (List->Size == 0) {
		return;
	}
	
	size_t WriteIndex = 0;
	size_t ReadIndex  = List->Size - 1;
	
	for (; WriteIndex < ReadIndex; ++WriteIndex) {
		if (List->Buffer[WriteIndex] == NULL) {
			List->Buffer[WriteIndex] = List->Buffer[ReadIndex];
			List->Buffer[ReadIndex] = NULL;
			
			for (; List->Buffer[ReadIndex] == NULL 
				&& WriteIndex < ReadIndex; 
				--ReadIndex)
			{
			}
		}
	}
	
	if (List->Buffer[0] == NULL) {
		List->Size = 0;
	}
	else {
		for (; List->Buffer[List->Size - 1] == NULL; --List->Size)
		{
		}
	}
}
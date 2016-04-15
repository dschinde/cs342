/*
 * See MdGetPaddingAfter() and MdGetUnusedBytes()
 */
 
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
 * A linked list of chunks of available memory.
 * Note that all the memory (e.g. the begin pointers) are
 * allocated out of a single contiguous block of memory.
 */
typedef struct MD_CHUNK {
    struct MD_CHUNK *Next;  /* Next chunk of memory */
    size_t  BytesUsed, 		/* Num currently used bytes, or -1 if it is free */
            MaxBytes;      	/* Max num bytes that can be used in this chunk */
    int Line;       		/* The line number which allocated the memory */
    unsigned char *Buffer,  /* Beginning of the chunk (starts with padding) */
				  *Usable;  /* Usable beginning (after padding). */
	bool Marked;
} MD_CHUNK;


/*
 * Number of bytes of padding before and after memory.
 */
#define     PADDING_SIZE            256


/*
 * The byte to be used for all padding segments.
 */
#define     PADDING_BYTE            0x55


/*
 * Four sizes for memory chunks.
 */
static int CHUNK_SIZES[] = { 1024, 2048, 4096, 8192 };

/*
 * Size of previous array.
 */
#define NUM_CHUNK_SIZES     4

#define MD_BYTES_USED_FREE  (size_t)-1


typedef struct MD_LIST {
	MD_CHUNK *First;
} MD_LIST;

static MD_LIST FreeChunks = { .First = NULL };
static MD_LIST UsedChunks = { .First = NULL };

static void MdRemoveFromList(
	MD_LIST  *List,
	MD_CHUNK *Chunk,
	MD_CHUNK *PreviousChunk)
{
	assert(List);
	assert(Chunk);
	assert(Chunk == List->First || PreviousChunk != NULL);
	
	if (PreviousChunk) {
		PreviousChunk->Next = Chunk->Next;
	} else {
		List->First = Chunk->Next;
	}
	
	Chunk->Next = NULL;
}
	
static void MdAppendToList(
	MD_LIST  *List,
	MD_CHUNK *Chunk)
{
	assert(List);
	assert(Chunk);
	
	Chunk->Next = List->First;
	List->First = Chunk;
}

#define MdForEach(List, Chunk, PreviousChunk)	\
for (MD_CHUNK *Chunk = (List)->First, 			\
			  *PreviousChunk = NULL;			\
	 Chunk != NULL;								\
	 PreviousChunk = Chunk,						\
	 Chunk = Chunk->Next)

/*
 * The single block of memory allocated and available.
 * Note: this will be the same as first_chunk->beg.
 */
static unsigned char *Block = NULL;


static MD_CHUNK *MdCreateChunk(
	unsigned char *Buffer, 
	size_t Size);

static void *MdAllocateBuffer(size_t Size, int Line);
static void MdFreeBuffer(void *Buffer, int Line);
static void MdCollectGarbage(void **Active, size_t ActiveCount);
static int MdReportActiveChunks();

void xinit(size_t Size) {
    Block = malloc(Size);
	assert(Block);
	
    unsigned char *UpperLimit = Block;
    unsigned char *pb = Block;

    for (int i = 0; i < NUM_CHUNK_SIZES; i++) {
        size_t ChunkSize = CHUNK_SIZES[i];
        UpperLimit += Size / 4;

        while (pb + ChunkSize <= UpperLimit) {
            MD_CHUNK *Chunk = MdCreateChunk(pb, ChunkSize);
            
			MdAppendToList(&FreeChunks, Chunk);
			
            pb += ChunkSize;
        }
    }
}

void xshutdown() {
    free(Block);
    Block = NULL;
	
	MdForEach(&FreeChunks, Next, Current) {
		if (Current) {
			free(Current);
		}
	}
	
    FreeChunks.First = NULL;
}


void *_xmalloc(int nbytes, int line_num) {
	return MdAllocateBuffer(nbytes, line_num);
}


void _xfree(void *vp, int line_num) {
	MdFreeBuffer(vp, line_num);
}


int xdebug() {
	return MdReportActiveChunks();
}


static MD_CHUNK *MdCreateChunk(
	unsigned char *Buffer, 
	size_t Size)
{
    MD_CHUNK *Chunk = malloc(sizeof(MD_CHUNK));
	assert(Chunk);

    Chunk->BytesUsed = MD_BYTES_USED_FREE;
    Chunk->Line = 0;
    Chunk->MaxBytes = Size - 2 * PADDING_SIZE;
    Chunk->Next = NULL;
    Chunk->Buffer = Buffer;
    Chunk->Usable = Buffer + PADDING_SIZE;

	memset(Chunk->Buffer, PADDING_BYTE, PADDING_SIZE);

    return Chunk;
}

static bool MdIsChunkFree(const MD_CHUNK *Chunk)
{
	assert(Chunk);
	
	return Chunk->BytesUsed == MD_BYTES_USED_FREE;
}

static void *MdGetPaddingAfter(const MD_CHUNK *Chunk)
{
    assert(Chunk);
    assert(!MdIsChunkFree(Chunk));
 
    return Chunk->Usable + Chunk->BytesUsed;
}

static int MdGetUnusedBytes(const MD_CHUNK *Chunk)
{
   assert(Chunk);
   assert(!MdIsChunkFree(Chunk));
   
   return Chunk->MaxBytes - Chunk->BytesUsed;
}

static void *MdAllocateBuffer(size_t Size, int Line)
{
	if (INT_MAX < Size) {
		return NULL;
	}

	MdForEach(&FreeChunks, Chunk, PreviousChunk) {
		if (Size < Chunk->MaxBytes) {
			Chunk->BytesUsed = Size;
			memset(Chunk->Usable + Size, 
				PADDING_BYTE, 
				PADDING_SIZE);
			Chunk->Line = Line;
			
			MdRemoveFromList(
				&FreeChunks,
				Chunk,
				PreviousChunk);
			
			MdAppendToList(&UsedChunks, Chunk);
			
			return Chunk->Usable;
		}
	}
	
	return NULL;
}

static void MdFreeBuffer(void *Buffer, int Line)
{
	MdForEach(&UsedChunks, Chunk, PreviousChunk) {
		if (Chunk->Usable == Buffer) {
			for (unsigned char *pb = Chunk->Buffer; 
				pb != Chunk->Usable; ++pb)
			{
				if (*pb != PADDING_BYTE) {
					fprintf(stderr,
						"Illegal write %ld bytes before %lu"
						" bytes ofmemory allocated at line"
						" %d.\n",
						Chunk->Usable - pb,
						Chunk->BytesUsed,
						Chunk->Line);
					exit(EXIT_FAILURE);
				}
			}
			
			unsigned char *PaddingAfter = MdGetPaddingAfter(Chunk);
			for (int i = 0; i < PADDING_SIZE; ++i)
			{
				if (PaddingAfter[i] != PADDING_BYTE) {
					fprintf(stderr,
						"Illegal write %d bytes after %lu"
						" bytes of memory allocated at line"
						" %d.\n",
						i + 1,
						Chunk->BytesUsed,
						Chunk->Line);
					exit(EXIT_FAILURE);
				}
			}
			
			Chunk->BytesUsed = MD_BYTES_USED_FREE;
			MdRemoveFromList(&UsedChunks, Chunk, PreviousChunk);
			MdAppendToList(&FreeChunks, Chunk);
			
			return;
		}
	}
	
	fprintf(stderr, 
		"Free requested at line %d for unknown memory"
		" segment.\n", 
		Line);
	exit(EXIT_FAILURE);
}

static int MdReportActiveChunks()
{
	int Count = 0;
	
	MdForEach(&UsedChunks, Chunk, PreviousChunk) {
		fprintf(stderr,
			"%lu bytes of unfreed memory allocated at"
			" line %d.\n",
			Chunk->BytesUsed,
			Chunk->Line);
		++Count;
	}
	
	return Count;
}

static void MdCollectGarbage(
	void **Active, 
	size_t ActiveCount)
{
	MdForEach(&UsedChunks, Chunk, PreviousChunk) {
		bool Found = false;
		for (size_t i = 0; i < ActiveCount; i++) {
			if (Active[i] == Chunk->Usable) {
				Found = true;
			}
		}
		Chunk->Marked = Found;
	}

	
}
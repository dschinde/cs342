/*
 * See MdGetPaddingAfter() and MdGetUnusedBytes()
 */
 
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
 * A linked list of chunks of available memory.
 * Note that all the memory (e.g. the begin pointers) are
 * allocated out of a single contiguous block of memory.
 */
struct memory {
    struct memory *next;    /* Next chunk of memory */
    int     bytes_used,     /* Num currently used bytes, or -1 if it is free */
            bytes_max,      /* Max num bytes that can be used in this chunk */
            line_num;       /* The line number which allocated the memory */
    char    *begin,         /* Beginning of the chunk (starts with padding) */
            *ubegin;        /* Usable beginning (after padding). */
};


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



/*
 * The root of the linked list of memory chunks.
 */
static struct memory *first_chunk = NULL;


/*
 * The single block of memory allocated and available.
 * Note: this will be the same as first_chunk->beg.
 */
static char *block = NULL;


static struct memory *create_chunk(char *begin, int bytes);


static void *MdAllocateBuffer(size_t Size, int Line);
static void MdFreeBuffer(void *Buffer, int Line);
static int MdReportActiveChunks();

#define MdIsChunkFree(chunk) (chunk->bytes_used == -1)


void xinit(size_t Size) {
    struct memory *last_chunk = NULL;
    char *upper_limit;
    char *cp;
    int  i;

    block = malloc(Size);
    upper_limit = block;
    cp = block;

    for (i = 0; i < NUM_CHUNK_SIZES; i++) {
        int chunk_size = CHUNK_SIZES[i];
        upper_limit += Size / 4;

        while (cp + chunk_size <= upper_limit) {
            struct memory *chunk = create_chunk(cp, chunk_size);
            if (first_chunk == NULL) {
                first_chunk = chunk;
            } else {
                last_chunk->next = chunk;
            }
            last_chunk = chunk;
            cp += chunk_size;
        }
    }
}

void xshutdown() {
    free(block);
    block = NULL;

    struct memory *this = first_chunk,
                  *next;

    while (this != NULL) {
        next = this->next;
        free(this);
        this = next;
    }
    first_chunk = NULL;
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


static struct memory *create_chunk(char *begin, int bytes) {
    char            *cp;
    struct memory   *mp = malloc(sizeof(struct memory));

    mp->bytes_used = -1;
    mp->line_num = 0;
    mp->bytes_max = bytes - 2 * PADDING_SIZE;
    mp->next = NULL;
    mp->begin = begin;
    mp->ubegin = begin + PADDING_SIZE;

    for (cp = mp->begin; cp < mp->ubegin; cp++) {
        *cp = PADDING_BYTE;
    }

    return mp;
}

static void *MdGetPaddingAfter(struct memory *mp)
{
    assert(mp);
    assert(!MdIsChunkFree(mp));
 
    return mp->ubegin + mp->bytes_used;
}

static int MdGetUnusedBytes(struct memory *mp)
{
   assert(mp);
   assert(!MdIsChunkFree(mp));
   
   return mp->bytes_max - mp->bytes_used;
}

static void *MdAllocateBuffer(size_t Size, int Line)
{
	if (INT_MAX < Size) {
		return NULL;
	}

	struct memory *chunk = first_chunk;
	do {
		if (MdIsChunkFree(chunk)) {
			if ((int)Size < chunk->bytes_max) {
				chunk->bytes_used = (int)Size;
				memset(chunk->ubegin + Size, 
					PADDING_BYTE, 
					PADDING_SIZE);
				chunk->line_num = Line;
				break;
			}
		}
	} while ((chunk = chunk->next));
	
	if (chunk == NULL) {
		return NULL;
	}
	
	return chunk->ubegin;
}

static void MdFreeBuffer(void *Buffer, int Line)
{
	struct memory *chunk = first_chunk;
	do {
		if (chunk->ubegin == Buffer) {
			break;
		}
	} while ((chunk = chunk->next));
	
	if (chunk == NULL) {
		fprintf(stderr, 
			"Free requested at line %d for unknown memory"
			" segment.\n", 
			Line);
		exit(EXIT_FAILURE);
	}
	
	if (MdIsChunkFree(chunk)) {
		fprintf(stderr,
			"Free requested at line %d for previously freed"
			" memory segment allocated at line %d.\n",
			Line,
			chunk->line_num);
		exit(EXIT_FAILURE);
	}
	
	for (char *pb = chunk->begin; pb != chunk->ubegin; ++pb)
	{
		if (*pb != PADDING_BYTE) {
			fprintf(stderr,
				"Illegal write %ld bytes before %d bytes of"
				" memory allocated at line %d.\n",
				chunk->ubegin - pb,
				chunk->bytes_used,
				chunk->line_num);
			exit(EXIT_FAILURE);
		}
	}
	
	char *PaddingAfter = MdGetPaddingAfter(chunk);
	for (int i = 0; i < PADDING_SIZE; ++i)
	{
		if (PaddingAfter[i] != PADDING_BYTE) {
			fprintf(stderr,
				"Illegal write %d bytes after %d bytes of"
				" memory allocated at line %d.\n",
				i + 1,
				chunk->bytes_used,
				chunk->line_num);
			exit(EXIT_FAILURE);
		}
	}
	
	chunk->bytes_used = -1;
}

static int MdReportActiveChunks()
{
	int Count = 0;
	
	struct memory *chunk = first_chunk;
	do {
		if (!MdIsChunkFree(chunk)) {
			fprintf(stderr,
				"%d bytes of unfreed memory allocated at"
				" line %d.\n",
				chunk->bytes_used,
				chunk->line_num);
			++Count;
		}
	} while ((chunk = chunk->next));
	
	return Count;
}
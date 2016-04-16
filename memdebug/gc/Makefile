CC=gcc
CFLAGS=-Wall -g
GC_LIBS=list.o log.o malloc.o gc.o

test: gc test.c
	$(CC) $(CFLAGS) test.c $(GC_LIBS) -o test
	
# Mark the 'gc' target as phony
.PHONY: gc

list.o: list.c
	$(CC) $(CFLAGS) -c list.c
	
log.o: log.c
	$(CC) $(CFLAGS) -c log.c
	
malloc.o: malloc.c
	$(CC) $(CFLAGS) -c malloc.c
	
gc.o: gc.c
	$(CC) $(CFLAGS) -c gc.c
	
gc: list.o log.o malloc.o gc.o

clean:
	rm -f *.o
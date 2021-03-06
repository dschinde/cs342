#include <stdio.h>

#include "malloc.h"

struct A;
struct B;
struct C;
struct D {
	long x;
	struct C *c;
};

struct C {
	struct D *d;
};

struct B {
	long x;
};

struct A {
	struct A *a;
	struct B *b;
	struct C *c;
	struct D *d;
};

struct A *Allocate()
{
	struct A *a = gcmalloc(sizeof(struct A));
	a->a = NULL;
	a->b = gcmalloc(sizeof(struct B));
	a->c = gcmalloc(sizeof(struct C));
	a->d = gcmalloc(sizeof(struct D));
	a->c->d = gcmalloc(sizeof(struct D));
	a->d->c = a->c;
	a->c->d->c = a->c;
	
	return a;
}

void **AllocateArray()
{
	void **a = gcmalloc(sizeof(void*) * 2);
	a[0] = gcmalloc(16);
	a[1] = gcmalloc(32);
	
	return a;
}

long leak()
{
	void *a = gcmalloc(100);
	void *b = gcmalloc(200);
	void **x = AllocateArray();
	
	return (long)a + (long)b ^ (long)x;
}

void leak2()
{
	struct A *a = Allocate();
	printf("%p\n", a);
}

int main(int argc, char *argv[])
{
	//struct A *a = Allocate();
	void **arr = AllocateArray();
	
	leak();
	leak2();
	
	gccollect();
	gcdebug();

	return 0;
}
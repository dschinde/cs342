#include <stdarg.h>
#include <stdio.h>

#include "log.h"

void GCTraceX(const char *Function, const char *Format, ...)
{
	va_list args;
	va_start(args, Format);
	
	printf("[%s] ", Function);
	vprintf(Format, args);
	printf("\n");
	
	va_end(args);
}

void GCDebugX(const char *Function, const char *Format, ...)
{
	va_list args;
	va_start(args, Format);
	
	printf("[%s] ", Function);
	vprintf(Format, args);
	printf("\n");
	
	va_end(args);
}

void GCErrorX(const char *Function, const char *Format, ...)
{
	va_list args;
	va_start(args, Format);
	
	fprintf(stderr, "[%s] ", Function);
	vfprintf(stderr, Format, args);
	fprintf(stderr, "\n");
	
	va_end(args);
}
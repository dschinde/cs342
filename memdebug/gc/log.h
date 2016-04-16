void GCTraceX(const char *Function, const char *Format, ...)
	__attribute__((format(printf, 2, 3)));
	
void GCDebugX(const char *Function, const char *Format, ...)
	__attribute__((format(printf, 2, 3)));
	
void GCErrorX(const char *Function, const char *Format, ...)
	__attribute__((format(printf, 2, 3)));
	
#define GCTrace(Format, ...) GCTraceX(__func__, Format, ##__VA_ARGS__)
#define GCDebug(Format, ...) GCDebugX(__func__, Format, ##__VA_ARGS__)
#define GCError(Format, ...) GCDebugX(__func__, Format, ##__VA_ARGS__)
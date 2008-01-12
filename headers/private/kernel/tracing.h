#ifndef KERNEL_UTIL_TRACING_H
#define KERNEL_UTIL_TRACING_H


#include <SupportDefs.h>


#define ENABLE_TRACING 0
#define MAX_TRACE_SIZE 1024 * 1024

struct trace_entry {
	uint16  size;
	uint16  flags;
};

#ifdef __cplusplus

#include <new>

class TraceEntry : trace_entry {
	public:
		TraceEntry();
		virtual ~TraceEntry();

		virtual void Dump();

		size_t Size() const { return size; }
		uint16 Flags() const { return flags; }

		void Initialized();

		void* operator new(size_t size);
};

#endif	// __cplusplus

#ifdef __cplusplus
extern "C"
#endif
status_t tracing_init(void);

#endif	/* KERNEL_UTIL_TRACING_H */

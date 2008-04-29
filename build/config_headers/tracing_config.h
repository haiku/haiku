#ifndef KERNEL_TRACING_CONFIG_H
#define KERNEL_TRACING_CONFIG_H

// general settings

// enable tracing (0/1)
#ifndef ENABLE_TRACING
#	define ENABLE_TRACING 0
#endif

// tracing buffer size (in bytes)
#ifndef MAX_TRACE_SIZE
#	define MAX_TRACE_SIZE (20 * 1024 * 1024)
#endif


#if ENABLE_TRACING

// macros specifying the tracing level for individual components (0 is disabled)

#define BMESSAGE_TRACING						0
#define BLOCK_CACHE_TRANSACTION_TRACING			0
#define KERNEL_HEAP_TRACING						0
#define PAGE_ALLOCATION_TRACING					0
#define PARANOIA_TRACING						0
#define PARANOIA_TRACING_STACK_TRACE			0	/* stack traced depth */
#define OBJECT_CACHE_TRACING					0
#define NET_BUFFER_TRACING						0
#define RUNTIME_LOADER_TRACING					0
#define SIGNAL_TRACING							0
#define SYSCALL_TRACING							0
#define SYSCALL_TRACING_IGNORE_KTRACE_OUTPUT	1
#define TCP_TRACING								0
#define TEAM_TRACING							0
#define USER_MALLOC_TRACING						0

#endif	// ENABLE_TRACING

#endif	// KERNEL_TRACING_CONFIG_H

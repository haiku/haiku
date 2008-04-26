#ifndef KERNEL_TRACING_CONFIG_H
#define KERNEL_TRACING_CONFIG_H

// general settings

// enable tracing (0/1)
#ifndef ENABLE_TRACING
#	define ENABLE_TRACING 0
#endif

// tracing buffer size (in bytes)
#ifndef MAX_TRACE_SIZE
#	define MAX_TRACE_SIZE (1024 * 1024)
#endif


// macros that enable tracing for individual components

//#define BMESSAGE_TRACING
//#define BLOCK_CACHE_TRANSACTION_TRACING
//#define KERNEL_HEAP_TRACING
//#define PAGE_ALLOCATION_TRACING
//#define OBJECT_CACHE_TRACING
//#define RUNTIME_LOADER_TRACING
//#define SIGNAL_TRACING
//#define SYSCALL_TRACING
//#define SYSCALL_TRACING_IGNORE_KTRACE_OUTPUT
//#define TEAM_TRACING
//#define USER_MALLOC_TRACING

#endif	// KERNEL_TRACING_CONFIG_H

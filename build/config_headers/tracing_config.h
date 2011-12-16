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

#define AHCI_PORT_TRACING						0
#define ATA_DMA_TRACING							0
#define ATA_TRACING								0
#define ATAPI_TRACING							0
#define BFS_TRACING								0
#define BLOCK_CACHE_BLOCK_TRACING				0
#define BLOCK_CACHE_TRANSACTION_TRACING			0
#define BMESSAGE_TRACING						0
#define FILE_DESCRIPTOR_TRACING					0
#define FILE_DESCRIPTOR_TRACING_STACK_TRACE		0	/* stack trace depth */
#define GUARDED_HEAP_TRACING					0
#define GUARDED_HEAP_TRACING_STACK_TRACE		0	/* stack trace depth */
#define IO_CONTEXT_TRACING						0
#define IO_CONTEXT_TRACING_STACK_TRACE			0	/* stack trace depth */
#define KERNEL_HEAP_TRACING						0
#define KTRACE_PRINTF_STACK_TRACE				0	/* stack trace depth */
#define NET_BUFFER_TRACING						0
#define NET_BUFFER_TRACING_STACK_TRACE			0	/* stack trace depth */
#define PAGE_ALLOCATION_TRACING					0
#define PAGE_ALLOCATION_TRACING_STACK_TRACE		0	/* stack trace depth */
#define PAGE_DAEMON_TRACING						0
#define PAGE_STATE_TRACING						0
#define PAGE_STATE_TRACING_STACK_TRACE			0	/* stack trace depth */
#define PAGE_WRITER_TRACING						0
#define PARANOIA_TRACING						0
#define PARANOIA_TRACING_STACK_TRACE			0	/* stack trace depth */
#define PORT_TRACING							0
#define RUNTIME_LOADER_TRACING					0
#define SCHEDULER_TRACING						0
#define SCHEDULING_ANALYSIS_TRACING				0
#define SIGNAL_TRACING							0
#define SLAB_MEMORY_MANAGER_TRACING				0
#define SLAB_MEMORY_MANAGER_TRACING_STACK_TRACE	0	/* stack trace depth */
#define SLAB_OBJECT_CACHE_TRACING				0
#define SLAB_OBJECT_CACHE_TRACING_STACK_TRACE	0	/* stack trace depth */
#define SWAP_TRACING							0
#define SYSCALL_TRACING							0
#define SYSCALL_TRACING_IGNORE_KTRACE_OUTPUT	1
#define TCP_TRACING								0
#define TEAM_TRACING							0
#define USER_MALLOC_TRACING						0
#define VFS_PAGES_IO_TRACING					0
#define VM_CACHE_TRACING						0
#define VM_CACHE_TRACING_STACK_TRACE			0	/* stack trace depth */
#define VM_PAGE_FAULT_TRACING					0
#define WAIT_FOR_OBJECTS_TRACING				0

#endif	// ENABLE_TRACING

#endif	// KERNEL_TRACING_CONFIG_H

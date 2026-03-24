#ifndef KERNEL_DEBUG_CONFIG_H
#define KERNEL_DEBUG_CONFIG_H

// Master switch:
// 0: Disables all debug code that hasn't been enabled otherwise.
// 1: Enables some lightweight debug code.
// 2: Enables more debug code. Will impact performance.
#define KDEBUG_LEVEL 2

#define KDEBUG_LEVEL_2	(KDEBUG_LEVEL >= 2)
#define KDEBUG_LEVEL_1	(KDEBUG_LEVEL >= 1)
#define KDEBUG_LEVEL_0	(KDEBUG_LEVEL >= 0)


// general kernel debugging

// Enables kernel ASSERT()s and various checks, locking primitives aren't
// benaphore-style.
#define KDEBUG							KDEBUG_LEVEL_2

// Size of the heap used by the kernel debugger.
#define KDEBUG_HEAP						(64 * 1024)

// Set to 0 to disable support for kernel breakpoints.
#define KERNEL_BREAKPOINTS				1

// Enables the debug syslog feature (accessing the previous syslog in the boot
// loader) by default. Can be overridden in the boot loader menu.
#define KDEBUG_ENABLE_DEBUG_SYSLOG		KDEBUG_LEVEL_1


// block/file cache

// Enables debugger commands.
#define DEBUG_BLOCK_CACHE				KDEBUG_LEVEL_1

// Enables checks that non-dirty blocks really aren't changed. Seriously
// degrades performance when the block cache is used heavily.
#define BLOCK_CACHE_DEBUG_CHANGED		KDEBUG_LEVEL_2

// Enables a global list of file maps and related debugger commands.
#define DEBUG_FILE_MAP					KDEBUG_LEVEL_1


// heap / slab

// Initialize newly allocated memory with something non zero.
#define PARANOID_KERNEL_MALLOC			KDEBUG_LEVEL_2

// Check for double free, and fill freed memory with 0xdeadbeef.
#define PARANOID_KERNEL_FREE			KDEBUG_LEVEL_2

// Validate sanity of the heap after each operation (slow!).
#define PARANOID_HEAP_VALIDATION		0

// Store size, thread and team info at the end of each allocation block.
// Enables the "allocations*" debugger commands.
#define KERNEL_HEAP_LEAK_CHECK			0

// Enables the "allocations*" debugger commands for the slab.
#define SLAB_ALLOCATION_TRACKING		0


// interrupts

// Adds statistics and unhandled counter per interrupts. Enables the "ints"
// debugger command.
#define DEBUG_INTERRUPTS				KDEBUG_LEVEL_1


// semaphores

// Enables tracking of the last threads that acquired/released a semaphore.
#define DEBUG_SEM_LAST_ACQUIRER			KDEBUG_LEVEL_1


// SMP

// Enables spinlock caller debugging. When acquiring a spinlock twice on a
// non-SMP machine, this will give a clue who locked it the first time.
// Furthermore (also on SMP machines) the "spinlock" debugger command will be
// available.
#define DEBUG_SPINLOCKS					KDEBUG_LEVEL_2


// VM

// Enables swap support.
#define ENABLE_SWAP_SUPPORT				1

// Enables the vm_page::queue field, i.e. it is tracked which queue the page
// should be in.
#define DEBUG_PAGE_QUEUE				0

// Enables the vm_page::access_count field, which is used to detect invalid
// concurrent access to the page.
#ifndef __riscv
#define DEBUG_PAGE_ACCESS				KDEBUG_LEVEL_2
#endif

// Enables a global list of all vm_cache structures.
#define DEBUG_CACHE_LIST				KDEBUG_LEVEL_2

// Enables the alternative heaps to be selected at startup.
#define DEBUG_HEAPS						KDEBUG_LEVEL_2

// Sets the default heap to be used as the generic memory allocator (malloc/free)
// when DEBUG_HEAPS is enabled. (This value can be overridden via "kernel_malloc".)
//
// The possible options are:
//  - "slab": Heap implementation based on the slab allocator (for production use).
//
//  - "debug": Heap implementation with additional debugging facilities.
//
//  - "guarded": Heap implementation that allocates memory so that the end of the
//	  allocation always coincides with a page end and is followed by a guard page
//	  which is marked non-present. Out of bounds access (both read and write)
//	  therefore cause a crash (page fault). Note that this allocator is neither
//	  speed nor space efficient, indeed it wastes huge amounts of pages and address
//    space so it is quite easy to hit limits.
#define DEBUG_HEAPS_DEFAULT				slab

// If 0, disables memory re-use by default in the guarded heap (can be overridden
// via "guarded_heap_options".) This means freed virtual memory is never reused and
// stays invalid, causing any access to crash. This is a magnitude more space
// inefficient than the guarded heap itself; fully booting may not work at all due
// to address space waste.
#define DEBUG_GUARDED_HEAP_MEMORY_REUSE_DEFAULT		1

// If DEBUG_HEAPS is enabled, allow the guarded heap to replace select object_cache(s)
// (but not slab malloc ones) specified via "guarded_heap_for_object_caches".
#define GUARDED_HEAP_CAN_REPLACE_OBJECT_CACHES		1

// Replace all object caches with the chosen debug heap to force debug features.
#define USE_DEBUG_HEAPS_FOR_ALL_OBJECT_CACHES		0

// Enables additional sanity checks in the slab allocator's memory manager.
#define DEBUG_SLAB_MEMORY_MANAGER_PARANOID_CHECKS	0

// When set limits the amount of available RAM (in MB).
//#define LIMIT_AVAILABLE_MEMORY		256

// Enables tracking of page allocations.
#define VM_PAGE_ALLOCATION_TRACKING		0

// Enables the (boot) system profiler for use with "profile -r"
#define SYSTEM_PROFILER					0
#define SYSTEM_PROFILE_SIZE				40 * 1024 * 1024
#define SYSTEM_PROFILE_SCHEDULING		0
#define SYSTEM_PROFILE_STACK_DEPTH		10
#define SYSTEM_PROFILE_INTERVAL			10000


// Network

// Enables additional assertions in the tcp add-on.
#define DEBUG_TCP_BUFFER_QUEUE			KDEBUG_LEVEL_2


#endif	// KERNEL_DEBUG_CONFIG_H

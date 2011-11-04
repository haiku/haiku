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

#define DEBUG_SPINLOCK_LATENCIES		0


// VM

// Enables the vm_page::queue field, i.e. it is tracked which queue the page
// should be in.
#define DEBUG_PAGE_QUEUE				0

// Enables the vm_page::access_count field, which is used to detect invalid
// concurrent access to the page.
#define DEBUG_PAGE_ACCESS				KDEBUG_LEVEL_2

// Enables a global list of all vm_cache structures.
#define DEBUG_CACHE_LIST				KDEBUG_LEVEL_1

// Enables swap support.
#define ENABLE_SWAP_SUPPORT				1

// Use the slab allocator as generic memory allocator (malloc()/free()).
#define USE_SLAB_ALLOCATOR_FOR_MALLOC	1

// Enables additional sanity checks in the slab allocator's memory manager.
#define DEBUG_SLAB_MEMORY_MANAGER_PARANOID_CHECKS	0

// When set limits the amount of available RAM (in MB).
//#define LIMIT_AVAILABLE_MEMORY		256

// Enables tracking of page allocations.
#define VM_PAGE_ALLOCATION_TRACKING		0


#endif	// KERNEL_DEBUG_CONFIG_H

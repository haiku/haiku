#ifndef KERNEL_DEBUG_CONFIG_H
#define KERNEL_DEBUG_CONFIG_H


// general kernel debugging

// Enables kernel ASSERT()s and various checks, locking primitives aren't
// benaphore-style.
#define KDEBUG 1

// Enable this to get support for kernel breakpoints.
//#define KERNEL_BREAKPOINTS 1


// block cache

// Enables debugger commands.
#define DEBUG_BLOCK_CACHE

// Enables checks that non-dirty blocks really aren't changed. Seriously
// degrades performance when the block cache is used heavily.
#define BLOCK_CACHE_DEBUG_CHANGED


// VM

// Enables the vm_page::queue, i.e. it is tracked which queue the page should
// be in.
//#define DEBUG_PAGE_QUEUE	1

// Enables extra debug fields in the vm_page used to track page transitions
// between caches.
//#define DEBUG_PAGE_CACHE_TRANSITIONS	1

// Enables a global list of all vm_cache structures.
//#define DEBUG_CACHE_LIST 1

// Enables swap support.
#define ENABLE_SWAP_SUPPORT 1

// When set limits the amount of available RAM (in MB).
//#define LIMIT_AVAILABLE_MEMORY	256


#endif	// KERNEL_DEBUG_CONFIG_H

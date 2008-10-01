#ifndef KERNEL_DEBUG_CONFIG_H
#define KERNEL_DEBUG_CONFIG_H


// general kernel debugging

// Enables kernel ASSERT()s and various checks, locking primitives aren't
// benaphore-style.
#define KDEBUG 1


// block cache

// Enables debugger commands.
#define DEBUG_BLOCK_CACHE

// Enables checks that non-dirty blocks really aren't changed. Seriously
// degrades performance when the block cache is used heavily.
#define BLOCK_CACHE_DEBUG_CHANGED


#endif	// KERNEL_DEBUG_CONFIG_H

/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KERNEL_H
#define _KERNEL_KERNEL_H

/**
 * @file kernel/kernel.h 
 * @brief Kernel Definitions
 */

/**
 * @defgroup OpenBeOS_Kernel OpenBeOS Kernel
 * @brief The OpenBeOS Kernel
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* david - same fiddle as stage2.h. Renamed the arch specific
 * kernel.h file into arch_kernel.h and then just include
 * it here as the arch specific directories are included on the
 * include paths for the build.
 */
 
#include <arch_kernel.h>

/**
 * Size of the kernel stack
 */
#define KSTACK_SIZE (PAGE_SIZE*2)
/**
 * Size of the stack given to processes
 */
#define STACK_SIZE  (PAGE_SIZE*16)


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

/** Is bit 'b' set in 'a' */
#define CHECK_BIT(a, b) ((a) & (1 << (b)))
/** Set bit 'b' in 'a' */
#define SET_BIT(a, b) ((a) | (1 << (b)))
/** Unset bit 'b' in 'a' */
#define CLEAR_BIT(a, b) ((a) & (~(1 << (b))))

#ifdef __cplusplus
}
#endif
/** @} */

#endif /* _KERNEL_KERNEL_H */

#ifndef _KERNEL_ATOMIC_H
#define _KERNEL_ATOMIC_H

#include <ktypes.h>

/**
 * @file kernel/atomic.h
 * @brief Prototypes for kernel and user versions of atomic
 *  functions.
 */

/**
 * @defgroup Kernel_Atomic Atomic Operations
 * @ingroup OpenBeOS_Kernel
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif 

/* atomic_add(), atomic_and(), atomic_or() must 
 * match the definitions in SupportDefs.h
 */

/**
 * Atomic add (user version)
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int32 user_atomic_add(vint32 *val, int32 incr);

/**
 * Atomic and (user version)
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int32 user_atomic_and(vint32 *val, int32 incr);

/**
 * Atomic or (user version)
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int32 user_atomic_or(vint32 *val, int32 incr);

int32 user_atomic_set(vint32 *val, int32 set_to);

/* Compare the value of val with test_val. If they
 * are equal then set the value of 'val' to
 * 'set_to'
 * @note This is the user version and should not be used within
 *       the kernel.
 */
int32 user_atomic_test_and_set(vint32 *val, int32 set_to, int32 test_val);

#ifdef __cplusplus
}
#endif 
/** @} */

#endif /* _KERNEL_ATOMIC_H */
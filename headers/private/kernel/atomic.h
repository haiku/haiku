#ifndef _KERNEL_ATOMIC_H
#define _KERNEL_ATOMIC_H

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

/* XXX - atomic_set is defined as using a volatile as this stops a 
 * compiler warning, but is there any reason why they shouldn't all
 * be so defined? They were in arch/cpu.h...
 */
/**
 * Perform an atomic addition.
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int atomic_add(int *val, int incr);
/**
 * Atomic and operation
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int atomic_and(int *val, int incr);
/**
 * Atomic or operation
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int atomic_or(int *val, int incr);
int atomic_set(volatile int *val, int set_to);
/* Compare the value of val with test_val. If they
 * are equal then set the value of 'val' to
 * 'set_to'
 */
int test_and_set(int *val, int set_to, int test_val);

/**
 * Atomic add (user version)
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int user_atomic_add(int *val, int incr);
/**
 * Atomic and (user version)
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int user_atomic_and(int *val, int incr);
/**
 * Atomic or (user version)
 * @param val Pointer to an integer
 * @param incr The increment to add (may be -ve)
 * @note Returns value of val before addition
 */
int user_atomic_or(int *val, int incr);
int user_atomic_set(int *val, int set_to);
/* Compare the value of val with test_val. If they
 * are equal then set the value of 'val' to
 * 'set_to'
 * @note This is the user version and should not be used within
 *       the kernel.
 */
int user_test_and_set(int *val, int set_to, int test_val);

#ifdef __cplusplus
}
#endif 
/** @} */

#endif /* _KERNEL_ATOMIC_H */

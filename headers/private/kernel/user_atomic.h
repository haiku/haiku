/* 
** Copyright 2003, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_USER_ATOMIC_H
#define _KERNEL_USER_ATOMIC_H

/* If the architecture doesn't support atomic functions
 * in userspace, they are implemented as these syscalls.
 */

#ifdef __cplusplus
extern "C" {
#endif

void _user_atomic_set(int32 *value, int32 newValue);
int32 _user_atomic_get_and_set(int32 *value, int32 newValue);
int32 _user_atomic_test_and_set(int32 *value, int32 newValue, int32 testAgainst);
int32 _user_atomic_add(int32 *value, int32 addValue);
int32 _user_atomic_and(int32 *value, int32 andValue);
int32 _user_atomic_or(int32 *value, int32 orValue);
int32 _user_atomic_get(int32 *value);

void _user_atomic_set64(int64 *value, int64 newValue);
int64 _user_atomic_get_and_set64(int64 *value, int64 newValue);
int64 _user_atomic_test_and_set64(int64 *value, int64 newValue, int64 testAgainst);
int64 _user_atomic_add64(int64 *value, int64 addValue);
int64 _user_atomic_and64(int64 *value, int64 andValue);
int64 _user_atomic_or64(int64 *value, int64 orValue);
int64 _user_atomic_get64(int64 *value);

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_USER_ATOMIC_H */

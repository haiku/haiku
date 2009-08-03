/* 
** Copyright 2003, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_USER_ATOMIC_H
#define _KERNEL_USER_ATOMIC_H

/* If the architecture doesn't support atomic functions
 * in userspace, they are implemented as these syscalls.
 */

#ifdef __cplusplus
extern "C" {
#endif

int32 _user_atomic_set(vint32 *value, int32 newValue);
int32 _user_atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst);
int32 _user_atomic_add(vint32 *value, int32 addValue);
int32 _user_atomic_and(vint32 *value, int32 andValue);
int32 _user_atomic_or(vint32 *value, int32 orValue);	
int32 _user_atomic_get(vint32 *value);

int64 _user_atomic_set64(vint64 *value, int64 newValue);
int64 _user_atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst);
int64 _user_atomic_add64(vint64 *value, int64 addValue);
int64 _user_atomic_and64(vint64 *value, int64 andValue);
int64 _user_atomic_or64(vint64 *value, int64 orValue);	
int64 _user_atomic_get64(vint64 *value);

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_USER_ATOMIC_H */

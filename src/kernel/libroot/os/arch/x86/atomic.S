/*
** Copyright 2003, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the OpenBeOS license.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* int32	atomic_set(vint32 *value, int32 newValue) */
FUNCTION(atomic_set):
	movl		4(%esp),%edx
	movl		8(%esp),%eax
	lock
	xchg		%eax,(%edx)
	ret

/* int32	atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst) */
FUNCTION(atomic_test_and_set):
	movl		4(%esp),%edx
	movl		8(%esp),%ecx
	movl		12(%esp),%eax
	lock
	cmpxchgl	%ecx,(%edx)
	ret

/* int32	atomic_add(vint32 *value, int32 addValue) */
FUNCTION(atomic_add):
	movl		4(%esp),%edx
	movl		8(%esp),%eax
	lock
	xaddl		%eax,(%edx)
	ret

/* int32	atomic_and(vint32 *value, int32 andValue) */
FUNCTION(atomic_and):
	movl		4(%esp),%edx
_atomic_and1:
	movl		8(%esp),%ecx
	movl		(%edx),%eax
	andl		%eax,%ecx
	lock
	cmpxchgl	%ecx,(%edx)
	jnz			_atomic_and1
	ret

/* int32	atomic_or(vint32 *value, int32 orValue) */
FUNCTION(atomic_or):
	movl		4(%esp),%edx
_atomic_or1:
	movl		8(%esp),%ecx
	movl		(%edx),%eax
	orl			%eax,%ecx
	lock
	cmpxchgl	%ecx,(%edx)
	jnz			_atomic_or1
	ret
	
/* int32	atomic_read(vint32 *value) */
FUNCTION(atomic_read):
	ret

/* int64	atomic_set64(vint64 *value, int64 newValue) */
FUNCTION(atomic_set64):
	ret

/* int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
FUNCTION(atomic_test_and_set64):
	ret

/* int64	atomic_add64(vint64 *value, int64 addValue) */
FUNCTION(atomic_add64):
	ret

/* int64	atomic_and64(vint64 *value, int64 andValue) */
FUNCTION(atomic_and64):
	ret

/* int64	atomic_or64(vint64 *value, int64 orValue) */
FUNCTION(atomic_or64):
	ret

/* int64	atomic_read64(vint64 *value) */
FUNCTION(atomic_read64):
	ret

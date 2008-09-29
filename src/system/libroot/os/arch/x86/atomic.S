/*
** Copyright 2003, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the OpenBeOS license.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <asm_defs.h>


.text

/* int32	atomic_set(vint32 *value, int32 newValue) */
FUNCTION(atomic_set):
	movl		4(%esp),%edx
	movl		8(%esp),%eax
	lock
	xchg		%eax,(%edx)
	ret
FUNCTION_END(atomic_set)

/* int32	atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst) */
FUNCTION(atomic_test_and_set):
	movl		4(%esp),%edx
	movl		8(%esp),%ecx
	movl		12(%esp),%eax
	lock
	cmpxchgl	%ecx,(%edx)
	ret
FUNCTION_END(atomic_test_and_set)

/* int32	atomic_add(vint32 *value, int32 addValue) */
FUNCTION(atomic_add):
	movl		4(%esp),%edx
	movl		8(%esp),%eax
	lock
	xaddl		%eax,(%edx)
	ret
FUNCTION_END(atomic_add)

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
FUNCTION_END(atomic_and)

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
FUNCTION_END(atomic_or)

/* int32	atomic_get(vint32 *value) */
FUNCTION(atomic_get):
	movl		4(%esp), %edx
_atomic_get1:
	movl		(%edx), %eax
	movl		%eax, %ecx
	lock
	cmpxchgl	%ecx, (%edx)
	jnz			_atomic_get1
	ret
FUNCTION_END(atomic_get)

/* int64	atomic_set64(vint64 *value, int64 newValue) */
FUNCTION(atomic_set64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi	/* value */
	movl		16(%esp), %ebx	/* newValue low */
	movl		20(%esp), %ecx	/* newValue high */
_atomic_set64_1:
	movl		(%esi), %eax	/* testAgainst low */
	movl		4(%esi), %edx	/* testAgainst high */
	lock
	cmpxchg8b	(%esi)
	jnz			_atomic_set64_1
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_set64)

/* int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
FUNCTION(atomic_test_and_set64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi	/* value */
	movl		16(%esp), %ebx	/* newValue low */
	movl		20(%esp), %ecx	/* newValue high */
	movl		24(%esp), %eax	/* testAgainst low */
	movl		28(%esp), %edx	/* testAgainst high */
	lock
	cmpxchg8b	(%esi)
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_test_and_set64)

/* int64	atomic_add64(vint64 *value, int64 addValue) */
FUNCTION(atomic_add64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
_atomic_add64_1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	addl		16(%esp), %ebx
	adcl		20(%esp), %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			_atomic_add64_1
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_add64)

/* int64	atomic_and64(vint64 *value, int64 andValue) */
FUNCTION(atomic_and64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
_atomic_and64_1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	andl		16(%esp), %ebx
	andl		20(%esp), %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			_atomic_and64_1
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_and64)

/* int64	atomic_or64(vint64 *value, int64 orValue) */
FUNCTION(atomic_or64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
_atomic_or64_1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	orl			16(%esp), %ebx
	orl			20(%esp), %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			_atomic_or64_1
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_or64)

/* int64	atomic_get64(vint64 *value) */
FUNCTION(atomic_get64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
_atomic_get64_1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			_atomic_get64_1
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_get64)

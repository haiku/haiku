/*
** Copyright 2003, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the OpenBeOS license.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* int64	atomic_set64(vint64 *value, int64 newValue) */
FUNCTION(atomic_set64):
	push		%ebp
	push		%ebx
	movl		12(%esp), %ebp	/* value */
	movl		16(%esp), %ebx	/* newValue low */
	movl		20(%esp), %ecx	/* newValue high */
_atomic_set64_1:
	movl		(%ebp), %eax	/* testAgainst low */
	movl		4(%ebp), %edx	/* testAgainst high */
	lock
	cmpxchg8b	(%ebp)
	jnz			_atomic_set64_1
	pop			%ebx
	pop			%ebp
	ret

/* int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
FUNCTION(atomic_test_and_set64):
	push		%ebp
	push		%ebx
	movl		12(%esp), %ebp	/* value */
	movl		16(%esp), %ebx	/* newValue low */
	movl		20(%esp), %ecx	/* newValue high */
	movl		24(%esp), %eax	/* testAgainst low */
	movl		28(%esp), %edx	/* testAgainst high */
	lock
	cmpxchg8b	(%ebp)
	pop			%ebx
	pop			%ebp
	ret

/* int64	atomic_add64(vint64 *value, int64 addValue) */
FUNCTION(atomic_add64):
	push		%ebp
	push		%ebx
	movl		12(%esp), %ebp
_atomic_add64_1:
	movl		(%ebp), %eax
	movl		4(%ebp), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	addl		16(%esp), %ebx
	adcl		20(%esp), %ecx
	lock
	cmpxchg8b	(%ebp)
	jnz			_atomic_add64_1
	pop			%ebx
	pop			%ebp
	ret

/* int64	atomic_and64(vint64 *value, int64 andValue) */
FUNCTION(atomic_and64):
	push		%ebp
	push		%ebx
	movl		12(%esp), %ebp
_atomic_and64_1:
	movl		(%ebp), %eax
	movl		4(%ebp), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	andl		16(%esp), %ebx
	andl		20(%esp), %ecx
	lock
	cmpxchg8b	(%ebp)
	jnz			_atomic_and64_1
	pop			%ebx
	pop			%ebp
	ret

/* int64	atomic_or64(vint64 *value, int64 orValue) */
FUNCTION(atomic_or64):
	push		%ebp
	push		%ebx
	movl		12(%esp), %ebp
_atomic_or64_1:
	movl		(%ebp), %eax
	movl		4(%ebp), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	orl			16(%esp), %ebx
	orl			20(%esp), %ecx
	lock
	cmpxchg8b	(%ebp)
	jnz			_atomic_or64_1
	pop			%ebx
	pop			%ebp
	ret

/* int64	atomic_get64(vint64 *value) */
FUNCTION(atomic_get64):
	push		%ebp
	push		%ebx
	movl		12(%esp), %ebp
_atomic_get64_1:
	movl		(%ebp), %eax
	movl		4(%ebp), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	lock
	cmpxchg8b	(%ebp)
	jnz			_atomic_get64_1
	pop			%ebx
	pop			%ebp
	ret

/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 *
** Copyright 2003, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the MIT License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <asm_defs.h>


.text


/* void		atomic_set(int32* value, int32 newValue) */
FUNCTION(atomic_set):
	movl		4(%esp), %edx
	movl		8(%esp), %eax
	lock
	addl		$0, (%esp)
	movl		%eax, (%edx)
	ret
FUNCTION_END(atomic_set)


/* int32	atomic_get_and_set(int32* value, int32 newValue) */
FUNCTION(atomic_get_and_set):
	movl		4(%esp), %edx
	movl		8(%esp), %eax
	xchg		%eax, (%edx)
	ret
FUNCTION_END(atomic_get_and_set)


/* int32	atomic_test_and_set(int32* value, int32 newValue,
	int32 testAgainst) */
FUNCTION(atomic_test_and_set):
	movl		4(%esp), %edx
	movl		8(%esp), %ecx
	movl		12(%esp), %eax
	lock
	cmpxchgl	%ecx, (%edx)
	ret
FUNCTION_END(atomic_test_and_set)


/* int32	atomic_add(int32* value, int32 addValue) */
FUNCTION(atomic_add):
	movl		4(%esp), %edx
	movl		8(%esp), %eax
	lock
	xaddl		%eax, (%edx)
	ret
FUNCTION_END(atomic_add)


/* int32	atomic_and(int32* value, int32 andValue) */
FUNCTION(atomic_and):
	movl		4(%esp), %edx
1:
	movl		8(%esp), %ecx
	movl		(%edx), %eax
	andl		%eax, %ecx
	lock
	cmpxchgl	%ecx, (%edx)
	jnz			1b
	ret
FUNCTION_END(atomic_and)


/* int32	atomic_or(int32* value, int32 orValue) */
FUNCTION(atomic_or):
	movl		4(%esp), %edx
1:
	movl		8(%esp), %ecx
	movl		(%edx), %eax
	orl			%eax, %ecx
	lock
	cmpxchgl	%ecx, (%edx)
	jnz			1b
	ret
FUNCTION_END(atomic_or)


/* int32	atomic_get(int32* value) */
FUNCTION(atomic_get):
	movl		4(%esp), %edx
	movl		(%edx), %eax
	lock
	addl		$0, (%esp)
	ret
FUNCTION_END(atomic_get)


/* void		atomic_set64(int64* value, int64 newValue) */
FUNCTION(atomic_set64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi	/* value */
	movl		16(%esp), %ebx	/* newValue low */
	movl		20(%esp), %ecx	/* newValue high */
1:
	movl		(%esi), %eax	/* testAgainst low */
	movl		4(%esi), %edx	/* testAgainst high */
	lock
	cmpxchg8b	(%esi)
	jnz			1b
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_set64)


/* void		atomic_get_and_set64(int64* value, int64 newValue) */
FUNCTION(atomic_get_and_set64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi	/* value */
	movl		16(%esp), %ebx	/* newValue low */
	movl		20(%esp), %ecx	/* newValue high */
1:
	movl		(%esi), %eax	/* testAgainst low */
	movl		4(%esi), %edx	/* testAgainst high */
	lock
	cmpxchg8b	(%esi)
	jnz			1b
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_get_and_set64)


/* int64	atomic_test_and_set64(int64* value, int64 newValue,
	int64 testAgainst) */
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


/* int64	atomic_add64(int64* value, int64 addValue) */
FUNCTION(atomic_add64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	addl		16(%esp), %ebx
	adcl		20(%esp), %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			1b
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_add64)

/* int64	atomic_and64(int64* value, int64 andValue) */
FUNCTION(atomic_and64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	andl		16(%esp), %ebx
	andl		20(%esp), %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			1b
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_and64)


/* int64	atomic_or64(int64* value, int64 orValue) */
FUNCTION(atomic_or64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	orl			16(%esp), %ebx
	orl			20(%esp), %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			1b
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_or64)


/* int64	atomic_get64(int64* value) */
FUNCTION(atomic_get64):
	push		%esi
	push		%ebx
	movl		12(%esp), %esi
1:
	movl		(%esi), %eax
	movl		4(%esi), %edx
	movl		%eax, %ebx
	movl		%edx, %ecx
	lock
	cmpxchg8b	(%esi)
	jnz			1b
	pop			%ebx
	pop			%esi
	ret
FUNCTION_END(atomic_get64)


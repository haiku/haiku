/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


.text

/* int32 atomic_set(vint32 *value, int32 newValue) */
FUNCTION(atomic_set):
	movl		%esi, %eax
	lock
	xchgl		%eax, (%rdi)
	ret
FUNCTION_END(atomic_set)

/* int32 atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst) */
FUNCTION(atomic_test_and_set):
	movl		%edx, %eax
	lock
	cmpxchgl	%esi, (%rdi)
	ret
FUNCTION_END(atomic_test_and_set)

/* int32 atomic_add(vint32 *value, int32 addValue) */
FUNCTION(atomic_add):
	movl		%esi, %eax
	lock
	xaddl		%eax, (%rdi)
	ret
FUNCTION_END(atomic_add)

/* int32 atomic_and(vint32 *value, int32 andValue) */
FUNCTION(atomic_and):
	movl		(%rdi), %eax
1:	movl		%eax, %edx
	movl		%eax, %ecx
	andl		%esi, %edx
	lock
	cmpxchgl	%edx, (%rdi)
	jnz			1b
	movl		%ecx, %eax
	ret
FUNCTION_END(atomic_and)

/* int32 atomic_or(vint32 *value, int32 orValue) */
FUNCTION(atomic_or):
	movl		(%rdi), %eax
1:	movl		%eax, %edx
	movl		%eax, %ecx
	orl			%esi, %edx
	lock
	cmpxchgl	%edx, (%rdi)
	jnz			1b
	movl		%ecx, %eax
	ret
FUNCTION_END(atomic_or)

/* int32 atomic_get(vint32 *value) */
FUNCTION(atomic_get):
	movl		(%rdi), %eax
1:	lock
	cmpxchgl	%eax, (%rdi)
	jnz			1b
	ret
FUNCTION_END(atomic_get)

/* int64 atomic_set64(vint64 *value, int64 newValue) */
FUNCTION(atomic_set64):
	movq		%rsi, %rax
	lock
	xchgq		%rax, (%rdi)
	ret
FUNCTION_END(atomic_set64)

/* int64 atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst) */
FUNCTION(atomic_test_and_set64):
	movq		%rdx, %rax
	lock
	cmpxchgq	%rsi, (%rdi)
	ret
FUNCTION_END(atomic_test_and_set64)

/* int64 atomic_add64(vint64 *value, int64 addValue) */
FUNCTION(atomic_add64):
	movq		%rsi, %rax
	lock
	xaddq		%rax, (%rdi)
	ret
FUNCTION_END(atomic_add64)

/* int64 atomic_and64(vint64 *value, int64 andValue) */
FUNCTION(atomic_and64):
	movq		(%rdi), %rax
1:	movq		%rax, %rdx
	movq		%rax, %rcx
	andq		%rsi, %rdx
	lock
	cmpxchgq	%rdx, (%rdi)
	jnz			1b
	movq		%rcx, %rax
	ret
FUNCTION_END(atomic_and64)

/* int64 atomic_or64(vint64 *value, int64 orValue) */
FUNCTION(atomic_or64):
	movq		(%rdi), %rax
1:	movq		%rax, %rdx
	movq		%rax, %rcx
	orq			%rsi, %rdx
	lock
	cmpxchgq	%rdx, (%rdi)
	jnz			1b
	movq		%rcx, %rax
	ret
FUNCTION_END(atomic_or64)

/* int64 atomic_get64(vint64 *value) */
FUNCTION(atomic_get64):
	movq		(%rdi), %rax
1:	lock
	cmpxchgq	%rax, (%rdi)
	jnz			1b
	ret
FUNCTION_END(atomic_get64)

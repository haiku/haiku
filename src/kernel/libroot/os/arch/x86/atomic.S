/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* int atomic_add(int *val, int incr) */
FUNCTION(atomic_add):
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	lock
	xaddl	%eax,(%edx)
	ret

/* int atomic_and(int *val, int incr) */
FUNCTION(atomic_and):
	movl	4(%esp),%edx

_atomic_and1:
	movl	8(%esp),%ecx
	movl	(%edx),%eax
	andl	%eax,%ecx

	lock
	cmpxchgl	%ecx,(%edx)

	jnz		_atomic_and1

	ret

/* int atomic_or(int *val, int incr) */
FUNCTION(atomic_or):
	movl	4(%esp),%edx

_atomic_or1:
	movl	8(%esp),%ecx
	movl	(%edx),%eax
	orl		%eax,%ecx

	lock
	cmpxchgl	%ecx,(%edx)

	jnz		_atomic_or1

	ret

/* int atomic_set(int *val, int set_to) */
FUNCTION(atomic_set):
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	xchg	%eax,(%edx)
	ret

/* int test_and_set(int *val, int set_to, int test_val) */
FUNCTION(test_and_set):
	movl	4(%esp),%edx
	movl	8(%esp),%ecx
	movl	12(%esp),%eax

	lock
	cmpxchgl	%ecx,(%edx)

	ret

/* saves the conversion factor needed for system_time */
cv_factor:
	.word 0

FUNCTION(setup_system_time):
	movl	4(%esp),%eax
	movl	%eax,cv_factor
	ret

/* long long system_time(); */
FUNCTION(system_time):
	/* load 64-bit factor into %eax (low), %edx (high) */
	rdtsc		/* time in %edx,%eax */

	pushl	%ebx
	pushl	%ecx
	movl	cv_factor, %ebx
	movl	%edx, %ecx	/* save high half */
	mull	%ebx 		/* truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* save high half of low */
	mull	%ebx /*, %eax*/
	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%ebx, %edx
	popl	%ecx
	popl	%ebx
	ret

/* void i386_switch_stack_and_call(addr stack, void (*func)(void *), void *arg); */
FUNCTION(i386_switch_stack_and_call):
	movl	4(%esp),%eax	// new stack
	movl	8(%esp),%ecx	// func
	movl	12(%esp),%edx	// args

	movl	%eax,%esp		// switch the stack
	pushl	%edx			// push the argument
	call	*%ecx			// call the target function
_loop:
	jmp		_loop


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
.global cv_factor
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

/* void arch_cpu_global_TLB_invalidate(); */
FUNCTION(arch_cpu_global_TLB_invalidate):
	movl	%cr3,%eax
	movl	%eax,%cr3
	ret

/* void i386_fsave_swap(void *old_fpu_state, void *new_fpu_state); */
FUNCTION(i386_fsave_swap):
	movl	4(%esp),%eax
	fsave	(%eax)
	movl	8(%esp),%eax
	frstor	(%eax)
	ret

/* void i386_fxsave_swap(void *old_fpu_state, void *new_fpu_state); */
FUNCTION(i386_fxsave_swap):
	movl	4(%esp),%eax
	fxsave	(%eax)
	movl	8(%esp),%eax
	fxrstor	(%eax)
	ret

/* void i386_context_switch(unsigned int **old_esp, unsigned int *new_esp, addr new_pgdir); */
FUNCTION(i386_context_switch):
	pusha					/* pushes 8 words onto the stack */
	movl	36(%esp),%eax
	movl	%esp,(%eax)
	movl	44(%esp),%eax	/* get possible new pgdir */
	cmpl	$0x0,%eax		/* is it null? */
	je		skip_pgdir_swap
	movl	%eax,%cr3
skip_pgdir_swap:
	movl	40(%esp),%eax
	movl	%eax,%esp
	popa
	ret

/* void i386_swap_pgdir(addr new_pgdir); */
FUNCTION(i386_swap_pgdir):
	movl	4(%esp),%eax
	movl	%eax,%cr3
	ret

/* thread exit stub */
	.align 4
i386_uspace_exit_stub:
	pushl	%eax
	movl	$1, %ecx
	lea	(%esp), %edx
	movl	$25, %eax;
	int	$99
	.align 4
i386_uspace_exit_stub_end:


/* void i386_enter_uspace(addr entry, void *args, addr ustack_top); */
FUNCTION(i386_enter_uspace):
	movl	4(%esp),%eax	// get entry point
	movl	8(%esp),%edx	// get arguments
	movl	12(%esp),%ebx	// get user stack
	movw	$0x23,%cx
	movw	%cx,%ds
	movw	%cx,%es
	movw	%cx,%fs
	movw	%cx,%gs

	// copy exit stub to stack
	movl	$i386_uspace_exit_stub_end, %esi
_copy_more:
	lea	-4(%esi), %esi
	lea	-4(%ebx), %ebx
	mov	(%esi), %ecx
	mov	%ecx, (%ebx)
	cmp	$i386_uspace_exit_stub, %esi
	jg	_copy_more


	// push the args onto the user stack
	movl	%edx,-4(%ebx)	// args
	movl	%ebx,-8(%ebx)	// fake return address to copied exit stub
	sub		$8,%ebx

	pushl	$0x23			// user data segment
	pushl	%ebx			// user stack
	pushl	$(1 << 9) | 2	// user flags
	pushl	$0x1b			// user code segment
	pushl	%eax			// user IP
	iret

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

null_idt_descr:
	.word	0
	.word	0,0

FUNCTION(reboot):
	lidt	null_idt_descr
	int		$0
done:
	jmp		done


FUNCTION(dbg_save_registers):
	pushl	%esi
	pushl	%eax
	movl	12(%esp), %esi

	movl	%eax, 0(%esi)
	movl	%ebx, 4(%esi)
	movl	%ecx, 8(%esi)
	movl	%edx, 12(%esi)

	lea	16(%esp), %eax
	movl	%eax, 16(%esi)	// caller's %esp
	movl	%ebp, 20(%esi)

	movl	4(%esp), %eax
	movl	%eax, 24(%esi)	// caller's %esi
	movl	%edi, 28(%esi)

	movl	8(%esp), %eax
	movl	%eax, 32(%esi)	// caller's %ebp


	pushfl
	popl	%eax
	mov	%eax, 36(%esi)

	movl	%cs, 40(%esi)
	movl	%ss, 44(%esi)
	movl	%ds, 48(%esi)
	movl	%es, 52(%esi)

	popl	%eax
	popl	%esi
	ret

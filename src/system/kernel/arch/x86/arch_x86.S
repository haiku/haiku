/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Copyright 2002, Michael Noisternig. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/x86/descriptors.h>

#include "syscall_numbers.h"

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* void arch_cpu_user_TLB_invalidate(); */
FUNCTION(arch_cpu_user_TLB_invalidate):
	movl	%cr3,%eax
	movl	%eax,%cr3
	ret

/* void i386_fnsave(void *fpu_state); */
FUNCTION(i386_fnsave):
	movl	4(%esp), %eax
	fnsave	(%eax)
	ret

/* void i386_fxsave(void *fpu_state); */
FUNCTION(i386_fxsave):
	movl	4(%esp), %eax
	fxsave	(%eax)
	ret

/* void i386_frstor(const void *fpu_state); */
FUNCTION(i386_frstor):
	movl	4(%esp), %eax
	frstor	(%eax)
	ret

/* void i386_fxrstor(const void *fpu_state); */
FUNCTION(i386_fxrstor):
	movl	4(%esp), %eax
	fxrstor	(%eax)
	ret

/* void i386_fsave_swap(void *old_fpu_state, const void *new_fpu_state); */
FUNCTION(i386_fnsave_swap):
	movl	4(%esp),%eax
	fnsave	(%eax)
	movl	8(%esp),%eax
	frstor	(%eax)
	ret

/* void i386_fxsave_swap(void *old_fpu_state, const void *new_fpu_state); */
FUNCTION(i386_fxsave_swap):
	movl	4(%esp),%eax
	fxsave	(%eax)
	movl	8(%esp),%eax
	fxrstor	(%eax)
	ret

/* uint32 x86_read_ebp(); */
FUNCTION(x86_read_ebp):
	movl	%ebp, %eax
	ret

/* uint32 x86_read_cr0(); */
FUNCTION(x86_read_cr0):
	movl	%cr0, %eax
	ret

/* void x86_write_cr0(uint32 value); */
FUNCTION(x86_write_cr0):
	movl	4(%esp), %eax
	movl	%eax, %cr0
	ret

/* uint32 x86_read_cr4(); */
FUNCTION(x86_read_cr4):
	movl	%cr4, %eax
	ret

/* void x86_write_cr4(uint32 value); */
FUNCTION(x86_write_cr4):
	movl	4(%esp), %eax
	movl	%eax, %cr4
	ret

/* uint64 x86_read_msr(uint32 register); */
FUNCTION(x86_read_msr):
	movl	4(%esp), %ecx
	rdmsr
	ret

/* void x86_write_msr(uint32 register, uint64 value); */
FUNCTION(x86_write_msr):
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	movl	12(%esp), %edx
	wrmsr
	ret

/* void i386_context_switch(struct arch_thread *old_state, struct arch_thread *new_state, addr new_pgdir); */
FUNCTION(i386_context_switch):
	pusha					/* pushes 8 words onto the stack */
	movl	36(%esp),%eax	/* save old_state->current_stack */
	movl	%esp,(%eax)
	pushl	%ss
	popl	%edx
	movl	%edx,4(%eax)
	movl	44(%esp),%eax	/* get possible new pgdir */
	orl		%eax,%eax		/* is it null? */
	je		skip_pgdir_swap
	movl	%eax,%cr3
skip_pgdir_swap:
	movl	40(%esp),%eax	/* get new new_state->current_stack */
	lss		(%eax),%esp
	popa
	ret

/* void i386_swap_pgdir(addr new_pgdir); */
FUNCTION(i386_swap_pgdir):
	movl	4(%esp),%eax
	movl	%eax,%cr3
	ret

/* thread exit stub - is copied to the userspace stack in arch_thread_enter_uspace() */
	.align 4
FUNCTION(x86_userspace_thread_exit):
	pushl	%eax
	movl	$1, %ecx
	lea		(%esp), %edx
	movl	$SYSCALL_EXIT_THREAD, %eax;
	int		$99
	.align 4
FUNCTION(x86_end_userspace_thread_exit):


/* void x86_enter_userspace(addr_t entry, addr_t stackTop); */
FUNCTION(x86_enter_userspace):
	movl	4(%esp), %eax	// get entry point
	movl	8(%esp), %ebx	// get user stack
	movw	$USER_DATA_SEG, %cx
	movw	%cx, %ds
	movw	%cx, %es
	//movw	$0x33 + cpu_num, %fs	-> fs points to the TLS storage (CPU dependent segment)
	movw	%cx, %gs

	xorl	%ebp, %ebp		// this is the last stack frame - we don't need one on return
							// (%ebp marks the beginning of this stack frame)
	pushl	$USER_DATA_SEG	// user data segment
	pushl	%ebx			// user stack
	pushl	$(1 << 9) | 2	// user flags
	pushl	$USER_CODE_SEG	// user code segment
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


FUNCTION(arch_debug_save_registers):
	pushl	%esi
	pushl	%eax
	movl	12(%esp), %esi

	movl	%eax, 0(%esi)
	movl	%ebx, 4(%esi)
	movl	%ecx, 8(%esi)
	movl	%edx, 12(%esi)

	lea		16(%esp), %eax
	movl	%eax, 16(%esi)	// caller's %esp
	movl	%ebp, 20(%esi)

	movl	4(%esp), %eax
	movl	%eax, 24(%esi)	// caller's %esi
	movl	%edi, 28(%esi)

	movl	8(%esp), %eax
	movl	%eax, 32(%esi)	// caller's %ebp

	pushfl
	popl	%eax
	movl	%eax, 36(%esi)

	xor	%eax, %eax
	movw	%cs, %ax
	movl	%eax, 40(%esi)
	movw	%ss, %ax
	movl	%eax, 44(%esi)
	movw	%ds, %ax
	movl	%eax, 48(%esi)
	movw	%es, %ax
	movl	%eax, 52(%esi)

	popl	%eax
	popl	%esi
	ret

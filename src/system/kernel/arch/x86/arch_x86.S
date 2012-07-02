/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Copyright 2002, Michael Noisternig. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <asm_defs.h>

#include <arch/x86/descriptors.h>

#include "asm_offsets.h"
#include "syscall_numbers.h"


.text

/*! \fn void arch_cpu_user_TLB_invalidate()
	Invalidates the TLB. Must be called with interrupts disabled.
*/
FUNCTION(arch_cpu_user_TLB_invalidate):
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret
FUNCTION_END(arch_cpu_user_TLB_invalidate)

/* void i386_fnsave(void *fpu_state); */
FUNCTION(i386_fnsave):
	movl	4(%esp), %eax
	fnsave	(%eax)
	ret
FUNCTION_END(i386_fnsave)

/* void i386_fxsave(void *fpu_state); */
FUNCTION(i386_fxsave):
	movl	4(%esp), %eax
	fxsave	(%eax)
	ret
FUNCTION_END(i386_fxsave)

/* void i386_frstor(const void *fpu_state); */
FUNCTION(i386_frstor):
	movl	4(%esp), %eax
	frstor	(%eax)
	ret
FUNCTION_END(i386_frstor)

/* void i386_fxrstor(const void *fpu_state); */
FUNCTION(i386_fxrstor):
	movl	4(%esp), %eax
	fxrstor	(%eax)
	ret
FUNCTION_END(i386_fxrstor)

/* void i386_noop_swap(void *old_fpu_state, const void *new_fpu_state); */
FUNCTION(i386_noop_swap):
	nop
	ret
FUNCTION_END(i386_noop_swap)

/* void i386_fsave_swap(void *old_fpu_state, const void *new_fpu_state); */
FUNCTION(i386_fnsave_swap):
	movl	4(%esp),%eax
	fnsave	(%eax)
	movl	8(%esp),%eax
	frstor	(%eax)
	ret
FUNCTION_END(i386_fnsave_swap)

/* void i386_fxsave_swap(void *old_fpu_state, const void *new_fpu_state); */
FUNCTION(i386_fxsave_swap):
	movl	4(%esp),%eax
	fxsave	(%eax)
	movl	8(%esp),%eax
	fxrstor	(%eax)
	ret
FUNCTION_END(i386_fxsave_swap)

/* uint32 x86_read_ebp(); */
FUNCTION(x86_read_ebp):
	movl	%ebp, %eax
	ret
FUNCTION_END(x86_read_ebp)

/* uint32 x86_read_cr0(); */
FUNCTION(x86_read_cr0):
	movl	%cr0, %eax
	ret
FUNCTION_END(x86_read_cr0)

/* void x86_write_cr0(uint32 value); */
FUNCTION(x86_write_cr0):
	movl	4(%esp), %eax
	movl	%eax, %cr0
	ret
FUNCTION_END(x86_write_cr0)

/* uint32 x86_read_cr4(); */
FUNCTION(x86_read_cr4):
	movl	%cr4, %eax
	ret
FUNCTION_END(x86_read_cr4)

/* void x86_write_cr4(uint32 value); */
FUNCTION(x86_write_cr4):
	movl	4(%esp), %eax
	movl	%eax, %cr4
	ret
FUNCTION_END(x86_write_cr4)

/* uint64 x86_read_msr(uint32 register); */
FUNCTION(x86_read_msr):
	movl	4(%esp), %ecx
	rdmsr
	ret
FUNCTION_END(x86_read_msr)

/* void x86_write_msr(uint32 register, uint64 value); */
FUNCTION(x86_write_msr):
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	movl	12(%esp), %edx
	wrmsr
	ret
FUNCTION_END(x86_write_msr)

/* void x86_context_switch(struct arch_thread* oldState,
	struct arch_thread* newState); */
FUNCTION(x86_context_switch):
	pusha					/* pushes 8 words onto the stack */
	movl	36(%esp),%eax	/* save oldState->current_stack */
	movl	%esp,(%eax)
	pushl	%ss
	popl	%edx
	movl	%edx,4(%eax)
	movl	40(%esp),%eax	/* get new newState->current_stack */
	lss		(%eax),%esp
	popa
	ret
FUNCTION_END(x86_context_switch)

/* void x86_swap_pgdir(uint32 newPageDir); */
FUNCTION(x86_swap_pgdir):
	movl	4(%esp),%eax
	movl	%eax,%cr3
	ret
FUNCTION_END(x86_swap_pgdir)

/* thread exit stub - is copied to the userspace stack in arch_thread_enter_uspace() */
	.align 4
FUNCTION(x86_userspace_thread_exit):
	pushl	%eax
	sub		$4, %esp
	movl	$1, %ecx
	lea		(%esp), %edx
	movl	$SYSCALL_EXIT_THREAD, %eax
	int		$99
	.align 4
FUNCTION_END(x86_userspace_thread_exit)
SYMBOL(x86_end_userspace_thread_exit):


null_idt_descr:
	.word	0
	.word	0,0

FUNCTION(reboot):
	lidt	null_idt_descr
	int		$0
done:
	jmp		done
FUNCTION_END(reboot)


/* status_t arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(arch_cpu_user_memcpy):
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%edi	/* dest */
	movl	16(%esp),%esi	/* source */
	movl	20(%esp),%ecx	/* count */

	/* set the fault handler */
	movl	24(%esp),%edx	/* fault handler */
	movl	(%edx),%eax
	movl	$.L_user_memcpy_error, (%edx)

	/* move by words */
	cld
	shrl	$2,%ecx
	rep
	movsl

	/* move any remaining data by bytes */
	movl	20(%esp),%ecx
	andl	$3,%ecx
	rep
	movsb

	/* restore the old fault handler */
	movl	%eax,(%edx)
	xor		%eax,%eax

	popl	%edi
	popl	%esi
	ret

	/* error condition */
.L_user_memcpy_error:
	/* restore the old fault handler */
	movl	%eax,(%edx)
	movl	$-1,%eax	/* return a generic error, the wrapper routine will deal with it */
	popl	%edi
	popl	%esi
	ret
FUNCTION_END(arch_cpu_user_memcpy)


/* status_t arch_cpu_user_memset(void *to, char c, size_t count, addr_t *faultHandler) */
FUNCTION(arch_cpu_user_memset):
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%edi	/* dest */
	movb	16(%esp),%al	/* c */
	movl	20(%esp),%ecx	/* count */

	/* set the fault handler */
	movl	24(%esp),%edx	/* fault handler */
	movl	(%edx),%esi
	movl	$.L_user_memset_error, (%edx)

	rep
	stosb

	/* restore the old fault handler */
	movl	%esi,(%edx)
	xor		%eax,%eax

	popl	%edi
	popl	%esi
	ret

	/* error condition */
.L_user_memset_error:
	/* restore the old fault handler */
	movl	%esi,(%edx)
	movl	$-1,%eax	/* return a generic error, the wrapper routine will deal with it */
	popl	%edi
	popl	%esi
	ret
FUNCTION_END(arch_cpu_user_memset)


/* ssize_t arch_cpu_user_strlcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(arch_cpu_user_strlcpy):
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	16(%esp),%edi	/* dest */
	movl	20(%esp),%esi	/* source */
	movl	24(%esp),%ecx	/* count */

	/* set the fault handler */
	movl	28(%esp),%edx	/* fault handler */
	movl	(%edx),%ebx
	movl	$.L_user_strlcpy_error, (%edx)

	/* Check for 0 length */
	cmp		$0,%ecx
	je		.L_user_strlcpy_source_count

	/* Copy at most count - 1 bytes */
	dec		%ecx

	/* If count is now 0, skip straight to null terminating
	   as our loop will otherwise overflow */
	jnz		.L_user_strlcpy_copy_begin
	movb	$0,(%edi)
	jmp		.L_user_strlcpy_source_count

.L_user_strlcpy_copy_begin:
	cld
.L_user_strlcpy_copy_loop:
	/* move data by bytes */
	movsb
	cmpb $0,-1(%esi)
	je .L_user_strlcpy_source_done
	loop .L_user_strlcpy_copy_loop

	/* null terminate string */
	movb	$0,(%edi)
	dec		%esi

	/* count remaining bytes in src */
.L_user_strlcpy_source_count:
	not		%ecx
	xor		%al,%al
	repnz
	scasb

.L_user_strlcpy_source_done:
	movl	%esi,%eax
	subl	20(%esp),%eax
	dec		%eax
	/* restore the old fault handler */
	movl	%ebx,(%edx)

	popl	%ebx
	popl	%edi
	popl	%esi
	ret

	/* error condition */
.L_user_strlcpy_error:
	/* restore the old fault handler */
	movl	%ebx,(%edx)
	movl	$-1,%eax	/* return a generic error, the wrapper routine will deal with it */
	popl	%ebx
	popl	%edi
	popl	%esi
	ret
FUNCTION_END(arch_cpu_user_strlcpy)


/*!	\fn void arch_debug_call_with_fault_handler(cpu_ent* cpu,
		jmp_buf jumpBuffer, void (*function)(void*), void* parameter)

	Called by debug_call_with_fault_handler() to do the dirty work of setting
	the fault handler and calling the function. If the function causes a page
	fault, the arch_debug_call_with_fault_handler() calls longjmp() with the
	given \a jumpBuffer. Otherwise it returns normally.

	debug_call_with_fault_handler() has already saved the CPU's fault_handler
	and fault_handler_stack_pointer and will reset them later, so
	arch_debug_call_with_fault_handler() doesn't need to care about it.

	\param cpu The \c cpu_ent for the current CPU.
	\param jumpBuffer Buffer to be used for longjmp().
	\param function The function to be called.
	\param parameter The parameter to be passed to the function to be called.
*/
FUNCTION(arch_debug_call_with_fault_handler):
	push	%ebp
	movl	%esp, %ebp

	// Set fault handler address, and fault handler stack pointer address. We
	// don't need to save the previous values, since that's done by the caller.
	movl	8(%ebp), %eax	// cpu to %eax
	lea		1f, %edx
	movl	%edx, CPU_ENT_fault_handler(%eax)
	movl	%ebp, CPU_ENT_fault_handler_stack_pointer(%eax)

	// call the function
	movl	20(%ebp), %eax	// parameter
	push	%eax
	movl	16(%ebp), %eax	// function
	call	*%eax

	// regular return
	movl	%ebp, %esp
	pop		%ebp
	ret

	// fault -- return via longjmp(jumpBuffer, 1)
1:
	movl	%ebp, %esp		// restore %esp
	pushl	$1
	movl	12(%ebp), %eax	// jumpBuffer
	pushl	%eax
	call	longjmp
FUNCTION_END(arch_debug_call_with_fault_handler)

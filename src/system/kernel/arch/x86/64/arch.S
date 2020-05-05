/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Copyright 2002, Michael Noisternig. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <asm_defs.h>

#include "asm_offsets.h"
#include "syscall_numbers.h"


.text


/* addr_t x86_get_stack_frame(); */
FUNCTION(x86_get_stack_frame):
	mov		%rbp, %rax
	ret
FUNCTION_END(x86_get_stack_frame)


/* void x86_64_thread_entry(); */
FUNCTION(x86_64_thread_entry):
	xorq	%rbp, %rbp

	movq	%rsp, %rax
	addq	$16, %rsp
	andq	$0xfffffffffffffff0, %rsp
	subq	$8, %rsp

	movq	8(%rax), %rdi
	jmp		*(%rax)
FUNCTION_END(x86_64_thread_entry)


/* thread exit stub */
.align 8
FUNCTION(x86_userspace_thread_exit):
	movq	%rax, %rdi
	movq	$SYSCALL_EXIT_THREAD, %rax
	syscall
.align 8
FUNCTION_END(x86_userspace_thread_exit)
SYMBOL(x86_end_userspace_thread_exit):


null_idt_descr:
	.word	0
	.quad	0

FUNCTION(x86_reboot):
	lidt	null_idt_descr
	int		$0
done:
	jmp		done
FUNCTION_END(x86_reboot)


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
	push	%rbp
	movq	%rsp, %rbp

	// Preserve the jump buffer address for the fault return.
	push	%rsi

	// Set fault handler address, and fault handler stack pointer address. We
	// don't need to save the previous values, since that's done by the caller.
	movq	$.L_debug_call_fault_handler, CPU_ENT_fault_handler(%rdi)
	movq	%rbp, CPU_ENT_fault_handler_stack_pointer(%rdi)

	// Call the function.
	movq	%rcx, %rdi
	call	*%rdx

	// Regular return.
	movq	%rbp, %rsp
	pop		%rbp
	ret

.L_debug_call_fault_handler:
	// Fault -- return via longjmp(jumpBuffer, 1)
	movq	%rbp, %rsp
	movq	-8(%rsp), %rdi
	movq	$1, %rsi
	call	longjmp
FUNCTION_END(arch_debug_call_with_fault_handler)


	.section .rodata
FUNCTION(_stac):
	stac
FUNCTION_END(_stac)

FUNCTION(_clac):
	clac
FUNCTION_END(_clac)

FUNCTION(_xsave):
	xsave64		(%rdi)
FUNCTION_END(_xsave)

FUNCTION(_xsavec):
	xsavec64	(%rdi)
FUNCTION_END(_xsavec)

FUNCTION(_xrstor):
	xrstor64	(%rdi)
FUNCTION_END(_xrstor)


/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include <arch/x86/descriptors.h>
#include <arch/x86/arch_cpu.h>

#include "asm_offsets.h"


// Push the remainder of the interrupt frame onto the stack.
#define PUSH_IFRAME_BOTTOM(iframeType)	\
	push	%rax;						\
	push	%rbx;						\
	push	%rcx;						\
	push	%rdx;						\
	push	%rdi;						\
	push	%rsi;						\
	push	%rbp;						\
	push	%r8;						\
	push	%r9;						\
	push	%r10;						\
	push	%r11;						\
	push	%r12;						\
	push	%r13;						\
	push	%r14;						\
	push	%r15;						\
	push	$iframeType;

// Restore the interrupt frame.
#define RESTORE_IFRAME()				\
	add		$8, %rsp;					\
	pop		%r15;						\
	pop		%r14;						\
	pop		%r13;						\
	pop		%r12;						\
	pop		%r11;						\
	pop		%r10;						\
	pop		%r9;						\
	pop		%r8;						\
	pop		%rbp;						\
	pop		%rsi;						\
	pop		%rdi;						\
	pop		%rdx;						\
	pop		%rcx;						\
	pop		%rbx;						\
	pop		%rax;

// The following code defines the interrupt service routines for all 256
// interrupts. It creates a block of handlers, each 16 bytes, that the IDT
// initialization code just loops through.

// Interrupt with no error code, pushes a 0 error code.
#define DEFINE_ISR(nr)					\
	.align 16;							\
	push	$0;							\
	push	$nr;						\
	jmp		int_bottom;

// Interrupt with an error code.
#define DEFINE_ISR_E(nr)				\
	.align 16;							\
	push	$nr;						\
	jmp		int_bottom;

// Array of interrupt service routines.
.align 16
SYMBOL(isr_array):
	// Exceptions (0-19) and reserved interrupts (20-31).
	DEFINE_ISR(0)
	DEFINE_ISR(1)
	DEFINE_ISR(2)
	DEFINE_ISR(3)
	DEFINE_ISR(4)
	DEFINE_ISR(5)
	DEFINE_ISR(6)
	DEFINE_ISR(7)
	DEFINE_ISR_E(8)
	DEFINE_ISR(9)
	DEFINE_ISR_E(10)
	DEFINE_ISR_E(11)
	DEFINE_ISR_E(12)
	DEFINE_ISR_E(13)
	DEFINE_ISR_E(14)
	DEFINE_ISR(15)
	DEFINE_ISR(16)
	DEFINE_ISR_E(17)
	DEFINE_ISR(18)
	DEFINE_ISR(19)
	DEFINE_ISR(20)
	DEFINE_ISR(21)
	DEFINE_ISR(22)
	DEFINE_ISR(23)
	DEFINE_ISR(24)
	DEFINE_ISR(25)
	DEFINE_ISR(26)
	DEFINE_ISR(27)
	DEFINE_ISR(28)
	DEFINE_ISR(29)
	DEFINE_ISR(30)
	DEFINE_ISR(31)

	// User-defined ISRs (32-255) - none take an error code.
	.Lintr = 32
	.rept 224
		DEFINE_ISR(.Lintr)
		.Lintr = .Lintr+1
	.endr


// Common interrupt handling code.
FUNCTION(int_bottom):
	// If coming from user-mode, need to load the kernel GS segment base.
	testl	$3, 24(%rsp)
	jz		1f
	swapgs
1:
	// Push the rest of the interrupt frame to the stack.
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_OTHER)

	cld

	// Frame pointer is the iframe.
	movq	%rsp, %rbp

	// Call the interrupt handler.
	movq	%rsp, %rdi
	movq	IFRAME_vector(%rsp), %rax
	call	*gInterruptHandlerTable(, %rax, 8)

	// Restore the saved registers.
	RESTORE_IFRAME()

	// Get rid of the error code and interrupt number, restore the previous
	// GS base if returning to user-mode, and return.
	addq	$16, %rsp
	testl	$3, 8(%rsp)
	jz		2f
	swapgs
2:	iretq
FUNCTION_END(int_bottom)


/*!	\fn void x86_return_to_userland(iframe* frame)
	\brief Returns to the userland environment given by \a frame.

	Before returning to userland all potentially necessary kernel exit work is
	done.

	\a frame must point to a location somewhere on the caller's stack (e.g. a
	local variable).
	The function must be called with interrupts disabled.

	\param frame The iframe defining the userland environment.
*/
FUNCTION(x86_return_to_userland):
	ud2a
FUNCTION_END(x86_return_to_userland)

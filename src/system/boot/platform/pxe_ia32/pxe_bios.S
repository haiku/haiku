/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Copyright 2006, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


/** This file contains code to call PXE BIOS functions out of a protected
 *	mode environment. It doesn't use the virtual86 mode - it switches
 *	to real mode, make the BIOS call, and switch back to protected
 *	mode again. It's meant to be used in a single-threaded boot loader,
 *	not in a multi-tasking operating system.
 *	It relies on the real mode segment descriptors found in pxe_stage1.S,
 *	and uses support functions from ../bios_ia32/bios.S
 */

#define FUNCTION(x) .globl x ; x ## :

#define REAL_MODE_STACK	0x9000
	// the location of the stack in real mode

#define SAVED_ESP		0x10000
#define SAVED_CR3		0x10004
#define SAVED_EAX		0x10008
#define SAVED_ES		0x1000c
#define SAVED_FLAGS		0x10010
	// we're overwriting the start of our boot loader to hold some
	// temporary values - the first 1024 bytes of it are used at
	// startup only, and we avoid some linking issues this way


.text
.code32


/** void call_pxe_bios(...)
 *	Does a call to the PXE BIOS functions in real mode.
 */

FUNCTION(call_pxe_bios)
	pushal
	pushfl

	// make sure the correct IDT is in place
	lidt 	idt_descriptor

	// get the interrupt vector and patch the instruction at the target address
	movl	40(%esp), %eax

	// Fills registers from the passed in structure
	// Since switch_to_real_mode() clobbers %eax, we have to handle
	// it specially here (by temporarily storing it to an arbitrary
	// memory location, SAVED_EAX)
	movl	44(%esp), %ebp

	movl	(%ebp), %eax
	movl	%eax, SAVED_EAX
	movl	4(%ebp), %ebx
	movl	8(%ebp), %ecx
	movl	12(%ebp), %edx
	movl	16(%ebp), %esi
	movl	20(%ebp), %edi
	movw	24(%ebp), %ax
	movw	%ax, SAVED_ES

	call	switch_to_real_mode

	.code16

	// restore %eax and %es from saved location
	movl	(SAVED_EAX - 0x10000), %eax
	movw	(SAVED_ES - 0x10000), %es

	// call the PXE hook



	// we're interested in the flags state as well
	pushf

	// save %eax from the call
	movl	%eax, (SAVED_EAX - 0x10000)

	// save flags from the call
	pop		%ax
	movw	%ax, (SAVED_FLAGS - 0x10000)

	// back to protected mode
	call	switch_to_protected_mode
	.code32

	// store the register state into the structure that has been passed in
	movl	44(%esp), %eax

	movl	%ebx, 4(%eax)
	movl	%ecx, 8(%eax)
	movl	%edx, 12(%eax)
	movl	%esi, 16(%eax)
	movl	%edi, 20(%eax)
	movl	SAVED_EAX, %ecx		// special handling for %eax and flags
	movl	%ecx, (%eax)
	movw	SAVED_FLAGS, %cx
	movw	%cx, 26(%eax)

	popfl
	popal

	ret

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

#define SAVED_EAX		0x10014
	// we're overwriting the start of our boot loader to hold some
	// temporary values - the first 1024 bytes of it are used at
	// startup only, and we avoid some linking issues this way


.text
.code32


/** uint16 call_pxe_bios(void *pxe, uint16 opcode, void *param)
 *	Does a call to the PXE BIOS functions in real mode.
 *  Both pxe and param must be as linear pointer into lower 1 MB,
 *  pxe is the pointer to the !PXE structure.
 */

FUNCTION(call_pxe_bios)

	pushal
	pushfl

	// make sure the correct IDT is in place
	lidt 	idt_descriptor

	// !PXE
	movl	40(%esp), %ebx
	// make %ebx a pointer to entry point SEG:OFS
	addl	$0x10, %ebx

	// opcode
	movl	44(%esp), %ecx

	// param
	movl	48(%esp), %edx

	// enter real mode (clobbers %eax, %ds, %es, %fs, %gs, %ss, %sp)
	call	switch_to_real_mode

	.code16

	// param (segment)
	movl	%edx, %eax
	shrl	$4, %eax
	pushw	%ax

	// param (offset)
	andw	$0xf, %dx
	pushw	%dx
	
	// opcode
	pushw	%cx

	// call PXE entry point
	movl	%ebx, %eax
	shrl	$4, %eax
	movw	%ax, %es
	andw	$0xf, %bx
	lcall *	%es:(%bx)

	addw	$6, %sp 

	// save %ax result
	xorl	%ebx, %ebx
	movw	%ax, %bx
	movl	%ebx, (SAVED_EAX - 0x10000)

	// back to protected mode (clobbers %eax)
	call	switch_to_protected_mode
	.code32

	popfl
	popal

	movl	SAVED_EAX, %eax

	ret

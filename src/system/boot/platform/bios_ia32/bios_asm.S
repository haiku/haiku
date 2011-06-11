/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


/*!	This file contains code to call BIOS functions out of a protected
	mode environment. It doesn't use the virtual86 mode - it switches
	to real mode, make the BIOS call, and switch back to protected
	mode again. It's meant to be used in a single-threaded boot loader,
	not in a multi-tasking operating system.
	It relies on the real mode segment descriptors found in shell.S.
*/


#define FUNCTION(x) .globl x ; x ## :

#define REAL_MODE_STACK	0x9000
	// the location of the stack in real mode

#define SAVED_ESP		0x10000
#define SAVED_CR3		0x10004
#define SAVED_EAX		0x10008
#define SAVED_ES		0x1000c
#define SAVED_FLAGS		0x10010
#define SAVED_EBP		0x10014
	// we're overwriting the start of our boot loader to hold some
	// temporary values - the first 1024 bytes of it are used at
	// startup only, and we avoid some linking issues this way


.text
.code32


/*!	This function brings you back to protected mode after you've
	switched to it using switch_to_real_mode().
	Should restore the whole environment to what it looked like
	before. Clobbers %eax.
*/
FUNCTION(switch_to_protected_mode)
	cli						// turn off interrupts

	.code16
	movl	%cr0, %eax		// set the PE bit (0) to switch to protected mode
	orb		$0x1, %al
	movl	%eax, %cr0

	.code32
	.byte	0x66			// jump to the protected mode segment
	ljmp	$0x8, $_protected_code_segment
_protected_code_segment:
	movw	$0x10, %ax		// setup data and stack selectors
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	// turn on paging again
	movl	SAVED_CR3, %eax	// restore the saved page directory
	orl		%eax, %eax		// is there a paging directory at all?
	jz		_no_paging;

	movl	%eax, %cr3

	movl	%cr0, %eax		// set the PG bit (31) to enable paging
	orl		$0x80000000, %eax
	movl	%eax, %cr0

_no_paging:
	// save the return address so that we can pick it up again later
	movl	(%esp), %eax
	movl	%eax, REAL_MODE_STACK

	// setup protected stack frame again
	movl	SAVED_ESP, %eax
	movl	%eax, %esp
	movl	SAVED_EBP, %ebp

	// copy the return address to the current stack
	movl	REAL_MODE_STACK, %eax
	movl	%eax, (%esp)

	ret

//--------------------------------------------------------------


/*!	Switches from protected mode back to real mode.
	It will disable paging and set the real mode segment selectors to 0x1000,
	except for the stack selector, which will be 0x0 (the stack is at 0x9000
	which is where the BFS boot loader puts it as well).
	Clobbers %eax.
*/
FUNCTION(switch_to_real_mode)
	// save the %esp register
	movl	%esp, %eax
	movl	%eax, SAVED_ESP

	movl	%ebp, SAVED_EBP

	// put the return address on the real mode stack
	movl	(%esp), %eax
	movl	%eax, REAL_MODE_STACK

	// disable paging
	movl	%cr3, %eax			// save the page directory address
	movl	%eax, SAVED_CR3

	movl	%cr0, %eax
	andl	$0x7fffffff, %eax	// clear PG bit (31)
	movl	%eax, %cr0

	xor		%eax, %eax			// clear page directory to flush TLBs
	movl	%eax, %cr3

	// setup real mode stack
	movl	$REAL_MODE_STACK, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	// setup selectors to point to our 16 bit segments
	movw	$0x20, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	$0x28, %ax
	movw	%ax, %ss

	ljmp	$0x18, $(_almost_real_code_segment - 0x10000)

_almost_real_code_segment:
	movl	%cr0, %eax			// switch to real mode
	andb	$0xfe, %al			// clear PE bit (0)
	movl	%eax, %cr0

	.byte	0x66
	ljmp	$0x1000, $(_real_code_segment - 0x10000)

_real_code_segment:
	.code16
	movw	$0x1000, %ax		// setup data & stack segments
	movw	%ax, %ds			// data in segment 0x1000,
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	xor		%ax, %ax			// stack in segment 0x0
	movw	%ax, %ss

	sti							// turn on interrupts again
	ret

	.code32

//--------------------------------------------------------------


/*!	void call_bios_internal(uint8 num, struct bios_regs *regs)
	Does a BIOS call by triggering a software interrupt in real
	mode.
*/
FUNCTION(call_bios_internal)
	pushal
	pushfl

	// make sure the correct IDT is in place
	lidt 	idt_descriptor

	// get the interrupt vector and patch the instruction at the target address
	movl	40(%esp), %eax
	mov		%al, int_number

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

	// call the interrupt (will be dynamically changed above)
	.byte	0xcd
int_number:
	.byte	0

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

//--------------------------------------------------------------


/*!	uint32  boot_key_in_keyboard_buffer()
	Search keyboard buffer for the keycodes for space in the first run, and,
	if not found - for the escape key at the last two positions of this buffer
*/
FUNCTION(boot_key_in_keyboard_buffer)
	pushal
	pushfl

	// make sure the correct IDT is in place
	lidt 	idt_descriptor

	call	switch_to_real_mode
	.code16

	cld
	push	%ds
	xorl	%eax, %eax
	mov		%ax, %ds
	mov		$0x41E, %si		// BIOS kbd buffer
search_cycle1:
	lodsw
	cmp		$0x3920, %ax	// test space key
	jz		to_ret
	cmp		$0x440, %si
	jnz		search_cycle1

	addw	0x41C, %si
	movw	-0x42(%si), %ax
	cmp		$0x011B, %ax	// test ESC key
	jz		to_ret
	movw	-0x44(%si), %ax
	cmp		$0x011B, %ax	// test ESC key
to_ret:
	pop		%ds

	// save %eax
	movl	%eax, (SAVED_EAX - 0x10000)

	call	switch_to_protected_mode
	.code32

	popfl
	popal

	// restore %eax
	movl	SAVED_EAX, %eax

	ret

//--------------------------------------------------------------


.globl idt_descriptor
idt_descriptor:
	.short	0x7ff				// IDT at 0x0, default real mode location
	.long	0x0

/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
// expects a stack page like this:
// (stack has to be at (0x9e000)
//	0x9effc            : final esp
//  0x9eff8            : page dir
//
//  0x9e000 - 0x9e006  : gdt descriptor
//	0x9e008 - 0x9e020  : gdt
//
// smp_trampoline must be located at 0x9f000
.globl smp_trampoline
.globl smp_trampoline_end
.globl foo

.code16
smp_trampoline:
	cli
	
	mov    $0x9e00,%ax
	mov    %ax,%ds

//	lgdt   0x9e000          # load the gdt
.byte 0x66, 0x0f, 0x01, 0x15, 0x00, 0xe0, 0x09, 0x00

	movl   %cr0,%eax
	orl    $0x01,%eax
	movl   %eax,%cr0              # switch into protected mode

.code32
_trampoline_32:
	.byte 0x66
	ljmp   $0x08,$(trampoline_32 - smp_trampoline + 0x9f000)
trampoline_32:
	mov    $0x10, %ax
	mov    %ax, %ds
	mov    %ax, %es
	mov    %ax, %fs
	mov    %ax, %gs
	mov    %ax, %ss

	movl   $0x9eff8,%esp    # set up the stack pointer

	popl   %eax             # get the page dir
	movl   %eax,%cr3        # set the page dir
	
	popl   %eax             # get the final stack location

	// Clear the final stack location to notify the startup code that
	// we loaded the address and it is now safe to be overwritten.
	pushl  $0x00000000

	movl   %eax,%esp

	// load an address for an indirect jump
	movl   $trampoline_after_paging,%ecx

	movl   %cr0,%eax
	orl    $0x80000000,%eax
	movl   %eax,%cr0          # enable paging

	// jump to the address previously loaded. NOTE:
	// this address is the address at which the code is originally linked,
	// which is > 1MB. We will be out of the low memory at this point.
	jmp    *%ecx
trampoline_after_paging:
	// just return, the bsp would have set the return address to the
	// target function at the top of the passed stack
	ret

smp_trampoline_end:


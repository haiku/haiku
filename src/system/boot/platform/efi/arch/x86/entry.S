/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <asm_defs.h>

#include <arch/x86/descriptors.h>

.text
.code32

/*
extern "C" void arch_enter_kernel(uint32_t pageDirectory, addr_t kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop,
	struct gdt_idt_descr *gdtDescriptor);
*/
FUNCTION(arch_enter_kernel):

	movl	4(%esp), %edx	// pageDirectory
	movl	8(%esp), %ecx	// kernelArgs
	movl	12(%esp), %ebx	// kernelEntry
	movl	16(%esp), %eax	// kernelStackTop
	movl	20(%esp), %esi	// gdtDescriptor

	// initialize stack
	movl	%eax, %esp

	// ensure that Paging is disabled
	movl	%cr0, %eax
	andl	$0x7fffffff, %eax
	movl	%eax, %cr0

	// ensure that PAE is disabled
	movl	%cr4, %eax
	andl	$0xffffffdf, %eax
	movl	%eax, %cr4

	// set page directory
	movl	%edx, %eax
	movl	%eax, %cr3

	// disable interrupts
	cli

	// clear direction flag
	cld

	// initialize floating point unit
	fninit

	movl	%esi, %eax
	lgdt	(%eax)

	// initialize CR0
	// - bit #31: Enable Paging
	// - bit #16: Write Protect
	// - bit  #5: Numeric Error Handling
	// - bit  #0: Protected Mode
	movl	$0x80010021, %eax
	movl	%eax, %cr0

	// Set data segments.
	movw	$KERNEL_DATA_SELECTOR, %ax
	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	pushl	$0x0		// currentCpu
	pushl	%ecx		// kernelArgs
	pushl	$0x0		// fake return address
	pushl	$KERNEL_CODE_SELECTOR
	pushl	%ebx		// kernelEntry
	lret

	//return
	movl	$-1, %eax
	ret
FUNCTION_END(arch_enter_kernel)

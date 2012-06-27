/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include <arch/x86_64/descriptors.h>


.code32


/*!	void long_enter_kernel(uint32 pml4, uint64 entry, uint64 stackTop,
		uint64 kernelArgs, int currentCPU);
*/
FUNCTION(long_enter_kernel):
	// Currently running with 32-bit paging tables at an identity mapped
	// address. To switch to 64-bit paging we must first disable 32-bit paging,
	// otherwise loading the new CR3 will fault.
	movl	%cr0, %eax
	andl	$~(1 << 31), %eax
	movl	%eax, %cr0

	// Enable PAE.
	movl	%cr4, %eax
	orl		$(1 << 5), %eax
	movl	%eax, %cr4

	// Point CR3 to the kernel's PML4.
	movl	4(%esp), %eax
	movl	%eax, %cr3

	// Enable long mode by setting EFER.LME.
	movl	$0xC0000080, %ecx
	rdmsr
	orl		$(1 << 8), %eax
	wrmsr

	// Re-enable paging, which will put us in compatibility mode as we are
	// currently in a 32-bit code segment.
	movl	%cr0, %ecx
	orl		$(1 << 31), %ecx
	movl	%ecx, %cr0

	// Jump into the 64-bit code segment.
	ljmp	$KERNEL_CODE_SEG, $.Llmode
.align 8
.code64
.Llmode:
	// Set data segments.
	mov		$KERNEL_DATA_SEG, %ax
	mov		%ax, %ss
	mov		%ax, %ds
	mov		%ax, %es
	mov		%ax, %fs
	mov		%ax, %gs

	// Clear the high 32 bits of RSP.
	movl	%esp, %esp

	// Get the entry point address, arguments and new stack pointer.
	movq	8(%rsp), %rax
	movq	24(%rsp), %rdi
	movl	32(%rsp), %esi
	movq	16(%rsp), %rsp

	// Clear the stack frame/RFLAGS.
	xorq	%rbp, %rbp
	push	$0
	popf

	// Call the kernel entry point.
	call	*%rax

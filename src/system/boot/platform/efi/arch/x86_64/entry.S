/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include <arch/x86/descriptors.h>


#define GDT_LIMIT 0x800


.code64


/*!	void arch_enter_kernel(uint64 pml4, uint64 entry_point, uint64 stackTop); */
FUNCTION(arch_enter_kernel):
	// Point CR3 to the kernel's PML4.
	movq	%rdi, %cr3

	// Load 64-bit enabled GDT
	lgdtq	gLongGDTR(%rip)

	// Jump into the 64-bit code segment.
        push    $KERNEL_CODE_SELECTOR
        lea     .Llmode(%rip), %rax
        push    %rax
        lretq
.align 8
.code64
.Llmode:
	// Set data segments.
	mov		$KERNEL_DATA_SELECTOR, %ax
	mov		%ax, %ss
	xor		%ax, %ax
	mov		%ax, %ds
	mov		%ax, %es
	mov		%ax, %fs
	mov		%ax, %gs

	// Set the stack pointer.
	movq	%rdx, %rsp

	// Clear the stack frame/RFLAGS.
	xorq	%rbp, %rbp
	push	$2
	popf

	// Get arguments and call the kernel entry point.
        mov     %rsi, %rax  // entry point
	leaq	gKernelArgs(%rip), %rdi
        xorl    %esi, %esi // current cpu
	call	*%rax


.data


SYMBOL(gLongGDTR):
	.word	BOOT_GDT_SEGMENT_COUNT * 8 - 1
SYMBOL(gLongGDT):
	.quad	0

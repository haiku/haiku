/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
** Distributed under the terms of the NewOS License.
*/

// Relocatable. Before calling, smp_trampoline_args should point
// to a struct trampoline_args (see smp.cpp). This pointer should
// be 16-byte aligned, below 1MB and identity-mapped.
.globl smp_trampoline
.globl smp_trampoline_end
.globl smp_trampoline_args

#include <asm_defs.h>

#include <arch/x86/descriptors.h>
#include "mmu.h"

.code16
smp_trampoline:
	cli

//  movl 0xdeadbeef, esi
	.byte 0x66
	.byte 0xbe
smp_trampoline_args:
	.long 0xdeadbeef

	// Load the trampoline args into ss.
	movl %esi, %eax
	shrl $4, %eax
	movw %ax, %ss
	xorw %sp, %sp

	// Switch to protected mode.
	popl %ebx
	popl %edx
	lgdt (%edx)

	movl   %cr0,%eax
	orl    $0x01,%eax
	movl   %eax,%cr0

	pushl  $8
	leal  (trampoline_32 - smp_trampoline)(%ebx), %eax
	pushl  %eax
	.byte 0x66
.code32
	lret

trampoline_32:
	mov    $0x10, %ax
	mov    %ax, %ds
	mov    %ax, %es
	mov    %ax, %fs
	mov    %ax, %gs
	mov    %ax, %ss
	// Put the trampoline args on the stack.
	movl   %esi, %esp
	addl   $8, %esp

	// Enable PAE and PGE
	movl	%cr4, %eax
	orl		$(1 << 5) | (1 << 7), %eax
	movl	%eax, %cr4

	// Point CR3 to the kernel's PML4.
	popl	%eax
	movl	%eax, %cr3

	// Enable long mode by setting EFER.LME.
	movl	$0xc0000080, %ecx
	rdmsr
	orl		$(1 << 8), %eax
	wrmsr

	// Re-enable paging, which will put us in compatibility mode as we are
	// currently in a 32-bit code segment.
	movl	%cr0, %ecx
	orl		$(1 << 31), %ecx
	movl	%ecx, %cr0

	// Load 64-bit enabled GDT
	popl	%eax
	lgdtl	(%eax)

	// Jump into the 64-bit code segment.
	pushl	$KERNEL_CODE_SELECTOR
	leal	(.Llmode - smp_trampoline)(%ebx), %eax
	pushl	%eax
	lret
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

	// Initialisation that comes from long_smp_start_kernel.
	movq	%cr0, %rax
	orq		$0x10000, %rax
	andq	$(~6), %rax
	movq	%rax, %cr0
	fninit
	movq	%cr4, %rax
	orq		$0x600, %rax
	movq	%rax, %cr4

	// Get kernel arguments.
	popq	%rax
	popq	%rdi
	popq	%rsi

	// Set the stack pointer, write to the sentinel and clear the stack frame/RFLAGS.
	popq	%rbp
	movq	$0, (%rsp)
	movq	%rbp, %rsp
	xorq	%rbp, %rbp
	push	$0
	popf

	// Call the entry point.
	call	*%rax
smp_trampoline_end:
